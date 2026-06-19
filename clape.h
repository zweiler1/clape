#pragma once

#define DEBUG_MODE

#ifdef DEBUG_MODE
#define CLAPE_IMPLEMENTATION
#endif

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ------ ARRAY IMPLEMENTATION ------

/// @struct `clape_arr_t`
/// @brief Structure representing a simple dynamic array
typedef struct clape_arr_t {
    /// @var `len`
    /// @brief The length of the array
    size_t len;
    /// @var `value`
    /// @brief The payload of the array, allocated as a variable member of bytes where the array
    /// elements are present directly inline in the array structure
    char value[];
} clape_arr_t;

/// @function `clape_arr_create`
/// @brief Creates a new dynamic arr of size `len` where each element is of size `type_size`
///
/// @param `len` The length of the array to create
/// @param `type_size` The size of the type which is meant to be put into the array
/// @return `clape_arr_t *` A newly created array of size `len`
clape_arr_t *clape_arr_create(const size_t type_size, const size_t len);

/// @function `clape_arr_append`
/// @brief Appends a single value to an array
///
/// @param `type_size` The size of the type which is meant to be appended
/// @param `arr` The array in which to append the value to
/// @param `value` The value to append to the array
void clape_arr_append(const size_t type_size, clape_arr_t **const arr, void *const value);

/// @macro `ACCESS_ARR_AT`
/// @brief Accesses the array at a given index and returns a pointer to the value stored in the
/// array
///
/// @param `type` The type of value stored in the array
/// @param `arr` The array to access
/// @param `idx` The index at which to access the array at
#define ACCESS_ARR_AT(type, arr, idx) ((type *)(void *)&(arr->value[(idx) * sizeof(type)]))

#ifdef CLAPE_IMPLEMENTATION

clape_arr_t *clape_arr_create(const size_t type_size, const size_t len) {
    const size_t arr_len = sizeof(clape_arr_t) + len * type_size;
    clape_arr_t *arr = (clape_arr_t *)malloc(arr_len);
    arr->len = len;
    return arr;
}

void clape_arr_append(const size_t type_size, clape_arr_t **const arr, void *const value) {
    const size_t new_arr_size = sizeof(clape_arr_t) + ((*arr)->len + 1) * type_size;
    *arr = (clape_arr_t *)realloc(*arr, new_arr_size);
    memcpy(&(*arr)->value[(*arr)->len * type_size], value, type_size);
    (*arr)->len++;
}

#endif

// ------ UTILITY -----

// Helper to generate unique names using __COUNTER__
#define _DEFER_CONCAT(a, b) a##b
#define _DEFER_NAME(a, b) _DEFER_CONCAT(a, b)

// Context structure to store the function pointer and argument
struct _defer_ctx {
    void (*fn)(void *);
    void *arg;
};

// The cleanup function called when the variable goes out of scope
static inline void _defer_cleanup(struct _defer_ctx *ctx) {
    if (ctx->fn) {
        ctx->fn(ctx->arg);
    }
}

// The defer macro
// Usage: defer(function_pointer, argument_to_pass);
#define defer(fn, ptr)                                                                             \
    struct _defer_ctx _DEFER_NAME(_defer_var_, __COUNTER__)                                        \
        __attribute__((cleanup(_defer_cleanup))) = {(void (*)(void *))(fn), (void *)(ptr)}

// ------ TYPES ------

/// @enum `clape_value_e`
/// @brief The enum to tag the clape value with
typedef enum {
    CLAPE_VAL_INT,
    // TODO:
    // CLAPE_VAL_FLOAT,
    // CLAPE_VAL_PRODUCT,
    // CLAPE_VAL_LIST,
} clape_value_e;

/// @struct `clape_value_t`
/// @brief Represents a single clape value
typedef struct clape_value_t {
    /// @var `tag`
    /// @brief The tag telling us which type of value this is
    clape_value_e tag;

    /// @var `value`
    /// @brief A union of all possible value types
    union {
        /// @variation `ival`
        /// @brief An `Int` literal value
        int64_t ival;
    } value;
} clape_value_t;

/// @enum `clape_expr_e`
/// @brief The enum to tag the clape expr with
typedef enum {
    CLAPE_EXPR_LIT,
} clape_expr_e;

/// @struct `clape_expr_t`
/// @brief A clape expression
typedef struct clape_expr_t {
    /// @var `tag`
    /// @brief The tag telling us which type of expression this is
    clape_expr_e tag;

    /// @var `value`
    /// @brief A union of all possible expression types
    union {
        /// @variation `lit`
        /// @brief A `lit` variation is a literal like `69`, `3.14`, `true` or `false`
        clape_value_t lit;
    } value;
} clape_expr_t;

