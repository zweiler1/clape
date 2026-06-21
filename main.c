#define CLAPE_IMPLEMENTATION
#include "clape.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct Options {
    char *file;
} Options;
Options options = (Options){
    .file = NULL,
};

void print_usage(FILE *const stream) {
    fprintf(stream,
        "Usage: clape <file> [OPTIONS]\n"    //
        "\n"                                 //
        "Available Options:\n"               //
        "  -h, --help           Show help\n" //
    );
}

int parse_command_line_args(int argc, char *args[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(args[i], "-h") == 0 || strcmp(args[i], "--help") == 0) {
            print_usage(stdout);
            return 0;
        }
        if (!is_valid_file_path(args[i])) {
            fprintf(                                                                       //
                stderr, "Error: The given file '%s' does not exist or is invalid", args[i] //
            );
            return 1;
        }
        options.file = args[i];
    }
    if (options.file == NULL) {
        fprintf(stderr, "Error: No file to run was passed to clape\n");
        print_usage(stderr);
        return 1;
    }
    return 0;
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

    clape_program_t program = {0};
    defer(clape_free_program, &program);
    if (!clape_parse(&program, tokens)) {
        return 1;
    }
    clape_interpret(&program);

    return 0;
}
