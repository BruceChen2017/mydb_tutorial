// REPL for sqlite
#include <stdio.h>
#include <stdlib.h> // malloc/free
#include <stdbool.h> // true
#include <string.h> // strcmp

typedef struct InputBuffer_t
{
    char * buffer;
    size_t buffer_length;
    size_t input_length;
} InputBuffer;

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

// for metacommand of the form `.xxx`
typedef enum {
    META_COMMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

MetaCommandResult do_meta_command(InputBuffer* input_buffer){
    if (strcmp(input_buffer->buffer, ".exit") == 0){
        // close_input_buffer(input_buffer);
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// add support for select and insert
// two step
// step1: prepare statement <-- check statement is valid and decide its type
typedef enum {STATEMENT_SELECT, STATEMENT_INSERT} StatementType;

typedef struct
{
    StatementType type;
} Statement;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement){
    if (strncmp(input_buffer->buffer, "select", 6) == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    if (strncmp(input_buffer->buffer, "insert", 6) == 0){
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

// step2: execute statement
void execute_statement(Statement* statement){
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        printf("We are going to insert.\n");
        break;
    case (STATEMENT_SELECT):
        printf("We are going to select.\n");
        break;
    }
} 


int main(int argc, char ** argv){
    InputBuffer* input_buffer = &(InputBuffer){
        NULL, 0, 0
    };
    while (true){
        print_prompt();
        read_input(input_buffer);
        if (input_buffer->buffer[0] == '.'){
            switch (do_meta_command(input_buffer))
            {
            case (META_COMMMAND_SUCCESS):
                continue;
            case (META_COMMAND_UNRECOGNIZED_COMMAND):
                printf("Unrecognized keyword at start of '%s'.\n",input_buffer->buffer);
                continue;
            }
        }

        Statement statement;
        switch (prepare_statement(input_buffer, &statement))
        {
        case (PREPARE_SUCCESS):
            break;
        case (PREPARE_UNRECOGNIZED_STATEMENT):
            printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
            continue;
        }
        execute_statement(&statement);
        printf("Executed.\n");
    }
}



