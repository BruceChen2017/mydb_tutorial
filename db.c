// REPL for sqlite
#include <stdio.h>
#include <stdlib.h> // malloc/free
#include <stdbool.h> // true
#include <string.h> // strcmp
#include <stdint.h> // uint32_t
#include <fcntl.h> // O_RDWR
#include <sys/stat.h> // S_IWUSR
#include <sys/types.h> // ssize_t
#include <errno.h> // errno
#include <unistd.h> // read/write

#define TABLE_MAX_PAGES 4
#define PAGE_SIZE 4096

// Row to store data
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct InputBuffer_t
{
    char * buffer;
    size_t buffer_length;
    size_t input_length;
} InputBuffer;

typedef struct
{
    int file_id;
    uint32_t file_len;
    void * pages[TABLE_MAX_PAGES];
} Pager;

typedef struct
{
    Pager * pager;
    uint32_t num_rows;
}Table;

typedef struct
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE+1];
    char email[COLUMN_EMAIL_SIZE+1];
} Row;

typedef enum {STATEMENT_SELECT, STATEMENT_INSERT} StatementType;

typedef struct
{
    StatementType type;
    Row row_to_insert; // only used by insert statement
} Statement;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

// for metacommand of the form `.xxx`
typedef enum {
    META_COMMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;


// initialize
InputBuffer* new_input_buffer(){
    InputBuffer * input_buffer = malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}

void print_prompt(){
    printf("db > ");
}

// read input by getline
void read_input(InputBuffer* input_buffer){
    // ssize_t getline(char **lineptr, size_t *n, FILE *stream); <-- <stdio.h>
    // return the number of characters read, including the delimiter character(i.e newline character)
    // before getline, if *lineptr == null, *n == 0, it will allocate a buffer for storing the line
    // Otherwises, if the buffer is not large enough to hold the line, getline() resizes it
    // with `realloc`, updating *lineptr and *n as necessary
    size_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
    if (bytes_read <= 0){
        printf("Error read input\n");
        exit(EXIT_FAILURE);
    }
    // ignore trailing newline
    input_buffer->input_length = bytes_read-1;
    input_buffer->buffer[bytes_read-1] = 0; // null terminated
}

// free heap-allocated memory
void close_input_buffer(InputBuffer * input_buffer){
    free(input_buffer->buffer);
    free(input_buffer);
}

/*
Like a B-tree, it will group rows into pages, 
but instead of arranging those pages as a tree it will arrange them as an array
*/

// compact representation of a row
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)NULL)-> Attribute)
#define ID_SIZE  size_of_attribute(Row, id)
#define USERNAME_SIZE size_of_attribute(Row, username)
#define EMAIL_SIZE size_of_attribute(Row, email)
#define ID_OFFSET 0
#define USERNAME_OFFSET (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET (USERNAME_OFFSET + USERNAME_SIZE)
#define ROW_SIZE (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)

#define ROWS_PER_PAGE (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)

Pager * pager_open(const char * filename){
    int fd = open(filename,
                O_RDWR | // Read/Write mode
                O_CREAT, // Create file if not exist
                S_IWUSR | //User write permission
                S_IRUSR  //User read permission
    );
    if (fd == -1){
        printf("Uable to open file.\n");
        exit(EXIT_FAILURE);
    }

    off_t file_len = lseek(fd, 0, SEEK_END);

    Pager * pager = malloc(sizeof(Pager));
    pager->file_id = fd;
    pager->file_len = file_len;

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++){
        pager->pages[i] = NULL;
    }

    return pager;
}
// initialize
// - opening the database file
// - initializing a pager data structure
// - initializing a table data structure
Table * db_open(const char * filename){
    Pager * pager = pager_open(filename);
    Table * table = malloc(sizeof(Table));
    table->num_rows = pager->file_len / ROW_SIZE;
    table->pager = pager;
    return table;
}

// close
// - flushes the page cache to disk
// - closes the database file
// - frees the memory for the Pager and Table data structures

