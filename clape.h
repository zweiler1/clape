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

/// @enum `clape_type_e`
/// @brief The enum describing the type of a clape value
typedef enum {
    CLAPE_TYPE_INT,
    CLAPE_TYPE_FLOAT,
    CLAPE_TYPE_BOOL,
    CLAPE_TYPE_FUNC,
    CLAPE_TYPE_UNIT,
} clape_type_e;

/// @struct `clape_type_t`
/// @brief A richer type descriptor, e.g. function signatures
typedef struct clape_type_t {
    /// @var `tag`
    /// @brief The tag telling us which type this is
    clape_type_e tag;

    /// @var `value`
    /// @brief A union of all possible types containing a payload
    union {
        /// @variation `func`
        /// @brief A Function type containing a parameter and return type
        struct {
            struct clape_type_t *param;
            struct clape_type_t *ret;
        } func;
    } value;
} clape_type_t;

/// @struct `clape_param_t`
/// @brief A single parameter declaration with a name and type
typedef struct clape_param_t {
    /// @var `name`
    /// @brief The name of the function parameter
    char *name;

    /// @var `type`
    /// @brief The type of the function parameter
    clape_type_t type;
} clape_param_t;

/// Forward declaration for self-referencing function pointer
typedef struct clape_value_t clape_value_t;
typedef clape_value_t (*clape_builtin_fn)(clape_value_t);

/// @struct `clape_value_t`
/// @brief Represents a single Clape value
struct clape_value_t {
    /// @var `type`
    /// @brief The type of this Clape value
    clape_type_t type;

    /// @var `value`
    /// @brief A union of all possible value types
    union {
        /// @variation `ival`
        /// @brief An `Int` literal value
        int64_t ival;

        /// @variation `fval`
        /// @brief A `Float` literal value
        double fval;

        /// @variation `bval`
        /// @brief A `Bool` literal value
        bool bval;

        /// @variation `fn`
        /// @brief A function descriptor (builtin or user-defined)
        struct clape_fn_t *fn;
    } value;
};

/// @enum `clape_binop_e`
/// @brief The enum for binary operator types
typedef enum {
    CLAPE_BINOP_ADD,
    CLAPE_BINOP_SUB,
    CLAPE_BINOP_MUL,
    CLAPE_BINOP_DIV,
    CLAPE_BINOP_LT,
    CLAPE_BINOP_GT,
    CLAPE_BINOP_LE,
    CLAPE_BINOP_GE,
    CLAPE_BINOP_EQ,
    CLAPE_BINOP_NE,
    CLAPE_BINOP_AND,
    CLAPE_BINOP_OR,
} clape_binop_e;

/// @enum `clape_unop_e`
/// @brief The enum for unary operator types
typedef enum {
    CLAPE_UNOP_NOT,
} clape_unop_e;

/// @enum `clape_expr_e`
/// @brief The enum to tag the clape expr with
typedef enum {
    CLAPE_EXPR_LIT,
    CLAPE_EXPR_IDENT,
    CLAPE_EXPR_UNARY,
    CLAPE_EXPR_BINOP,
    CLAPE_EXPR_CALL,
    CLAPE_EXPR_LAMBDA,
    CLAPE_EXPR_BLOCK,
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

        /// @variation `ident`
        /// @brief An identifier reference
        char *ident;

        /// @variation `unary`
        /// @brief A unary operation expression
        struct {
            clape_unop_e op;
            struct clape_expr_t *operand;
        } unary;

        /// @variation `binop`
        /// @brief A binary operation expression
        struct {
            clape_binop_e op;
            struct clape_expr_t *lhs;
            struct clape_expr_t *rhs;
        } binop;

        /// @variation `call`
        /// @brief A function call expression via juxtaposition
        struct {
            struct clape_expr_t *callee;
            struct clape_expr_t *arg;
        } call;

        /// @variation `lambda`
        /// @brief A lambda expression: (params) -> RetType { body }
        struct {
            /// @var `params`
            /// @brief An array of all paramters, where every value of the array is of type
            /// `clape_param_t`
            clape_arr_t *params;
            clape_type_t return_type;
            struct clape_expr_t *body;
        } lambda;

        /// @variation `block`
        /// @brief A block expression: { stmts; return_expr }
        struct {
            clape_arr_t *stmts;
            struct clape_expr_t *return_expr;
        } block;
    } value;
} clape_expr_t;

/// @enum `clape_stmt_e`
/// @brief The enum to tag the clape statement with
typedef enum {
    CLAPE_STMT_LET,
    CLAPE_STMT_USE,
} clape_stmt_e;

/// @struct `clape_stmt_t`
/// @brief A clape statement is either a `let xyz = ...` binding or a `use Module` import
typedef struct clape_stmt_t {
    /// @var `tag`
    /// @brief The tag telling us which type of statement this is
    clape_stmt_e tag;

    /// @var `value`
    /// @brief A union of all possible statement types
    union {
        /// @variation `let`
        /// @brief A let-binding statement
        struct {
            char *name;
            clape_expr_t expr;
        } let;

        /// @variation `use`
        /// @brief A module import statement
        struct {
            char *module;
        } use;
    } value;
} clape_stmt_t;

/// @struct `clape_program_t`
/// @brief A clape program is a list of statements to evaluate in that exact order
typedef struct clape_program_t {
    /// @var `statements`
    /// @brief A dynamic array containing all parsed statements, every value of the dynamic array is
    /// of type `clape_stmt_t`
    clape_arr_t *statements;
} clape_program_t;

/// @struct `clape_env_t`
/// @brief A linked-list node representing a single binding in the runtime environment
typedef struct clape_env_t {
    /// @var `name`
    /// @brief The bound variable name
    char *name;

    /// @var `value`
    /// @brief The bound value
    clape_value_t value;

    /// @var `next`
    /// @brief A pointer to the next node in the environment chain
    struct clape_env_t *next;
} clape_env_t;