/// @struct `clape_stmt_t`
/// @brief A clape statement is a `let xyz = ...` line
typedef struct clape_stmt_t {
    /// @var `name`
    /// @brief The name of the SSA initialized value, if the name is NULL, then it's a `let _ = ...`
    /// line, which marks side-effects
    char *name;

    /// @var `expr`
    /// @brief A single expression which will be the RHS of the SSA statement
    clape_expr_t expr;
} clape_stmt_t;

/// @struct `clape_program_t`
/// @brief A clape program is a list of statements to evaluate in that exact order
typedef struct clape_program_t {
    /// @var `statements`
    /// @brief A dynamic array containing all parsed statements, every value of the dynamic array is
    /// of type `clape_stmt_t`
    clape_arr_t *statements;
} clape_program_t;

/// @enum `token_type_e`
/// @brief The enum of all possible Clape token types
typedef enum : uint8_t {
    // Symbols
    TOK_EQ = 0,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    // Keywords
    TOK_LET,
    // Literals
    TOK_INT,
    // Other
    TOK_IDENTIFIER,
    TOK_EOF,
} token_type_e;

/// @struct `token_t`
/// @brief The structure representing a single parsed token
typedef struct token_t {
    /// @var `tag`
    /// @brief The tag telling us which type of token this is
    token_type_e tag;

    /// @var `value`
    /// @brief A union of all possible value types
    union {
        /// @variation `ival`
        /// @brief An `Int` literal value
        int64_t ival;

        /// @variation `identifier`
        /// @brief An identifier token value
        char *identifier;
    } value;
} token_t;

// ------ LEXING ------

/// @function `clape_tokenize`
/// @brief Tokenizes a given file content and returns an array of `token_t` values as a dynamic
/// array. If tokenization failed in some way, the returned array is NULL
///
/// @param `file_content` The file content to tokenize
/// @return `clape_arr_t *` A dynamic `token_t` array containing all tokenized tokens
///
/// @note All identifiers of all tokens are allocated individually (for now) so they also need to be
/// freed indiviadually as well
[[nodiscard]] clape_arr_t *clape_tokenize(char *const file_content);

/// @function `clape_print_token`
/// @brief Prints a given token to the given file stream
///
/// @param `stream` The file stream to print the given token to
/// @param `token` The token to print
void clape_print_token(FILE *const stream, token_t *const token);

/// @function `clape_free_tokens`
/// @brief Frees a given token array and frees all potentially allocated memory inside (from
/// identifiers for example)
///
/// @param `arr` The array to free deeply
void clape_free_tokens(clape_arr_t *const tokens);

#ifdef CLAPE_IMPLEMENTATION

static bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

clape_arr_t *clape_tokenize(char *const file_content) {
    if (!file_content) {
        return NULL;
    }

    // Create an emppty tokens array which will be filled gradually
    clape_arr_t *tokens = clape_arr_create(sizeof(token_t), 0);
    if (!tokens) {
        return NULL;
    }

    char *p = file_content;

    while (p != NULL) {
        // Skip whitespace
        while (is_whitespace(*p) || *p == '\n') {
            p++;
        }

        if (*p == '\0') {
            // EOF
            token_t t = {.tag = TOK_EOF};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            if (*p == '\n') {
                p++;
            } else {
                p = NULL;
            }
            continue;
        }

        if (strncmp(p, "let", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_LET};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            continue;
        }

        if (is_digit(*p)) {
            int64_t val = 0;
            while (is_digit(*p)) {
                val = val * 10 + (*p - '0');
                p++;
            }
            token_t t = {.tag = TOK_INT, .value.ival = val};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            continue;
        }

        if (isalpha(*p) || *p == '_') {
            char *start = p;
            while (isalnum(*p) || *p == '_') {
                p++;
            }
            size_t len = p - start;
            char *ident = malloc(len + 1);
            memcpy(ident, start, len);
            ident[len] = '\0';

            token_t t = {.tag = TOK_IDENTIFIER, .value.identifier = ident};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            continue;
        }

        if (*p == '=') {
            token_t t = {.tag = TOK_EQ};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == '+') {
            token_t t = {.tag = TOK_PLUS};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == '-') {
            token_t t = {.tag = TOK_MINUS};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == '*') {
            token_t t = {.tag = TOK_MUL};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == '/') {
            token_t t = {.tag = TOK_DIV};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }

        // Unknown character → error (for now just skip or handle later)
        fprintf(stderr, "Unknown character: '%c'\n", *p);
        return NULL;
    }

    // Add final EOF if not already present
    if (ACCESS_ARR_AT(token_t, tokens, tokens->len - 1)->tag != TOK_EOF) {
        token_t eof = {.tag = TOK_EOF};
        clape_arr_append(sizeof(token_t), &tokens, &eof);
    }

    return tokens;
}

