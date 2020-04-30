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


int main(int argc, char ** argv){
    InputBuffer* input_buffer = new_input_buffer();
    while (true){
        print_prompt();
        read_input(input_buffer);
        if (strcmp(input_buffer->buffer, ".exit") == 0){
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }else{
            printf("Unrecognizd command '%s'.\n", input_buffer->buffer);
        }
    }
}