/// @struct `clape_fn_t`
/// @brief A function descriptor holding either a builtin or a user-defined function including
/// partial application state
typedef struct clape_fn_t {
    /// @var `is_builtin`
    /// @brief Whether this function is a builtin function, like the `print` function(s)
    bool is_builtin;

    /// @var `params`
    /// @brief An array of all parameters, where every element is of type `clape_param_t`
    clape_arr_t *params;

    /// @var `return_type`
    /// @brief The return type of the function
    clape_type_t return_type;

    /// @var `body`
    /// @brief The body expression of the function
    struct clape_expr_t *body;

    /// @var `builtin_fn`
    /// @brief The builtin function to call if this function is a builtin function
    clape_builtin_fn builtin_fn;

    /// @var `next_param_index`
    /// @brief The index of the next parameter, for partially applied calls
    size_t next_param_index;

    /// @var `closure`
    /// @brief The captured environment of this defined function, for closures
    struct clape_env_t *closure;
} clape_fn_t;

/// @enum `token_type_e`
/// @brief The enum of all possible Clape token types
typedef enum : uint8_t {
    // Symbols
    TOK_PLUS = 0,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_EQ,
    TOK_EQ_EQ,
    TOK_NE,
    TOK_LT,
    TOK_LE,
    TOK_GT,
    TOK_GE,
    // Keywords
    TOK_LET,
    TOK_USE,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    // Literals
    TOK_TRUE,
    TOK_FALSE,
    TOK_INT,
    TOK_FLOAT,
    // Other
    TOK_IDENTIFIER,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_COLON,
    TOK_ARROW,
    TOK_COMMA,
    TOK_SEMICOLON,
    // Type keywords
    TOK_TYPE_INT,
    TOK_TYPE_FLOAT,
    TOK_TYPE_BOOL,
    TOK_TYPE_UNIT,
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

        /// @variation `fval`
        /// @brief A `Float` literal value
        double fval;

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

        if (strncmp(p, "use", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_USE};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            continue;
        }

        if (is_digit(*p)) {
            int64_t ival = 0;
            while (is_digit(*p)) {
                ival = ival * 10 + (*p - '0');
                p++;
            }
            if (*p == '.' && is_digit(*(p + 1))) {
                p++;
                double fval = (double)ival;
                double frac = 0.1;
                while (is_digit(*p)) {
                    fval += frac * (*p - '0');
                    frac *= 0.1;
                    p++;
                }
                token_t t = {.tag = TOK_FLOAT, .value.fval = fval};
                clape_arr_append(sizeof(token_t), &tokens, &t);
            } else {
                token_t t = {.tag = TOK_INT, .value.ival = ival};
                clape_arr_append(sizeof(token_t), &tokens, &t);
            }
            continue;
        }

        if (strncmp(p, "true", 4) == 0 && !isalnum(p[4])) {
            token_t t = {.tag = TOK_TRUE};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 4;
            continue;
        }

        if (strncmp(p, "false", 5) == 0 && !isalnum(p[5])) {
            token_t t = {.tag = TOK_FALSE};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 5;
            continue;
        }

        if (strncmp(p, "and", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_AND};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            continue;
        }

        if (strncmp(p, "or", 2) == 0 && !isalnum(p[2])) {
            token_t t = {.tag = TOK_OR};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 2;
            continue;
        }

        if (strncmp(p, "not", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_NOT};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            continue;
        }

        if (strncmp(p, "Int", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_TYPE_INT};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            continue;
        }
        if (strncmp(p, "Float", 5) == 0 && !isalnum(p[5])) {
            token_t t = {.tag = TOK_TYPE_FLOAT};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 5;
            continue;
        }
        if (strncmp(p, "Bool", 4) == 0 && !isalnum(p[4])) {
            token_t t = {.tag = TOK_TYPE_BOOL};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 4;
            continue;
        }
        if (strncmp(p, "Unit", 4) == 0 && !isalnum(p[4])) {
            token_t t = {.tag = TOK_TYPE_UNIT};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 4;
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
            if (*(p + 1) == '=') {
                token_t t = {.tag = TOK_EQ_EQ};
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
            } else {
                token_t t = {.tag = TOK_EQ};
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p++;
            }
            continue;
        }
        if (*p == '<') {
            if (*(p + 1) == '=') {
                token_t t = {.tag = TOK_LE};
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
            } else {
                token_t t = {.tag = TOK_LT};
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p++;
            }
            continue;
        }
        if (*p == '>') {
            if (*(p + 1) == '=') {
                token_t t = {.tag = TOK_GE};
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
            } else {
                token_t t = {.tag = TOK_GT};
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p++;
            }
            continue;
        }
        if (*p == '!') {
            if (*(p + 1) == '=') {
                token_t t = {.tag = TOK_NE};
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
            } else {
                fprintf(stderr, "Expected '=' after '!'\n");
                return NULL;
            }
            continue;
        }
        if (*p == '+') {
            token_t t = {.tag = TOK_PLUS};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == '-') {
            token_t t = {.tag = *(p + 1) == '>' ? TOK_ARROW : TOK_MINUS};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += t.tag == TOK_ARROW ? 2 : 1;
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
        if (*p == '(') {
            token_t t = {.tag = TOK_LPAREN};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == ')') {
            token_t t = {.tag = TOK_RPAREN};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == '{') {
            token_t t = {.tag = TOK_LBRACE};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == '}') {
            token_t t = {.tag = TOK_RBRACE};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == '#') {
            // Single-line comment: skip until \n or \0
            while (*p && *p != '\n') {
                p++;
            }
            continue;
        }

        if (*p == ':') {
            token_t t = {.tag = TOK_COLON};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == ',') {
            token_t t = {.tag = TOK_COMMA};
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            continue;
        }
        if (*p == ';') {
            token_t t = {.tag = TOK_SEMICOLON};
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
        case TOK_EQ:
            fprintf(stream, "=");
            break;
        case TOK_EQ_EQ:
            fprintf(stream, "==");
            break;
        case TOK_NE:
            fprintf(stream, "!=");
            break;
        case TOK_LT:
            fprintf(stream, "<");
            break;
        case TOK_LE:
            fprintf(stream, "<=");
            break;
        case TOK_GT:
            fprintf(stream, ">");
            break;
        case TOK_GE:
            fprintf(stream, ">=");
            break;
        case TOK_LET:
            fprintf(stream, "let");
            break;
        case TOK_USE:
            fprintf(stream, "use");
            break;
        case TOK_AND:
            fprintf(stream, "and");
            break;
        case TOK_OR:
            fprintf(stream, "or");
            break;
        case TOK_NOT:
            fprintf(stream, "not");
            break;
        case TOK_TRUE:
            fprintf(stream, "true");
            break;
        case TOK_FALSE:
            fprintf(stream, "false");
            break;
        case TOK_INT:
            fprintf(stream, "%li", token->value.ival);
            break;
        case TOK_FLOAT:
            fprintf(stream, "%g", token->value.fval);
            break;
        case TOK_LPAREN:
            fprintf(stream, "(");
            break;
        case TOK_RPAREN:
            fprintf(stream, ")");
            break;
        case TOK_LBRACE:
            fprintf(stream, "{");
            break;
        case TOK_RBRACE:
            fprintf(stream, "}");
            break;
        case TOK_COLON:
            fprintf(stream, ":");
            break;
        case TOK_ARROW:
            fprintf(stream, "->");
            break;
        case TOK_COMMA:
            fprintf(stream, ",");
            break;
        case TOK_SEMICOLON:
            fprintf(stream, ";");
            break;
        case TOK_TYPE_INT:
            fprintf(stream, "Int");
            break;
        case TOK_TYPE_FLOAT:
            fprintf(stream, "Float");
            break;
        case TOK_TYPE_BOOL:
            fprintf(stream, "Bool");
            break;
        case TOK_TYPE_UNIT:
            fprintf(stream, "Unit");
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
            case TOK_PLUS:
            case TOK_MINUS:
            case TOK_MUL:
            case TOK_DIV:
            case TOK_EQ:
            case TOK_EQ_EQ:
            case TOK_NE:
            case TOK_LT:
            case TOK_LE:
            case TOK_GT:
            case TOK_GE:
            case TOK_LET:
            case TOK_USE:
            case TOK_AND:
            case TOK_OR:
            case TOK_NOT:
            case TOK_TRUE:
            case TOK_FALSE:
            case TOK_INT:
            case TOK_FLOAT:
            case TOK_LPAREN:
            case TOK_RPAREN:
            case TOK_LBRACE:
            case TOK_RBRACE:
            case TOK_COLON:
            case TOK_ARROW:
            case TOK_COMMA:
            case TOK_SEMICOLON:
            case TOK_TYPE_INT:
            case TOK_TYPE_FLOAT:
            case TOK_TYPE_BOOL:
            case TOK_TYPE_UNIT:
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

// Forward-declarations
char *strdup(const char *s);

// ----- Parser helpers -----

typedef enum {
    CLAPE_BINDING_POWER_DEFAULT,
    CLAPE_BINDING_POWER_LOGICAL,
    CLAPE_BINDING_POWER_RELATIONAL,
    CLAPE_BINDING_POWER_TERM,
    CLAPE_BINDING_POWER_FACTOR,
    CLAPE_BINDING_POWER_CALL,
} clape_binding_power_t;

typedef struct {
    clape_arr_t *tokens;
    size_t pos;
} clape_parser_t;

static token_t *clape_peek(clape_parser_t *p) {
    return ACCESS_ARR_AT(token_t, p->tokens, p->pos);
}

static token_t *clape_advance(clape_parser_t *p) {
    return ACCESS_ARR_AT(token_t, p->tokens, p->pos++);
}

static clape_stmt_t clape_parse_stmt(clape_parser_t *p);
static clape_expr_t clape_parse_expr(clape_parser_t *p, clape_binding_power_t min_bp);
static clape_expr_t clape_parse_block(clape_parser_t *p);

static clape_type_t clape_parse_type(clape_parser_t *p) {
    token_t *tok = clape_peek(p);
    if (tok->tag == TOK_LPAREN) {
        clape_advance(p);
        clape_type_t param = clape_parse_type(p);
        tok = clape_advance(p);
        if (tok->tag != TOK_ARROW) {
            fprintf(stderr, "Expected '->' in function type\n");
            exit(1);
        }
        clape_type_t ret = clape_parse_type(p);
        tok = clape_advance(p);
        if (tok->tag != TOK_RPAREN) {
            fprintf(stderr, "Expected ')' in function type\n");
            exit(1);
        }
        clape_type_t *param_ptr = malloc(sizeof(clape_type_t));
        *param_ptr = param;
        clape_type_t *ret_ptr = malloc(sizeof(clape_type_t));
        *ret_ptr = ret;
        return (clape_type_t){
            .tag = CLAPE_TYPE_FUNC,
            .value.func = {.param = param_ptr, .ret = ret_ptr},
        };
    }
    tok = clape_advance(p);
    switch (tok->tag) {
        case TOK_TYPE_INT:
            return (clape_type_t){.tag = CLAPE_TYPE_INT};
        case TOK_TYPE_FLOAT:
            return (clape_type_t){.tag = CLAPE_TYPE_FLOAT};
        case TOK_TYPE_BOOL:
            return (clape_type_t){.tag = CLAPE_TYPE_BOOL};
        case TOK_TYPE_UNIT:
            return (clape_type_t){.tag = CLAPE_TYPE_UNIT};
        default:
            fprintf(stderr, "Expected a type (Int, Float, Bool, Unit)\n");
            exit(1);
    }
}

static clape_binding_power_t clape_infix_bp(token_type_e tag) {
    switch (tag) {
        case TOK_PLUS:
        case TOK_MINUS:
            return CLAPE_BINDING_POWER_TERM;
        case TOK_MUL:
        case TOK_DIV:
            return CLAPE_BINDING_POWER_FACTOR;
        case TOK_AND:
        case TOK_OR:
            return CLAPE_BINDING_POWER_LOGICAL;
        case TOK_EQ_EQ:
        case TOK_NE:
        case TOK_LT:
        case TOK_LE:
        case TOK_GT:
        case TOK_GE:
            return CLAPE_BINDING_POWER_RELATIONAL;
        default:
            return CLAPE_BINDING_POWER_DEFAULT;
    }
}

static clape_expr_t clape_parse_expr(clape_parser_t *p, clape_binding_power_t min_bp) {
    token_t *tok = clape_advance(p);
    clape_expr_t lhs;

    switch (tok->tag) {
        case TOK_INT:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .value.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_INT},
                        .value.ival = tok->value.ival,
                    },
            };
            break;
        case TOK_FLOAT:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .value.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_FLOAT},
                        .value.fval = tok->value.fval,
                    },
            };
            break;
        case TOK_TRUE:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .value.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .value.bval = true,
                    },
            };
            break;
        case TOK_FALSE:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .value.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .value.bval = false,
                    },
            };
            break;
        case TOK_NOT: {
            clape_expr_t *op = malloc(sizeof(clape_expr_t));
            *op = clape_parse_expr(p, CLAPE_BINDING_POWER_CALL);
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_UNARY,
                .value.unary = {.op = CLAPE_UNOP_NOT, .operand = op},
            };
            break;
        }
        case TOK_LPAREN: {
            token_t *next = clape_peek(p);
            bool is_lambda = false;
            if (next->tag == TOK_RPAREN) {
                if (p->pos + 1 < p->tokens->len) {
                    is_lambda = (ACCESS_ARR_AT(token_t, p->tokens, p->pos + 1)->tag == TOK_ARROW);
                }
            } else if (next->tag == TOK_IDENTIFIER) {
                if (p->pos + 1 < p->tokens->len) {
                    is_lambda = (ACCESS_ARR_AT(token_t, p->tokens, p->pos + 1)->tag == TOK_COLON);
                }
            }
            if (!is_lambda) {
                lhs = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);
                tok = clape_advance(p);
                if (tok->tag != TOK_RPAREN) {
                    fprintf(stderr, "Expected ')'\n");
                    exit(1);
                }
            }

            clape_arr_t *params = clape_arr_create(sizeof(clape_param_t), 0);
            if (next->tag != TOK_RPAREN) {
                while (true) {
                    token_t *name_tok = clape_advance(p);
                    clape_advance(p);
                    clape_type_t param_type = clape_parse_type(p);
                    clape_param_t param = {
                        .name = strdup(name_tok->value.identifier),
                        .type = param_type,
                    };
                    clape_arr_append(sizeof(clape_param_t), &params, &param);
                    if (clape_peek(p)->tag == TOK_COMMA) {
                        clape_advance(p);
                    } else {
                        break;
                    }
                }
            }
            tok = clape_advance(p);
            if (tok->tag != TOK_RPAREN) {
                fprintf(stderr, "Expected ')' in lambda parameter list\n");
                exit(1);
            }
            tok = clape_advance(p);
            if (tok->tag != TOK_ARROW) {
                fprintf(stderr, "Expected '->' after lambda parameters\n");
                exit(1);
            }
            clape_type_t return_type = clape_parse_type(p);
            clape_expr_t body = clape_parse_block(p);
            clape_expr_t *body_ptr = malloc(sizeof(clape_expr_t));
            *body_ptr = body;
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LAMBDA,
                .value.lambda = {.params = params, .return_type = return_type, .body = body_ptr},
            };
            break;
        }
        case TOK_LBRACE:
            lhs = clape_parse_block(p);
            break;
        case TOK_IDENTIFIER:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_IDENT,
                .value.ident = strdup(tok->value.identifier),
            };
            break;
        default:
            fprintf(stderr, "Unexpected token in expression: ");
            clape_print_token(stderr, tok);
            fprintf(stderr, "\n");
            exit(1);
    }

    while (true) {
        token_type_e next = clape_peek(p)->tag;
        clape_binding_power_t cur_bp = clape_infix_bp(next);

        if (cur_bp > min_bp) {
            token_t *op_tok = clape_advance(p);
            clape_binop_e op;
            switch (op_tok->tag) {
                case TOK_PLUS:
                    op = CLAPE_BINOP_ADD;
                    cur_bp = CLAPE_BINDING_POWER_TERM;
                    break;
                case TOK_MINUS:
                    op = CLAPE_BINOP_SUB;
                    cur_bp = CLAPE_BINDING_POWER_TERM;
                    break;
                case TOK_MUL:
                    op = CLAPE_BINOP_MUL;
                    cur_bp = CLAPE_BINDING_POWER_FACTOR;
                    break;
                case TOK_DIV:
                    op = CLAPE_BINOP_DIV;
                    cur_bp = CLAPE_BINDING_POWER_FACTOR;
                    break;
                case TOK_EQ_EQ:
                    op = CLAPE_BINOP_EQ;
                    cur_bp = CLAPE_BINDING_POWER_RELATIONAL;
                    break;
                case TOK_NE:
                    op = CLAPE_BINOP_NE;
                    cur_bp = CLAPE_BINDING_POWER_RELATIONAL;
                    break;
                case TOK_LT:
                    op = CLAPE_BINOP_LT;
                    cur_bp = CLAPE_BINDING_POWER_RELATIONAL;
                    break;
                case TOK_LE:
                    op = CLAPE_BINOP_LE;
                    cur_bp = CLAPE_BINDING_POWER_RELATIONAL;
                    break;
                case TOK_GT:
                    op = CLAPE_BINOP_GT;
                    cur_bp = CLAPE_BINDING_POWER_RELATIONAL;
                    break;
                case TOK_GE:
                    op = CLAPE_BINOP_GE;
                    cur_bp = CLAPE_BINDING_POWER_RELATIONAL;
                    break;
                case TOK_AND:
                    op = CLAPE_BINOP_AND;
                    cur_bp = CLAPE_BINDING_POWER_LOGICAL;
                    break;
                case TOK_OR:
                    op = CLAPE_BINOP_OR;
                    cur_bp = CLAPE_BINDING_POWER_LOGICAL;
                    break;
                default:
                    fprintf(stderr, "Unexpected infix operator\n");
                    exit(1);
            }
            clape_expr_t rhs = clape_parse_expr(p, cur_bp);
            clape_expr_t *lhs_ptr = malloc(sizeof(clape_expr_t));
            *lhs_ptr = lhs;
            clape_expr_t *rhs_ptr = malloc(sizeof(clape_expr_t));
            *rhs_ptr = rhs;
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_BINOP,
                .value.binop = {.op = op, .lhs = lhs_ptr, .rhs = rhs_ptr},
            };
        } else if ((next == TOK_INT || next == TOK_FLOAT || next == TOK_TRUE || next == TOK_FALSE ||
                       next == TOK_NOT || next == TOK_LPAREN || next == TOK_LBRACE ||
                       next == TOK_IDENTIFIER) &&
            CLAPE_BINDING_POWER_CALL > min_bp) {
            cur_bp = CLAPE_BINDING_POWER_CALL;
            clape_expr_t arg = clape_parse_expr(p, cur_bp);
            clape_expr_t *callee_ptr = malloc(sizeof(clape_expr_t));
            *callee_ptr = lhs;
            clape_expr_t *arg_ptr = malloc(sizeof(clape_expr_t));
            *arg_ptr = arg;
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_CALL,
                .value.call = {.callee = callee_ptr, .arg = arg_ptr},
            };
        } else {
            break;
        }
    }
    return lhs;
}

