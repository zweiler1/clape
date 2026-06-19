#define CLAPE_IMPLEMENTATION
#include "clape.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

typedef struct Options {
    char *file;
} Options;
Options options = (Options){
    .file = NULL,
};

void print_usage(FILE *const stream) {
    fprintf(stream,
        "Usage: clape [OPTIONS]\n"                     //
        "\n"                                           //
        "Available Options:\n"                         //
        "  -h, --help           Show help\n"           //
        "  -f, --file <file>    The file to execute\n" //
    );
}

bool is_valid_file_path(const char *path) {
    struct stat st;

    // Attempt to get file status
    if (stat(path, &st) != 0) {
        // Path does not exist or error occurred
        return false;
    }

    // Check if it is a regular file (not a directory, device, etc.)
    return S_ISREG(st.st_mode);
}

int parse_command_line_args(int argc, char *args[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(args[i], "-h") == 0 || strcmp(args[i], "--help") == 0) {
            print_usage(stdout);
            return 0;
        } else if (strcmp(args[i], "-f") == 0 || strcmp(args[i], "--file") == 0) {
            if (i + 1 >= argc) {
                fprintf(                                                              //
                    stderr, "Error: Expected a <file> after the '%s' flag\n", args[i] //
                );
                print_usage(stderr);
                return 1;
            }
            i++;
            if (!is_valid_file_path(args[i])) {
                fprintf(                                                                       //
                    stderr, "Error: The given file '%s' does not exist or is invalid", args[i] //
                );
                return 1;
            }
            options.file = args[i];
        }
    }
    if (options.file == NULL) {
        fprintf(stderr, "Error: No file to run was passed to clape\n");
        print_usage(stderr);
        return 1;
    }
    return 0;
}

char *load_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    // 1. Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 2. Allocate memory (+1 for null terminator)
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    // 3. Read file into buffer
    if (fread(buffer, 1, size, file) != (size_t)size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    // Add null-terminator to the string
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char *args[]) {
    const int clap_result = parse_command_line_args(argc, args);
    if (clap_result != 0) {
        return clap_result;
    }

    char *const loaded_file = load_file(options.file);
    defer(free, loaded_file);

    clape_arr_t *const tokens = clape_tokenize(loaded_file);
    defer(clape_free_tokens, tokens);
    for (size_t i = 0; i < tokens->len; i++) {
        token_t *const token = ACCESS_ARR_AT(token_t, tokens, i);
        printf("tokens[%lu] = ", i);
        clape_print_token(stdout, token);
        printf("\n");
    }
    printf("\n");

    clape_program_t program = {0};
    defer(clape_free_program, &program);
    if (!clape_parse(&program, tokens)) {
        return 1;
    }
    clape_print_program(&program);

    return 0;
}