void pager_flush(Pager * pager, uint32_t page_num, uint32_t size){
    off_t offset = lseek(pager->file_id, page_num * PAGE_SIZE, SEEK_SET);
    
    if (offset == -1){
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    // write into file
    ssize_t bytes_written = write(pager->file_id, pager->pages[page_num], size);

    if (bytes_written == -1){
        printf("Error writing: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

void db_close(Table * table){
    Pager * pager = table->pager;
    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for (uint32_t i = 0; i < num_full_pages; i++){
        if (pager->pages[i] == NULL){
            continue;
        }
        pager_flush(pager, i, PAGE_SIZE);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    // handle partial page
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if (num_additional_rows > 0){
        uint32_t page_num = num_full_pages;
        if (pager->pages[page_num] != NULL){
            pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
            free(pager->pages[page_num]);
            pager->pages[page_num] = NULL;
        }
    }

    // close file
    int result = close(pager->file_id);
    if (result == -1){
        printf("Error closing db file.\n");
        exit(EXIT_FAILURE);
    }

    // free 
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++){
        void* page = pager->pages[i];
        if (page){
            free(page);
            pager->pages[i] = NULL;
        }
    }
    free(pager);
    free(table);
}



MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table * table){
    if (strcmp(input_buffer->buffer, ".exit") == 0){
        close_input_buffer(input_buffer);
        db_close(table);
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// dst is page
void serialize_row(Row * src, void * dst){
    memcpy(dst + ID_OFFSET, &(src->id), ID_SIZE);
    memcpy(dst + USERNAME_OFFSET, &(src->username), USERNAME_SIZE);
    memcpy(dst + EMAIL_OFFSET, &(src->email), EMAIL_SIZE);
}

// src is page
void deserialize_row(void * src, Row * dst){
    memcpy(&(dst->id), src + ID_OFFSET, ID_SIZE);
    memcpy(&(dst->username), src + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(dst->email), src + EMAIL_OFFSET, EMAIL_SIZE);
}

// Rows should not cross page boundaries, 
// since pages probably won’t exist next to each other in memory
// Thus we need to find row position: which page + offset in that page
// row_num is target row number

// handling a cache miss
void* get_page(Pager * pager, uint32_t page_num){
    if (page_num >= TABLE_MAX_PAGES){
        printf("Tried to fetch page number out of bounds. %d > %d", page_num+1, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL){

        // Cache miss. Allocate memory and load from file
        void *page = malloc(PAGE_SIZE);
    
        uint32_t num_pages = pager->file_len / PAGE_SIZE;
        // partial page
        if (pager->file_len % PAGE_SIZE){
            num_pages += 1;
        }

        if (page_num <= num_pages){
            lseek(pager->file_id, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->file_id, page, PAGE_SIZE);
            if (bytes_read == -1){
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        
        pager->pages[page_num] = page;
    }

    return pager->pages[page_num];
}

void * row_slot(Table * table, uint32_t row_num){
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void * page = get_page(table->pager, page_num);
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}

// add support for select and insert
// two step
// step1: prepare statement <-- check statement is valid and decide its type

PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement){
        statement->type = STATEMENT_INSERT;
        // check string length before reading into row
        char * keyword = strtok(input_buffer->buffer, " ");
        char * id_string = strtok(NULL, " ");
        char * username = strtok(NULL, " ");
        char * email = strtok(NULL, " ");
        if (id_string == NULL || username == NULL || email == NULL){
            return PREPARE_SYNTAX_ERROR;
        }
        int id = atoi(id_string);
        if (id < 0){
            return PREPARE_NEGATIVE_ID;
        }
        if (strlen(username) > COLUMN_USERNAME_SIZE){
            return PREPARE_STRING_TOO_LONG;
        }
        if (strlen(email) > COLUMN_EMAIL_SIZE){
            return PREPARE_STRING_TOO_LONG;
        }

        statement->row_to_insert.id = id;
        strcpy(statement->row_to_insert.username, username);
        strcpy(statement->row_to_insert.email, email);

        return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement){
    if (strncmp(input_buffer->buffer, "select", 6) == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    if (strncmp(input_buffer->buffer, "insert", 6) == 0){
        return prepare_insert(input_buffer, statement);
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

// step2: execute statement

ExecuteResult execute_insert(Statement * statement, Table * table){
    if (table->num_rows >= TABLE_MAX_ROWS){
        return EXECUTE_TABLE_FULL;
    }
    Row * row_to_insert = &(statement->row_to_insert);
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;
    return EXIT_SUCCESS;
}

void print_row(Row * row){
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

ExecuteResult execute_select(Table * table){
    Row row;
    for (uint32_t i = 0; i < table->num_rows; i++){
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table * table){
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        return execute_insert(statement, table);
    case (STATEMENT_SELECT):
        return execute_select(table);
    }
    return EXECUTE_SUCCESS;
} 


int main(int argc, char ** argv){

    if (argc < 2){
        printf("Must supply a database filename.\n");
        exit(EXIT_FAILURE);
    }

    char * filename = argv[1];
    Table * table = db_open(filename);

    InputBuffer* input_buffer = new_input_buffer();
    while (true){
        print_prompt();
        read_input(input_buffer);
        if (input_buffer->buffer[0] == '.'){
            switch (do_meta_command(input_buffer, table))
            {
            case (META_COMMMAND_SUCCESS):
                continue;
            case (META_COMMAND_UNRECOGNIZED_COMMAND):
                printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
                continue;
            }
        }

        Statement statement;
        switch (prepare_statement(input_buffer, &statement))
        {
        case (PREPARE_SUCCESS):
            break;
        case (PREPARE_SYNTAX_ERROR):
            printf("Syntax error. Could not parse statement.\n");
            continue;
        case (PREPARE_STRING_TOO_LONG):
            printf("String is too long.\n");
            continue;
        case (PREPARE_NEGATIVE_ID):
            printf("ID must be nonnegative.\n");
            continue;
        case (PREPARE_UNRECOGNIZED_STATEMENT):
            printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
            continue;
        }
        switch (execute_statement(&statement, table)){
            case (EXECUTE_SUCCESS):
            printf("Executed.\n");
            break;
            case (EXECUTE_TABLE_FULL):
            printf("Error: Table is full.\n");
            break;
        }
        
    }
}