static clape_stmt_t clape_parse_stmt(clape_parser_t *p) {
    token_t *first = clape_advance(p);
    if (first->tag == TOK_USE) {
        token_t *mod_tok = clape_advance(p);
        return (clape_stmt_t){
            .tag = CLAPE_STMT_USE,
            .value.use = {.module = strdup(mod_tok->value.identifier)},
        };
    }
    token_t *name_tok = clape_advance(p);
    clape_advance(p);
    clape_stmt_t stmt = {
        .tag = CLAPE_STMT_LET,
        .value.let =
            {
                .name = strdup(name_tok->value.identifier),
                .expr = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT),
            },
    };
    return stmt;
}

static clape_expr_t clape_parse_block(clape_parser_t *p) {
    token_t *tok = clape_advance(p);
    if (tok->tag != TOK_LBRACE) {
        fprintf(stderr, "Expected '{'\n");
        exit(1);
    }
    clape_arr_t *stmts = clape_arr_create(sizeof(clape_stmt_t), 0);
    while (clape_peek(p)->tag == TOK_LET) {
        clape_stmt_t stmt = clape_parse_stmt(p);
        clape_arr_append(sizeof(clape_stmt_t), &stmts, &stmt);
        while (clape_peek(p)->tag == TOK_SEMICOLON) {
            clape_advance(p);
        }
    }
    clape_expr_t *ret_expr = NULL;
    if (clape_peek(p)->tag != TOK_RBRACE) {
        ret_expr = malloc(sizeof(clape_expr_t));
        *ret_expr = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);
    }
    tok = clape_advance(p);
    if (tok->tag != TOK_RBRACE) {
        fprintf(stderr, "Expected '}'\n");
        exit(1);
    }
    return (clape_expr_t){
        .tag = CLAPE_EXPR_BLOCK,
        .value.block = {.stmts = stmts, .return_expr = ret_expr},
    };
}