void clape_print_token(FILE *const stream, token_t *const token) {
    switch (token->tag) {
        case TOK_EQ:
            fprintf(stream, "=");
            break;
        case TOK_PLUS:
            fprintf(stream, "+");
            break;
        case TOK_MINUS:
            fprintf(stream, "-");
            break;
        case TOK_MUL:
            fprintf(stream, "*");
            break;
        case TOK_DIV:
            fprintf(stream, "/");
            break;
        case TOK_LET:
            fprintf(stream, "let");
            break;
        case TOK_INT:
            fprintf(stream, "%li", token->value.ival);
            break;
        case TOK_IDENTIFIER:
            fprintf(stream, "%s", token->value.identifier);
            break;
        case TOK_EOF:
            fprintf(stream, "EOF");
            break;
    }
}

void clape_free_tokens(clape_arr_t *const tokens) {
    for (size_t i = 0; i < tokens->len; i++) {
        token_t *tok = ACCESS_ARR_AT(token_t, tokens, i);
        switch (tok->tag) {
            case TOK_EQ:
            case TOK_PLUS:
            case TOK_MINUS:
            case TOK_MUL:
            case TOK_DIV:
            case TOK_LET:
            case TOK_INT:
                break;
            case TOK_IDENTIFIER:
                free(tok->value.identifier);
                break;
            case TOK_EOF:
                break;
        }
    }
    free(tokens);
}

#endif

// ------ PARSING ------

/// @function `clape_parse`
/// @brief Parses the given list of tokens down to a Clape program which then can be executed later
///
/// @param `program` The program to parse into (out param)
/// @param `tokens` The list of tokens to parse
/// @return `bool` The whether the program could be parsed successfully
[[nodiscard]] bool clape_parse(clape_program_t *const program, clape_arr_t *const tokens);

/// @function `clape_print_program`
/// @brief Prints the given program
///
/// @param `program` The program to print
void clape_print_program(clape_program_t *const program);

/// @function `clape_free_program`
/// @brief Frees the entire Clape program passed to the function
///
/// @param `program` The program to free
void clape_free_program(clape_program_t *const program);

#ifdef CLAPE_IMPLEMENTATION

bool clape_parse(clape_program_t *const program, clape_arr_t *const tokens) {
    program->statements = clape_arr_create(sizeof(clape_stmt_t), 0);
    for (size_t i = 0; i < tokens->len; i++) {
        token_t *const token = ACCESS_ARR_AT(token_t, tokens, i);
        if (token->tag != TOK_LET) {
            fprintf(stderr, "Error: Statements must begin with a 'let' keyword\n");
            fprintf(stderr, "- Got the '");
            clape_print_token(stderr, token);
            fprintf(stderr, "' token instead\n");
            return false;
        }
        token_t *const ident = token + 1;
        if (ident->tag != TOK_IDENTIFIER) {
            fprintf(stderr, "Error: Missing name of statement\n");
            return false;
        }
        if ((ident + 1)->tag != TOK_EQ) {
            fprintf(stderr, "Error: Expected token '=' but got'");
            clape_print_token(stderr, token);
            fprintf(stderr, "'\n");
            return false;
        }
        // Consume all tokens until the next `let` or `EOF` token, all tokens in between are
        // considered the expression tokens
        token_t *start = token + 3;
        token_t *end = start;
        while (end->tag != TOK_LET && end->tag != TOK_EOF) {
            end++;
        }
        // Okay so all tokens from start->end now are the expression tokens...
        while (start != end) {
            printf("expr_token = ");
            clape_print_token(stdout, start);
            printf("\n");
            start++;
        }
        printf("\n");

        i += end - token - 1;
        if (end->tag == TOK_EOF) {
            break;
        }
    }
    return true;
}

void clape_print_program(clape_program_t *const program) {
    (void)program;
}

void clape_free_program(clape_program_t *const program) {
    if (program->statements == NULL) {
        return;
    }
    for (size_t i = 0; i < program->statements->len; i++) {
        clape_stmt_t *const stmt = ACCESS_ARR_AT(clape_stmt_t, program->statements, i);
        (void)stmt;
    }
}

#endif