// ----- Public API -----

bool clape_parse(clape_program_t *const program, clape_arr_t *const tokens) {
    clape_parser_t p = {.tokens = tokens, .pos = 0};
    program->statements = clape_arr_create(sizeof(clape_stmt_t), 0);
    while (clape_peek(&p)->tag != TOK_EOF) {
        token_type_e t = clape_peek(&p)->tag;
        if (t != TOK_LET && t != TOK_USE) {
            fprintf(stderr, "Expected 'let' or 'use' at start of statement\n");
            return false;
        }
        clape_stmt_t stmt = clape_parse_stmt(&p);
        clape_arr_append(sizeof(clape_stmt_t), &program->statements, &stmt);
        while (clape_peek(&p)->tag == TOK_SEMICOLON) {
            clape_advance(&p);
        }
    }
    return true;
}

static void clape_print_expr(FILE *stream, clape_expr_t *expr) {
    switch (expr->tag) {
        case CLAPE_EXPR_LIT:
            switch (expr->value.lit.type.tag) {
                case CLAPE_TYPE_INT:
                    fprintf(stream, "%li", expr->value.lit.value.ival);
                    break;
                case CLAPE_TYPE_FLOAT:
                    fprintf(stream, "%g", expr->value.lit.value.fval);
                    break;
                case CLAPE_TYPE_BOOL:
                    fprintf(stream, "%s", expr->value.lit.value.bval ? "true" : "false");
                    break;
                default:
                    fprintf(stream, "<value>");
                    break;
            }
            break;
        case CLAPE_EXPR_UNARY:
            fprintf(stream, "(not ");
            clape_print_expr(stream, expr->value.unary.operand);
            fprintf(stream, ")");
            break;
        case CLAPE_EXPR_IDENT:
            fprintf(stream, "%s", expr->value.ident);
            break;
        case CLAPE_EXPR_BINOP: {
            const char *op_str;
            switch (expr->value.binop.op) {
                case CLAPE_BINOP_ADD:
                    op_str = "+";
                    break;
                case CLAPE_BINOP_SUB:
                    op_str = "-";
                    break;
                case CLAPE_BINOP_MUL:
                    op_str = "*";
                    break;
                case CLAPE_BINOP_DIV:
                    op_str = "/";
                    break;
                case CLAPE_BINOP_EQ:
                    op_str = "==";
                    break;
                case CLAPE_BINOP_NE:
                    op_str = "!=";
                    break;
                case CLAPE_BINOP_LT:
                    op_str = "<";
                    break;
                case CLAPE_BINOP_LE:
                    op_str = "<=";
                    break;
                case CLAPE_BINOP_GT:
                    op_str = ">";
                    break;
                case CLAPE_BINOP_GE:
                    op_str = ">=";
                    break;
                case CLAPE_BINOP_AND:
                    op_str = "and";
                    break;
                case CLAPE_BINOP_OR:
                    op_str = "or";
                    break;
            }
            fprintf(stream, "(");
            clape_print_expr(stream, expr->value.binop.lhs);
            fprintf(stream, " %s ", op_str);
            clape_print_expr(stream, expr->value.binop.rhs);
            fprintf(stream, ")");
            break;
        }
        case CLAPE_EXPR_CALL:
            fprintf(stream, "(");
            clape_print_expr(stream, expr->value.call.callee);
            fprintf(stream, " ");
            clape_print_expr(stream, expr->value.call.arg);
            fprintf(stream, ")");
            break;
        case CLAPE_EXPR_LAMBDA:
            fprintf(stream, "<lambda>");
            break;
        case CLAPE_EXPR_BLOCK:
            fprintf(stream, "<block>");
            break;
    }
}

void clape_print_program(clape_program_t *const program) {
    for (size_t i = 0; i < program->statements->len; i++) {
        clape_stmt_t *stmt = ACCESS_ARR_AT(clape_stmt_t, program->statements, i);
        if (stmt->tag == CLAPE_STMT_LET) {
            fprintf(stdout, "(let %s = ", stmt->value.let.name);
            clape_print_expr(stdout, &stmt->value.let.expr);
            fprintf(stdout, ")\n");
        } else {
            fprintf(stdout, "(use %s)\n", stmt->value.use.module);
        }
    }
}

static void clape_free_expr(clape_expr_t *expr) {
    switch (expr->tag) {
        case CLAPE_EXPR_LIT:
            break;
        case CLAPE_EXPR_IDENT:
            free(expr->value.ident);
            break;
        case CLAPE_EXPR_UNARY:
            clape_free_expr(expr->value.unary.operand);
            free(expr->value.unary.operand);
            break;
        case CLAPE_EXPR_BINOP:
            clape_free_expr(expr->value.binop.lhs);
            clape_free_expr(expr->value.binop.rhs);
            free(expr->value.binop.lhs);
            free(expr->value.binop.rhs);
            break;
        case CLAPE_EXPR_CALL:
            clape_free_expr(expr->value.call.callee);
            clape_free_expr(expr->value.call.arg);
            free(expr->value.call.callee);
            free(expr->value.call.arg);
            break;
        case CLAPE_EXPR_LAMBDA:
            for (size_t i = 0; i < expr->value.lambda.params->len; i++) {
                free(ACCESS_ARR_AT(clape_param_t, expr->value.lambda.params, i)->name);
            }
            free(expr->value.lambda.params);
            clape_free_expr(expr->value.lambda.body);
            free(expr->value.lambda.body);
            break;
        case CLAPE_EXPR_BLOCK:
            for (size_t i = 0; i < expr->value.block.stmts->len; i++) {
                clape_stmt_t *s = ACCESS_ARR_AT(clape_stmt_t, expr->value.block.stmts, i);
                free(s->value.let.name);
                clape_free_expr(&s->value.let.expr);
            }
            free(expr->value.block.stmts);
            if (expr->value.block.return_expr) {
                clape_free_expr(expr->value.block.return_expr);
                free(expr->value.block.return_expr);
            }
            break;
    }
}

void clape_free_program(clape_program_t *const program) {
    if (!program->statements) {
        return;
    }
    for (size_t i = 0; i < program->statements->len; i++) {
        clape_stmt_t *stmt = ACCESS_ARR_AT(clape_stmt_t, program->statements, i);
        if (stmt->tag == CLAPE_STMT_LET) {
            free(stmt->value.let.name);
            clape_free_expr(&stmt->value.let.expr);
        } else {
            free(stmt->value.use.module);
        }
    }
    free(program->statements);
}

#endif

// ----- Interpreter -----

/// @function `clape_interpret`
/// @brief Interprets (executes) a parsed Clape program
///
/// @param `program` The program to interpret
void clape_interpret(clape_program_t *const program);

#ifdef CLAPE_IMPLEMENTATION

static clape_value_t clape_eval(clape_expr_t *expr, clape_env_t *env) {
    switch (expr->tag) {
        case CLAPE_EXPR_LIT:
            return expr->value.lit;
        case CLAPE_EXPR_IDENT: {
            for (clape_env_t *e = env; e; e = e->next) {
                if (strcmp(e->name, expr->value.ident) == 0) {
                    return e->value;
                }
            }
            fprintf(stderr, "Undefined variable: %s\n", expr->value.ident);
            exit(1);
        }
        case CLAPE_EXPR_UNARY: {
            clape_value_t operand = clape_eval(expr->value.unary.operand, env);
            if (operand.type.tag != CLAPE_TYPE_BOOL) {
                fprintf(stderr, "not requires a Bool operand\n");
                exit(1);
            }
            return (clape_value_t){
                .type = {.tag = CLAPE_TYPE_BOOL},
                .value.bval = !operand.value.bval,
            };
        }
        case CLAPE_EXPR_BINOP: {
            clape_value_t lhs = clape_eval(expr->value.binop.lhs, env);
            clape_value_t rhs = clape_eval(expr->value.binop.rhs, env);
            if (lhs.type.tag != rhs.type.tag) {
                fprintf(stderr, "Type mismatch in binary operation\n");
                exit(1);
            }

            clape_binop_e op = expr->value.binop.op;
            switch (op) {
                case CLAPE_BINOP_AND:
                case CLAPE_BINOP_OR:
                    if (lhs.type.tag != CLAPE_TYPE_BOOL || rhs.type.tag != CLAPE_TYPE_BOOL) {
                        fprintf(stderr, "and/or require Bool operands\n");
                        exit(1);
                    }
                    return (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .value.bval = (op == CLAPE_BINOP_AND) ? (lhs.value.bval && rhs.value.bval)
                                                              : (lhs.value.bval || rhs.value.bval),
                    };
                case CLAPE_BINOP_ADD:
                case CLAPE_BINOP_SUB:
                case CLAPE_BINOP_MUL:
                case CLAPE_BINOP_DIV:
                    if (lhs.type.tag != rhs.type.tag) {
                        fprintf(stderr, "Type mismatch in arithmetic: cannot mix types\n");
                        exit(1);
                    }
                    switch (lhs.type.tag) {
                        default:
                            fprintf(stderr, "Arithmetic requires Int or Float\n");
                            exit(1);
                        case CLAPE_TYPE_INT: {
                            int64_t r;
                            switch (op) {
                                case CLAPE_BINOP_ADD:
                                    r = lhs.value.ival + rhs.value.ival;
                                    break;
                                case CLAPE_BINOP_SUB:
                                    r = lhs.value.ival - rhs.value.ival;
                                    break;
                                case CLAPE_BINOP_MUL:
                                    r = lhs.value.ival * rhs.value.ival;
                                    break;
                                case CLAPE_BINOP_DIV:
                                    r = lhs.value.ival / rhs.value.ival;
                                    break;
                                default:
                                    exit(1);
                            }
                            return (clape_value_t){
                                .type = {.tag = CLAPE_TYPE_INT},
                                .value.ival = r,
                            };
                        }
                        case CLAPE_TYPE_FLOAT: {
                            double r;
                            switch (op) {
                                case CLAPE_BINOP_ADD:
                                    r = lhs.value.fval + rhs.value.fval;
                                    break;
                                case CLAPE_BINOP_SUB:
                                    r = lhs.value.fval - rhs.value.fval;
                                    break;
                                case CLAPE_BINOP_MUL:
                                    r = lhs.value.fval * rhs.value.fval;
                                    break;
                                case CLAPE_BINOP_DIV:
                                    r = lhs.value.fval / rhs.value.fval;
                                    break;
                                default:
                                    exit(1);
                            }
                            return (clape_value_t){
                                .type = {.tag = CLAPE_TYPE_FLOAT},
                                .value.fval = r,
                            };
                        }
                    }
                case CLAPE_BINOP_LT: {
                    bool result = false;
                    if (lhs.type.tag == CLAPE_TYPE_INT)
                        result = lhs.value.ival < rhs.value.ival;
                    else if (lhs.type.tag == CLAPE_TYPE_FLOAT)
                        result = lhs.value.fval < rhs.value.fval;
                    else {
                        fprintf(stderr, "Comparison requires Int or Float\n");
                        exit(1);
                    }
                    return (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .value.bval = result,
                    };
                }
                case CLAPE_BINOP_GT: {
                    bool result = false;
                    if (lhs.type.tag == CLAPE_TYPE_INT)
                        result = lhs.value.ival > rhs.value.ival;
                    else if (lhs.type.tag == CLAPE_TYPE_FLOAT)
                        result = lhs.value.fval > rhs.value.fval;
                    else {
                        fprintf(stderr, "Comparison requires Int or Float\n");
                        exit(1);
                    }
                    return (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .value.bval = result,
                    };
                }
                case CLAPE_BINOP_LE: {
                    bool result = false;
                    if (lhs.type.tag == CLAPE_TYPE_INT)
                        result = lhs.value.ival <= rhs.value.ival;
                    else if (lhs.type.tag == CLAPE_TYPE_FLOAT)
                        result = lhs.value.fval <= rhs.value.fval;
                    else {
                        fprintf(stderr, "Comparison requires Int or Float\n");
                        exit(1);
                    }
                    return (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .value.bval = result,
                    };
                }
                case CLAPE_BINOP_GE: {
                    bool result = false;
                    if (lhs.type.tag == CLAPE_TYPE_INT)
                        result = lhs.value.ival >= rhs.value.ival;
                    else if (lhs.type.tag == CLAPE_TYPE_FLOAT)
                        result = lhs.value.fval >= rhs.value.fval;
                    else {
                        fprintf(stderr, "Comparison requires Int or Float\n");
                        exit(1);
                    }
                    return (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .value.bval = result,
                    };
                }
                case CLAPE_BINOP_EQ: {
                    bool result = false;
                    if (lhs.type.tag == CLAPE_TYPE_INT)
                        result = lhs.value.ival == rhs.value.ival;
                    else if (lhs.type.tag == CLAPE_TYPE_FLOAT)
                        result = lhs.value.fval == rhs.value.fval;
                    else if (lhs.type.tag == CLAPE_TYPE_BOOL)
                        result = lhs.value.bval == rhs.value.bval;
                    else {
                        fprintf(stderr, "Equality not supported for this type\n");
                        exit(1);
                    }
                    return (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .value.bval = result,
                    };
                }
                case CLAPE_BINOP_NE: {
                    bool result = false;
                    if (lhs.type.tag == CLAPE_TYPE_INT)
                        result = lhs.value.ival != rhs.value.ival;
                    else if (lhs.type.tag == CLAPE_TYPE_FLOAT)
                        result = lhs.value.fval != rhs.value.fval;
                    else if (lhs.type.tag == CLAPE_TYPE_BOOL)
                        result = lhs.value.bval != rhs.value.bval;
                    else {
                        fprintf(stderr, "Inequality not supported for this type\n");
                        exit(1);
                    }
                    return (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .value.bval = result,
                    };
                }
            }
        }
        case CLAPE_EXPR_LAMBDA: {
            clape_fn_t *fn = malloc(sizeof(clape_fn_t));
            *fn = (clape_fn_t){
                .is_builtin = false,
                .params = expr->value.lambda.params,
                .return_type = expr->value.lambda.return_type,
                .body = expr->value.lambda.body,
                .builtin_fn = NULL,
                .next_param_index = 0,
                .closure = env,
            };
            return (clape_value_t){
                .type = {.tag = CLAPE_TYPE_FUNC},
                .value.fn = fn,
            };
        }
        case CLAPE_EXPR_BLOCK: {
            clape_env_t *block_env = env;
            for (size_t i = 0; i < expr->value.block.stmts->len; i++) {
                clape_stmt_t *s = ACCESS_ARR_AT(clape_stmt_t, expr->value.block.stmts, i);
                clape_value_t val = clape_eval(&s->value.let.expr, block_env);
                if (strcmp(s->value.let.name, "_") != 0) {
                    clape_env_t *binding = malloc(sizeof(clape_env_t));
                    *binding = (clape_env_t){
                        .name = strdup(s->value.let.name),
                        .value = val,
                        .next = block_env,
                    };
                    block_env = binding;
                }
            }
            if (expr->value.block.return_expr) {
                return clape_eval(expr->value.block.return_expr, block_env);
            }
            return (clape_value_t){.type = {.tag = CLAPE_TYPE_UNIT}};
        }
        case CLAPE_EXPR_CALL: {
            clape_value_t callee = clape_eval(expr->value.call.callee, env);
            clape_value_t arg = clape_eval(expr->value.call.arg, env);
            if (callee.type.tag != CLAPE_TYPE_FUNC) {
                fprintf(stderr, "Attempted to call a non-function value\n");
                exit(1);
            }
            clape_fn_t *fn = callee.value.fn;

            if (fn->is_builtin) {
                return fn->builtin_fn(arg);
            }

            size_t idx = fn->next_param_index;
            clape_param_t *param = ACCESS_ARR_AT(clape_param_t, fn->params, idx);

            clape_env_t *new_closure = malloc(sizeof(clape_env_t));
            *new_closure = (clape_env_t){
                .name = strdup(param->name),
                .value = arg,
                .next = fn->closure,
            };

            if (idx + 1 == fn->params->len) {
                return clape_eval(fn->body, new_closure);
            }
            clape_fn_t *partial = malloc(sizeof(clape_fn_t));
            *partial = (clape_fn_t){
                .is_builtin = false,
                .params = fn->params,
                .return_type = fn->return_type,
                .body = fn->body,
                .builtin_fn = NULL,
                .next_param_index = idx + 1,
                .closure = new_closure,
            };
            return (clape_value_t){
                .type = {.tag = CLAPE_TYPE_FUNC},
                .value.fn = partial,
            };
        }
    }
}

static clape_value_t clape_builtin_print(clape_value_t arg) {
    switch (arg.type.tag) {
        case CLAPE_TYPE_INT:
            printf("%li\n", arg.value.ival);
            break;
        case CLAPE_TYPE_FLOAT:
            printf("%g\n", arg.value.fval);
            break;
        case CLAPE_TYPE_BOOL:
            printf("%s\n", arg.value.bval ? "true" : "false");
            break;
        case CLAPE_TYPE_UNIT:
            printf("Unit\n");
            break;
        case CLAPE_TYPE_FUNC:
            printf("<function>\n");
            break;
    }
    return (clape_value_t){.type = {.tag = CLAPE_TYPE_UNIT}};
}

static void clape_env_free(clape_env_t *env) {
    while (env) {
        clape_env_t *next = env->next;
        free(env->name);
        if (env->value.type.tag == CLAPE_TYPE_FUNC) {
            free(env->value.value.fn);
        }
        free(env);
        env = next;
    }
}

void clape_interpret(clape_program_t *const program) {
    clape_env_t *env = NULL;

    for (size_t i = 0; i < program->statements->len; i++) {
        clape_stmt_t *stmt = ACCESS_ARR_AT(clape_stmt_t, program->statements, i);

        switch (stmt->tag) {
            case CLAPE_STMT_LET: {
                clape_value_t val = clape_eval(&stmt->value.let.expr, env);
                if (strcmp(stmt->value.let.name, "_") == 0) {
                    // Discard result for side-effect calls
                    break;
                }
                clape_env_t *binding = malloc(sizeof(clape_env_t));
                *binding = (clape_env_t){
                    .name = strdup(stmt->value.let.name),
                    .value = val,
                    .next = env,
                };
                env = binding;
                break;
            }
            case CLAPE_STMT_USE: {
                // Use statement
                if (strcmp(stmt->value.use.module, "Print") != 0) {
                    fprintf(stderr, "Unknown module: %s\n", stmt->value.use.module);
                    exit(1);
                }
                clape_fn_t *print_fn = malloc(sizeof(clape_fn_t));
                *print_fn = (clape_fn_t){
                    .is_builtin = true,
                    .params = NULL,
                    .return_type = {.tag = CLAPE_TYPE_UNIT},
                    .body = NULL,
                    .builtin_fn = clape_builtin_print,
                    .next_param_index = 0,
                    .closure = NULL,
                };
                clape_env_t *binding = malloc(sizeof(clape_env_t));
                *binding = (clape_env_t){
                    .name = strdup("print"),
                    .value =
                        (clape_value_t){
                            .type = {.tag = CLAPE_TYPE_FUNC},
                            .value.fn = print_fn,
                        },
                    .next = env,
                };
                env = binding;
                break;
            }
        }
    }

    clape_env_free(env);
}

#endif
