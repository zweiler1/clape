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

/// @function `is_valid_file_path`
/// @brief Checks whether the given file path is valid and exists
///
/// @param `path` The path to check if it's valid
/// @return `bool` Whether the given path is valid
[[nodiscard]] bool is_valid_file_path(const char *path);

/// @function `load_file`
/// @brief Loads the given file path into a newly allocated string. Callee owns the returned value
///
/// @param `path` The path to the file to load
/// @return `char *` The loaded file, callee owns it
[[nodiscard]] char *load_file(const char *path);

#ifdef CLAPE_IMPLEMENTATION
#include <sys/stat.h>

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

#endif

// ------ TYPES ------

/// @enum `clape_type_e`
/// @brief The enum describing the type of a clape value
typedef enum {
    CLAPE_TYPE_INT,
    CLAPE_TYPE_FLOAT,
    CLAPE_TYPE_BOOL,
    CLAPE_TYPE_FUNC,
    CLAPE_TYPE_STRING,
    CLAPE_TYPE_CHAR,
    CLAPE_TYPE_LIST,
    CLAPE_TYPE_UNIT,
    CLAPE_TYPE_PRODUCT,
    CLAPE_TYPE_SUM,
    CLAPE_TYPE_GENERIC,
} clape_type_e;

/// @struct `clape_type_t`
/// @brief A richer type descriptor, e.g. function signatures
typedef struct clape_type_t {
    /// @var `tag`
    /// @brief The tag telling us which type this is
    clape_type_e tag;

    /// @var `u`
    /// @brief A union of all possible types containing a payload
    union {
        /// @variation `func`
        /// @brief A Function type containing a parameter and return type
        struct {
            struct clape_type_t *param;
            struct clape_type_t *ret;
        } func;

        /// @variation `product`
        /// @brief A product type containing an array of field definitions
        struct {
            clape_arr_t *fields;
        } product;

        /// @variation `sum`
        /// @brief A sum type containing an array of variant definitions
        struct {
            clape_arr_t *variants;
        } sum;

        /// @variation `list`
        /// @brief An element type for list types (NULL if element type unknown/unchecked)
        struct clape_type_t *element;

        /// @variation `generic`
        /// @brief A generic type parameter placeholder (e.g. `T` in `Option<T>`)
        char *generic;
    } u;
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

/// @struct `clape_sum_variant_t`
/// @brief A single variant in a sum type definition
typedef struct {
    /// @var `name`
    /// @brief The constructor name (must start with uppercase)
    char *name;

    /// @var `has_type`
    /// @brief Whether this variant carries a payload type
    bool has_type;

    /// @var `type`
    /// @brief The payload type (only valid if has_type is true)
    clape_type_t type;
} clape_sum_variant_t;

/// @struct `clape_product_field_t`
/// @brief A single field in a product type definition
typedef struct {
    /// @var `name`
    /// @brief The name of the field
    char *name;

    /// @var `type`
    /// @brief The type of the field
    clape_type_t type;
} clape_product_field_t;

/// Forward declaration for self-referencing function pointer
typedef struct clape_value_t clape_value_t;
typedef clape_value_t (*clape_builtin_fn)(clape_value_t);

/// @struct `clape_value_t`
/// @brief Represents a single Clape value
struct clape_value_t {
    /// @var `type`
    /// @brief The type of this Clape value
    clape_type_t type;

    /// @var `u`
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

        /// @variation `sval`
        /// @brief A string value (heap-allocated)
        char *sval;

        /// @variation `cval`
        /// @brief A single character value
        char cval;

        /// @variation `list`
        /// @brief A list value (linked list of cons cells)
        struct clape_cons_t *list;

        /// @variation `product`
        /// @brief A product value (dynamic array of field values)
        clape_arr_t *product;

        /// @variation `sum`
        /// @brief A sum value (constructor name + optional payload)
        struct {
            /// @var `constructor`
            /// @brief The constructor name (e.g. "Some", "None")
            char *constructor;

            /// @var `value`
            /// @brief Pointer to the payload value, or NULL if no payload
            struct clape_value_t *value;
        } sum;
    } u;
};

/// @struct `clape_cons_t`
/// @brief A cons cell in a linked list
typedef struct clape_cons_t {
    /// @var `head`
    /// @brief The first element of this cons cell
    clape_value_t head;

    /// @var `tail`
    /// @brief A pointer to the rest of the list, or NULL for the empty list
    struct clape_cons_t *tail;

    /// @var `arc`
    /// @brief Reference count for shared list tails
    size_t arc;
} clape_cons_t;

/// @struct `clape_product_value_t`
/// @brief A single field in a product value
typedef struct {
    /// @var `name`
    /// @brief The name of the field
    char *name;

    /// @var `value`
    /// @brief The value of the field
    clape_value_t value;
} clape_product_value_t;

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
    CLAPE_BINOP_CONS,
    CLAPE_BINOP_PROD,
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
    CLAPE_EXPR_IF,
    CLAPE_EXPR_BLOCK,
    CLAPE_EXPR_LIST,
    CLAPE_EXPR_MATCH,
    CLAPE_EXPR_PRODUCT_TYPE,
    CLAPE_EXPR_FIELD,
    CLAPE_EXPR_FIELD_ACCESS,
    CLAPE_EXPR_TYPE,
    CLAPE_EXPR_SUM,
    CLAPE_EXPR_SUM_TYPE,
} clape_expr_e;

/// @struct `clape_expr_t`
/// @brief A clape expression
typedef struct clape_expr_t {
    /// @var `tag`
    /// @brief The tag telling us which type of expression this is
    clape_expr_e tag;

    /// @var `u`
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

            /// @var `return_type`
            /// @brief The return type of the function
            clape_type_t return_type;

            /// @var `body`
            /// @brief The body expression of the lambda definition, this is always a block
            struct clape_expr_t *body;

            /// @var `generic_params`
            /// @brief Array of char* generic parameter names, or NULL if not generic
            clape_arr_t *generic_params;
        } lambda;

        /// @variation `if_`
        /// @brief An if-then-else ternary expression: then_expr if cond_expr else else_expr
        struct {
            struct clape_expr_t *condition;
            struct clape_expr_t *then_branch;
            struct clape_expr_t *else_branch;
        } if_;

        /// @variation `block`
        /// @brief A block expression: { stmts; return_expr }
        struct {
            clape_arr_t *stmts;
            struct clape_expr_t *return_expr;
        } block;

        /// @variation `list`
        /// @brief A list literal expression: [expr, expr, ...]
        struct {
            /// @var `elements`
            /// @brief An array of all elements in the list literal expression, where every value of
            /// the array is of type `clape_expr_t`
            clape_arr_t *elements;
        } lst;

        /// @variation `match_`
        /// @brief A match expression: match scrutinee { arms }
        struct {
            struct clape_expr_t *scrutinee;
            /// @var `arms`
            /// @brief An array of all arms of the match expression, where every element of the
            /// array is of type `clape_expr_t`
            clape_arr_t *arms;
        } match_;

        /// @variation `product_type`
        /// @brief A product type expression: x(Float) & y(Int)
        struct {
            clape_arr_t *fields;
        } product_type;

        /// @variation `sum_type`
        /// @brief A sum type binding stub expression
        struct {
            clape_arr_t *variants;
        } sum_type;

        /// @variation `sum`
        /// @brief A sum value expression: .Some(expr) or .None
        struct {
            char *constructor;
            struct clape_expr_t *expr;
        } sum;

        /// @variation `field`
        /// @brief A field value construction: .field_name(expr)
        struct {
            char *name;
            struct clape_expr_t *value;
        } field;

        /// @variation `field_access`
        /// @brief A field access expression: expr.field_name
        struct {
            struct clape_expr_t *expr;
            char *name;
        } field_access;

        /// @variation `type_expr`
        /// @brief A type expression node, e.g. Int, x(Float), x(Float) & y(Float)
        struct {
            clape_type_t type;
        } type_expr;
    } u;
} clape_expr_t;

/// @enum `clape_pattern_tag_e`
/// @brief The enum for pattern match tags
typedef enum {
    CLAPE_PATTERN_ANY,
    CLAPE_PATTERN_LIT,
    CLAPE_PATTERN_VARIABLE,
    CLAPE_PATTERN_EMPTY_LIST,
    CLAPE_PATTERN_CONS,
    CLAPE_PATTERN_SUM,
} clape_pattern_tag_e;

/// @struct `clape_pattern_t`
/// @brief A pattern in a match arm
typedef struct clape_pattern_t {
    /// @var `tag`
    /// @brief The tag telling us which type of pattern this is
    clape_pattern_tag_e tag;

    /// @var `u`
    /// @brief A union of all possible pattern types
    union {
        /// @variation `lit`
        /// @brief A literal value as the matcher of the match expression
        clape_value_t lit;

        /// @variation `variable`
        /// @brief A variable as the matcher of the match expression
        char *variable;

        /// @variation `cons`
        /// @brief A cons operation as the matcher of the match expression, like `head :: tail`
        struct {
            struct clape_pattern_t *head;
            struct clape_pattern_t *tail;
        } cons;

        /// @variation `sum`
        /// @brief A sum pattern: .Constructor(var) or .Constructor
        struct {
            char *constructor;
            char *var;
            bool has_payload;
        } sum;
    } u;
} clape_pattern_t;

/// @struct `clape_match_arm_t`
/// @brief A single arm in a match expression
typedef struct {
    /// @var `pattern`
    /// @brief The pattern to match the arm for
    clape_pattern_t pattern;

    /// @var `body`
    /// @brief The expression which is the body of the match arm
    clape_expr_t body;
} clape_match_arm_t;

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

    /// @var `u`
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
    } u;
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

/// @struct `clape_generic_binding`
/// @brief A linked-list node mapping a generic parameter name to its inferred concrete type
typedef struct clape_generic_binding {
    /// @var `name`
    /// @brief The generic parameter name (e.g. "T")
    char *name;

    /// @var `type`
    /// @brief The inferred concrete type
    clape_type_t type;

    /// @var `next`
    /// @brief The next binding in the chain
    struct clape_generic_binding *next;
} clape_generic_binding_t;

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

    /// @var `generic_params`
    /// @brief Array of char* generic parameter names, or NULL if not generic
    clape_arr_t *generic_params;

    /// @var `genv`
    /// @brief Linked list of inferred generic type bindings (built up during partial application)
    struct clape_generic_binding *genv;
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
    TOK_IF,
    TOK_ELSE,
    TOK_MATCH,
    // Literals
    TOK_TRUE,
    TOK_FALSE,
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_CHAR,
    // Other
    TOK_IDENTIFIER,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COLON,
    TOK_ARROW,
    TOK_FAT_ARROW,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_CONS,
    TOK_DOT,
    TOK_AMP,
    TOK_PIPE,
    // Type keywords
    TOK_TYPE_INT,
    TOK_TYPE_FLOAT,
    TOK_TYPE_BOOL,
    TOK_TYPE_STRING,
    TOK_TYPE_CHAR,
    TOK_TYPE_UNIT,
    TOK_EOF,
} token_type_e;

/// @struct `token_t`
/// @brief The structure representing a single parsed token
typedef struct token_t {
    /// @var `tag`
    /// @brief The tag telling us which type of token this is
    token_type_e tag;

    /// @var `line`
    /// @brief The line number where this token appears (1-indexed)
    uint32_t line;

    /// @var `column`
    /// @brief The column number where this token starts (0-indexed)
    uint32_t column;

    /// @var `u`
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

        /// @variation `string`
        /// @brief A string literal token value
        char *string;

        /// @variation `cval`
        /// @brief A character literal value
        char cval;
    } u;
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
void clape_print_token(FILE *const stream, const token_t *const token);

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
    uint32_t current_line = 1;
    uint32_t current_column = 0;

    // Helper macro to record token position before advancing
#define RECORD_TOKEN_POS(t)                                                                        \
    do {                                                                                           \
        (t).line = current_line;                                                                   \
        (t).column = current_column;                                                               \
    } while (0)

    while (p != NULL) {
        // Skip whitespace
        while (is_whitespace(*p) || *p == '\n') {
            if (*p == '\n') {
                current_line++;
                current_column = 0;
            } else {
                current_column++;
            }
            p++;
        }

        if (*p == '\0') {
            token_t t = {.tag = TOK_EOF};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p = NULL;
            continue;
        }

        if (strncmp(p, "let", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_LET};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            current_column += 3;
            continue;
        }

        if (strncmp(p, "use", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_USE};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            current_column += 3;
            continue;
        }

        if (is_digit(*p)) {
            char *start = p;
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
                token_t t = {.tag = TOK_FLOAT, .u.fval = fval};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                current_column += (p - start);
            } else {
                token_t t = {.tag = TOK_INT, .u.ival = ival};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                current_column += (p - start);
            }
            continue;
        }

        if (strncmp(p, "true", 4) == 0 && !isalnum(p[4])) {
            token_t t = {.tag = TOK_TRUE};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 4;
            current_column += 4;
            continue;
        }

        if (strncmp(p, "false", 5) == 0 && !isalnum(p[5])) {
            token_t t = {.tag = TOK_FALSE};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 5;
            current_column += 5;
            continue;
        }

        if (strncmp(p, "and", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_AND};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            current_column += 3;
            continue;
        }

        if (strncmp(p, "or", 2) == 0 && !isalnum(p[2])) {
            token_t t = {.tag = TOK_OR};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 2;
            current_column += 2;
            continue;
        }

        if (strncmp(p, "not", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_NOT};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            current_column += 3;
            continue;
        }

        if (strncmp(p, "match", 5) == 0 && !isalnum(p[5])) {
            token_t t = {.tag = TOK_MATCH};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 5;
            current_column += 5;
            continue;
        }

        if (strncmp(p, "if", 2) == 0 && !isalnum(p[2])) {
            token_t t = {.tag = TOK_IF};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 2;
            current_column += 2;
            continue;
        }

        if (strncmp(p, "else", 4) == 0 && !isalnum(p[4])) {
            token_t t = {.tag = TOK_ELSE};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 4;
            current_column += 4;
            continue;
        }

        if (strncmp(p, "Int", 3) == 0 && !isalnum(p[3])) {
            token_t t = {.tag = TOK_TYPE_INT};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 3;
            current_column += 3;
            continue;
        }
        if (strncmp(p, "Float", 5) == 0 && !isalnum(p[5])) {
            token_t t = {.tag = TOK_TYPE_FLOAT};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 5;
            current_column += 5;
            continue;
        }
        if (strncmp(p, "Bool", 4) == 0 && !isalnum(p[4])) {
            token_t t = {.tag = TOK_TYPE_BOOL};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 4;
            current_column += 4;
            continue;
        }
        if (strncmp(p, "String", 6) == 0 && !isalnum(p[6])) {
            token_t t = {.tag = TOK_TYPE_STRING};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 6;
            current_column += 6;
            continue;
        }
        if (strncmp(p, "Char", 4) == 0 && !isalnum(p[4])) {
            token_t t = {.tag = TOK_TYPE_CHAR};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 4;
            current_column += 4;
            continue;
        }
        if (strncmp(p, "Unit", 4) == 0 && !isalnum(p[4])) {
            token_t t = {.tag = TOK_TYPE_UNIT};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p += 4;
            current_column += 4;
            continue;
        }

        if (*p == '"') {
            char *start = p;
            p++;
            clape_arr_t *str = clape_arr_create(1, 0);
            while (*p && *p != '"') {
                if (*p == '\\') {
                    p++;
                    switch (*p) {
                        case 'n': {
                            char n = '\n';
                            clape_arr_append(1, &str, &n);
                            break;
                        }
                        case 't': {
                            char t = '\t';
                            clape_arr_append(1, &str, &t);
                            break;
                        }
                        case '\\': {
                            char b = '\\';
                            clape_arr_append(1, &str, &b);
                            break;
                        }
                        case '"': {
                            char q = '"';
                            clape_arr_append(1, &str, &q);
                            break;
                        }
                        default:
                            clape_arr_append(1, &str, p);
                            break;
                    }
                } else {
                    clape_arr_append(1, &str, p);
                }
                p++;
            }
            if (*p == '"') {
                p++;
            }
            char *s = malloc(str->len + 1);
            memcpy(s, str->value, str->len);
            s[str->len] = '\0';
            free(str);
            token_t t = {.tag = TOK_STRING, .u.string = s};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            current_column += (p - start);
            continue;
        }

        if (*p == '\'') {
            char *start = p;
            p++;
            char c;
            if (*p == '\\') {
                p++;
                switch (*p) {
                    case 'n':
                        c = '\n';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    case '\\':
                        c = '\\';
                        break;
                    case '\'':
                        c = '\'';
                        break;
                    default:
                        c = *p;
                        break;
                }
                p++;
            } else {
                c = *p;
                p++;
            }
            if (*p == '\'') {
                p++;
            }
            token_t t = {.tag = TOK_CHAR, .u.cval = c};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            current_column += (p - start);
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

            token_t t = {.tag = TOK_IDENTIFIER, .u.identifier = ident};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            current_column += len;
            continue;
        }

        if (*p == '=') {
            if (*(p + 1) == '=') {
                token_t t = {.tag = TOK_EQ_EQ};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
                current_column += 2;
            } else if (*(p + 1) == '>') {
                token_t t = {.tag = TOK_FAT_ARROW};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
                current_column += 2;
            } else {
                token_t t = {.tag = TOK_EQ};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p++;
                current_column++;
            }
            continue;
        }
        if (*p == '<') {
            if (*(p + 1) == '=') {
                token_t t = {.tag = TOK_LE};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
                current_column += 2;
            } else {
                token_t t = {.tag = TOK_LT};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p++;
                current_column++;
            }
            continue;
        }
        if (*p == '>') {
            if (*(p + 1) == '=') {
                token_t t = {.tag = TOK_GE};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
                current_column += 2;
            } else {
                token_t t = {.tag = TOK_GT};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p++;
                current_column++;
            }
            continue;
        }
        if (*p == '!') {
            if (*(p + 1) == '=') {
                token_t t = {.tag = TOK_NE};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
                current_column += 2;
            } else {
                fprintf(stderr, "Expected '=' after '!'\n");
                return NULL;
            }
            continue;
        }
        if (*p == '+') {
            token_t t = {.tag = TOK_PLUS};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == '-') {
            if (*(p + 1) == '>') {
                token_t t = {.tag = TOK_ARROW};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
                current_column += 2;
            } else {
                token_t t = {.tag = TOK_MINUS};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p++;
                current_column++;
            }
            continue;
        }
        if (*p == '*') {
            token_t t = {.tag = TOK_MUL};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == '/') {
            token_t t = {.tag = TOK_DIV};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == '(') {
            token_t t = {.tag = TOK_LPAREN};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == ')') {
            token_t t = {.tag = TOK_RPAREN};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == '{') {
            token_t t = {.tag = TOK_LBRACE};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == '}') {
            token_t t = {.tag = TOK_RBRACE};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == '[') {
            token_t t = {.tag = TOK_LBRACKET};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == ']') {
            token_t t = {.tag = TOK_RBRACKET};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == '.') {
            token_t t = {.tag = TOK_DOT};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == '&') {
            token_t t = {.tag = TOK_AMP};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == '|') {
            token_t t = {.tag = TOK_PIPE};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
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
            if (*(p + 1) == ':') {
                token_t t = {.tag = TOK_CONS};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p += 2;
                current_column += 2;
            } else {
                token_t t = {.tag = TOK_COLON};
                RECORD_TOKEN_POS(t);
                clape_arr_append(sizeof(token_t), &tokens, &t);
                p++;
                current_column++;
            }
            continue;
        }
        if (*p == ',') {
            token_t t = {.tag = TOK_COMMA};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }
        if (*p == ';') {
            token_t t = {.tag = TOK_SEMICOLON};
            RECORD_TOKEN_POS(t);
            clape_arr_append(sizeof(token_t), &tokens, &t);
            p++;
            current_column++;
            continue;
        }

        // Unknown character → error (for now just skip or handle later)
        fprintf(stderr, "Unknown character: '%c'\n", *p);
        return NULL;
    }

    return tokens;
}

void clape_print_token(FILE *const stream, const token_t *const token) {
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
        case TOK_MATCH:
            fprintf(stream, "match");
            break;
        case TOK_IF:
            fprintf(stream, "if");
            break;
        case TOK_ELSE:
            fprintf(stream, "else");
            break;
        case TOK_TRUE:
            fprintf(stream, "true");
            break;
        case TOK_FALSE:
            fprintf(stream, "false");
            break;
        case TOK_INT:
            fprintf(stream, "%li", token->u.ival);
            break;
        case TOK_FLOAT:
            fprintf(stream, "%g", token->u.fval);
            break;
        case TOK_STRING:
            fprintf(stream, "\"%s\"", token->u.string);
            break;
        case TOK_CHAR:
            fprintf(stream, "'%c'", token->u.cval);
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
        case TOK_FAT_ARROW:
            fprintf(stream, "=>");
            break;
        case TOK_CONS:
            fprintf(stream, "::");
            break;
        case TOK_LBRACKET:
            fprintf(stream, "[");
            break;
        case TOK_RBRACKET:
            fprintf(stream, "]");
            break;
        case TOK_COMMA:
            fprintf(stream, ",");
            break;
        case TOK_SEMICOLON:
            fprintf(stream, ";");
            break;
        case TOK_DOT:
            fprintf(stream, ".");
            break;
        case TOK_AMP:
            fprintf(stream, "&");
            break;
        case TOK_PIPE:
            fprintf(stream, "|");
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
        case TOK_TYPE_STRING:
            fprintf(stream, "String");
            break;
        case TOK_TYPE_CHAR:
            fprintf(stream, "Char");
            break;
        case TOK_TYPE_UNIT:
            fprintf(stream, "Unit");
            break;
        case TOK_IDENTIFIER:
            fprintf(stream, "%s", token->u.identifier);
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
            case TOK_MATCH:
            case TOK_IF:
            case TOK_ELSE:
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
            case TOK_FAT_ARROW:
            case TOK_CONS:
            case TOK_LBRACKET:
            case TOK_RBRACKET:
            case TOK_COMMA:
            case TOK_SEMICOLON:
            case TOK_DOT:
            case TOK_AMP:
            case TOK_PIPE:
            case TOK_TYPE_INT:
            case TOK_TYPE_FLOAT:
            case TOK_TYPE_BOOL:
            case TOK_TYPE_STRING:
            case TOK_TYPE_CHAR:
            case TOK_TYPE_UNIT:
            case TOK_CHAR:
                break;
            case TOK_IDENTIFIER:
                free(tok->u.identifier);
                break;
            case TOK_STRING:
                free(tok->u.string);
                break;
            case TOK_EOF:
                break;
        }
    }
    free(tokens);
}

static void throw_err(          //
    FILE *const stream,         //
    const char *const msg,      //
    const token_t *const token, //
    const bool print_token      //
) {
    if (token != NULL) {
        fprintf(stream, "Error: %u:%u: %s", token->line, token->column, msg);
        if (print_token) {
            clape_print_token(stream, token);
        }
        fprintf(stream, "\n");
    } else {
        fprintf(stream, "Error: %s\n", msg);
    }
    exit(1);
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
    CLAPE_BINDING_POWER_IF,
    CLAPE_BINDING_POWER_LOGICAL,
    CLAPE_BINDING_POWER_RELATIONAL,
    CLAPE_BINDING_POWER_TERM,
    CLAPE_BINDING_POWER_FACTOR,
    CLAPE_BINDING_POWER_CONS,
    CLAPE_BINDING_POWER_CALL,
} clape_binding_power_t;

typedef struct {
    clape_arr_t *tokens;
    size_t pos;

    /// @var `type_env`
    /// @brief Linked list of product / sum type bindings (populated at parse time only)
    struct clape_type_binding {
        /// @var `name`
        /// @brief The name of the binding
        char *name;

        /// @var `generic_params`
        /// @brief Array of char* generic parameter names, or NULL if not generic
        clape_arr_t *generic_params;

        /// @var `type_body`
        /// @brief The full type stored in this binding (product, sum, list, etc.)
        clape_type_t type_body;

        /// @var `next`
        /// @brief The next type binding in the environment
        struct clape_type_binding *next;
    } *type_env;

    /// @var `generic_names`
    /// @brief Array of generic parameter names (char*) active during RHS parsing, like `T` in
    /// `let Option<T> = ...`
    clape_arr_t *generic_names;
} clape_parser_t;

static token_t *clape_peek(clape_parser_t *p) {
    return ACCESS_ARR_AT(token_t, p->tokens, p->pos);
}

static token_t *clape_advance(clape_parser_t *p) {
    return ACCESS_ARR_AT(token_t, p->tokens, p->pos++);
}

static clape_stmt_t clape_parse_stmt(clape_parser_t *p);
static clape_expr_t clape_parse_expr(clape_parser_t *p, clape_binding_power_t min_bp);
static clape_expr_t clape_parse_block_body(clape_parser_t *p);
static clape_expr_t clape_parse_block(clape_parser_t *p);
static void clape_free_type(clape_type_t *const type);
static clape_type_t clape_type_clone(const clape_type_t *const type);
static const clape_generic_binding_t *clape_find_generic( //
    const clape_generic_binding_t *genv, const char *name //
);
static clape_generic_binding_t *clape_genv_prepend(                    //
    clape_generic_binding_t *genv, const char *name, clape_type_t type //
);
static void clape_genv_free(clape_generic_binding_t *genv);
static clape_type_t clape_type_substitute(                        //
    const clape_type_t *type, const clape_generic_binding_t *genv //
);

static clape_type_t clape_parse_type(clape_parser_t *p) {
    token_t *tok = clape_peek(p);
    clape_type_t result = {.tag = CLAPE_TYPE_UNIT};

    if (tok->tag == TOK_LPAREN) {
        clape_advance(p);
        clape_type_t param = clape_parse_type(p);
        if (param.tag == CLAPE_TYPE_UNIT) {
            fprintf(stderr, "Unit cannot be used as a parameter type\n");
            exit(1);
        }
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
        result = (clape_type_t){
            .tag = CLAPE_TYPE_FUNC,
            .u.func = {.param = param_ptr, .ret = ret_ptr},
        };
        goto check_amp;
    }

    if (tok->tag == TOK_LBRACKET) {
        clape_advance(p);
        clape_type_t *const inner = malloc(sizeof(clape_type_t));
        *inner = clape_parse_type(p);
        token_t *const close = clape_advance(p);
        if (close->tag != TOK_RBRACKET) {
            fprintf(stderr, "Expected ']' in list type\n");
            exit(1);
        }
        result = (clape_type_t){.tag = CLAPE_TYPE_LIST, .u.element = inner};
        goto check_amp;
    }

    // Inline product field: name(Type)
    if (tok->tag == TOK_IDENTIFIER && p->pos + 1 < p->tokens->len &&
        ACCESS_ARR_AT(token_t, p->tokens, p->pos + 1)->tag == TOK_LPAREN) {
        clape_arr_t *fields = clape_arr_create(sizeof(clape_product_field_t), 0);
        // Parse one field
        tok = clape_advance(p);
        char *field_name = strdup(tok->u.identifier);
        clape_advance(p); // consume (
        clape_type_t field_type = clape_parse_type(p);
        tok = clape_advance(p);
        if (tok->tag != TOK_RPAREN) {
            fprintf(stderr, "Expected ')' after field type\n");
            exit(1);
        }
        clape_product_field_t field = {.name = field_name, .type = field_type};
        clape_arr_append(sizeof(clape_product_field_t), &fields, &field);
        result = (clape_type_t){
            .tag = CLAPE_TYPE_PRODUCT,
            .u.product = {.fields = fields},
        };
        goto check_amp;
    }

    tok = clape_advance(p);
    switch (tok->tag) {
        case TOK_TYPE_INT:
            result = (clape_type_t){.tag = CLAPE_TYPE_INT};
            break;
        case TOK_TYPE_FLOAT:
            result = (clape_type_t){.tag = CLAPE_TYPE_FLOAT};
            break;
        case TOK_TYPE_BOOL:
            result = (clape_type_t){.tag = CLAPE_TYPE_BOOL};
            break;
        case TOK_TYPE_STRING:
            result = (clape_type_t){.tag = CLAPE_TYPE_STRING};
            break;
        case TOK_TYPE_CHAR:
            result = (clape_type_t){.tag = CLAPE_TYPE_CHAR};
            break;
        case TOK_TYPE_UNIT:
            result = (clape_type_t){.tag = CLAPE_TYPE_UNIT};
            break;
        case TOK_IDENTIFIER: {
            // Check if this is a generic param name
            if (p->generic_names != NULL) {
                for (size_t i = 0; i < p->generic_names->len; i++) {
                    char *const name = *ACCESS_ARR_AT(char *, p->generic_names, i);
                    if (strcmp(name, tok->u.identifier) != 0) {
                        continue;
                    }
                    result = (clape_type_t){
                        .tag = CLAPE_TYPE_GENERIC,
                        .u.generic = strdup(name),
                    };
                    return result;
                }
            }
            for (struct clape_type_binding *b = p->type_env; b; b = b->next) {
                if (strcmp(b->name, tok->u.identifier) != 0) {
                    continue;
                }
                if (clape_peek(p)->tag == TOK_LT) {
                    // Type application: Option<Int>
                    if (b->generic_params == NULL || b->generic_params->len == 0) {
                        fprintf(stderr, "Type '%s' is not generic\n", b->name);
                        exit(1);
                    }
                    // consume the `<`
                    clape_advance(p);
                    const size_t nparams = b->generic_params->len;
                    clape_generic_binding_t *genv = NULL;
                    for (size_t arg_index = 0; arg_index < nparams; arg_index++) {
                        clape_type_t arg_type = clape_parse_type(p);
                        char *const pname = *ACCESS_ARR_AT(char *, b->generic_params, arg_index);
                        genv = clape_genv_prepend(genv, pname, arg_type);
                        if (arg_index + 1 < nparams) {
                            token_t *const comma = clape_advance(p);
                            if (comma->tag != TOK_COMMA) {
                                fprintf(stderr, "Expected ',' in generic type arguments\n");
                                exit(1);
                            }
                        }
                    }
                    token_t *const close = clape_advance(p);
                    if (close->tag != TOK_GT) {
                        fprintf(stderr, "Expected '>' after generic type arguments\n");
                        exit(1);
                    }
                    // Substitute the type body
                    result = clape_type_substitute(&b->type_body, genv);
                    clape_genv_free(genv);
                    // Continue with & or | combining
                    goto check_amp;
                }
                // Non-generic reference: clone the stored type body
                result = clape_type_clone(&b->type_body);
                if (result.tag == CLAPE_TYPE_PRODUCT) {
                    goto check_amp;
                }
                // For sum types, allow pipe combination
                if (clape_peek(p)->tag == TOK_PIPE && result.tag != CLAPE_TYPE_SUM) {
                    fprintf(stderr, "'|' requires sum types\n");
                    exit(1);
                }
                while (clape_peek(p)->tag == TOK_PIPE) {
                    clape_advance(p);
                    clape_type_t rhs = clape_parse_type(p);
                    if (rhs.tag != CLAPE_TYPE_SUM) {
                        fprintf(stderr, "'|' requires sum types\n");
                        exit(1);
                    }
                    for (size_t i = 0; i < rhs.u.sum.variants->len; i++) {
                        clape_sum_variant_t *const src = ACCESS_ARR_AT( //
                            clape_sum_variant_t, rhs.u.sum.variants, i  //
                        );
                        bool found = false;
                        for (size_t j = 0; j < result.u.sum.variants->len; j++) {
                            clape_sum_variant_t *const dest = ACCESS_ARR_AT(  //
                                clape_sum_variant_t, result.u.sum.variants, j //
                            );
                            if (strcmp(dest->name, src->name) != 0) {
                                continue;
                            }
                            fprintf(                                                          //
                                stderr, "Duplicate constructor '%s' in sum type\n", src->name //
                            );
                            exit(1);
                        }
                        if (found) {
                            continue;
                        }
                        clape_sum_variant_t v = {
                            .name = strdup(src->name),
                            .has_type = src->has_type,
                        };
                        if (src->has_type) {
                            v.type = clape_type_clone(&src->type);
                        }
                        clape_arr_append(                                           //
                            sizeof(clape_sum_variant_t), &result.u.sum.variants, &v //
                        );
                    }
                    if (rhs.u.sum.variants != NULL) {
                        for (size_t i = 0; i < rhs.u.sum.variants->len; i++) {
                            clape_sum_variant_t *const v = ACCESS_ARR_AT(  //
                                clape_sum_variant_t, rhs.u.sum.variants, i //
                            );
                            free(v->name);
                            if (v->has_type) {
                                clape_free_type(&v->type);
                            }
                        }
                        free(rhs.u.sum.variants);
                    }
                }
                return result;
            }
            fprintf(stderr, "Unknown type name '%s'\n", tok->u.identifier);
            exit(1);
        }
        default:
            fprintf(stderr,
                "Expected a type (Int, Float, Bool, String, Char, Unit, [T], (T -> U), or type name)\n");
            exit(1);
    }
    return result;

check_amp:
    while (clape_peek(p)->tag == TOK_AMP) {
        clape_advance(p);
        clape_type_t rhs = clape_parse_type(p);
        if (rhs.tag != CLAPE_TYPE_PRODUCT) {
            fprintf(stderr, "'&' requires product types\n");
            exit(1);
        }
        for (size_t i = 0; i < rhs.u.product.fields->len; i++) {
            clape_product_field_t *const src = ACCESS_ARR_AT(  //
                clape_product_field_t, rhs.u.product.fields, i //
            );
            bool found = false;
            for (size_t j = 0; j < result.u.product.fields->len; j++) {
                clape_product_field_t *const dest = ACCESS_ARR_AT(    //
                    clape_product_field_t, result.u.product.fields, j //
                );
                if (strcmp(dest->name, src->name) != 0) {
                    continue;
                }
                free(dest->name);
                clape_free_type(&dest->type);
                dest->name = strdup(src->name);
                dest->type = clape_type_clone(&src->type);
                found = true;
                break;
            }
            if (found) {
                continue;
            }
            clape_product_field_t field = {
                .name = strdup(src->name),
                .type = clape_type_clone(&src->type),
            };
            clape_arr_append(sizeof(clape_product_field_t), &result.u.product.fields, &field);
        }
        if (rhs.u.product.fields != NULL) {
            for (size_t i = 0; i < rhs.u.product.fields->len; i++) {
                clape_product_field_t *const field = ACCESS_ARR_AT( //
                    clape_product_field_t, rhs.u.product.fields, i  //
                );
                free(field->name);
                clape_free_type(&field->type);
            }
            free(rhs.u.product.fields);
        }
    }
    return result;
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
        case TOK_CONS:
            return CLAPE_BINDING_POWER_CONS;
        case TOK_AMP:
        case TOK_PIPE:
            return CLAPE_BINDING_POWER_FACTOR;
        case TOK_DOT:
            return CLAPE_BINDING_POWER_CALL;
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

static void clape_free_expr(clape_expr_t *expr);

static clape_pattern_t clape_parse_primary_pattern(clape_parser_t *p) {
    token_t *tok = clape_peek(p);
    if (tok->tag == TOK_IDENTIFIER && strcmp(tok->u.identifier, "_") == 0) {
        clape_advance(p);
        return (clape_pattern_t){.tag = CLAPE_PATTERN_ANY};
    }
    if (tok->tag == TOK_LBRACKET) {
        if (p->pos + 1 < p->tokens->len &&
            ACCESS_ARR_AT(token_t, p->tokens, p->pos + 1)->tag == TOK_RBRACKET) {
            clape_advance(p);
            clape_advance(p);
            return (clape_pattern_t){.tag = CLAPE_PATTERN_EMPTY_LIST};
        }
        fprintf(stderr, "Non-empty list patterns not yet supported\n");
        exit(1);
    }
    if (tok->tag == TOK_INT || tok->tag == TOK_FLOAT || tok->tag == TOK_TRUE ||
        tok->tag == TOK_FALSE || tok->tag == TOK_STRING || tok->tag == TOK_CHAR) {
        clape_expr_t lit = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);
        clape_pattern_t pat = {.tag = CLAPE_PATTERN_LIT, .u.lit = lit.u.lit};
        if (lit.u.lit.type.tag == CLAPE_TYPE_STRING) {
            pat.u.lit.u.sval = strdup(lit.u.lit.u.sval);
        }
        clape_free_expr(&lit);
        return pat;
    }
    if (tok->tag == TOK_DOT) {
        clape_advance(p);
        token_t *ctor = clape_advance(p);
        if (ctor->tag != TOK_IDENTIFIER || !isupper(ctor->u.identifier[0])) {
            fprintf(stderr, "Expected uppercase constructor name after '.' in pattern\n");
            exit(1);
        }
        if (clape_peek(p)->tag == TOK_LPAREN) {
            clape_advance(p); // consume (
            token_t *var = clape_advance(p);
            if (var->tag != TOK_IDENTIFIER) {
                fprintf(stderr, "Expected variable name in sum pattern\n");
                exit(1);
            }
            token_t *close = clape_advance(p);
            if (close->tag != TOK_RPAREN) {
                fprintf(stderr, "Expected ')' after variable in sum pattern\n");
                exit(1);
            }
            return (clape_pattern_t){
                .tag = CLAPE_PATTERN_SUM,
                .u.sum =
                    {
                        .constructor = strdup(ctor->u.identifier),
                        .var = strdup(var->u.identifier),
                        .has_payload = true,
                    },
            };
        } else {
            return (clape_pattern_t){
                .tag = CLAPE_PATTERN_SUM,
                .u.sum =
                    {
                        .constructor = strdup(ctor->u.identifier),
                        .var = NULL,
                        .has_payload = false,
                    },
            };
        }
    }
    if (tok->tag == TOK_IDENTIFIER) {
        clape_advance(p);
        return (clape_pattern_t){
            .tag = CLAPE_PATTERN_VARIABLE,
            .u.variable = strdup(tok->u.identifier),
        };
    }
    fprintf(stderr, "Expected pattern\n");
    exit(1);
}

static clape_pattern_t clape_parse_pattern(clape_parser_t *p) {
    clape_pattern_t pat = clape_parse_primary_pattern(p);
    if (clape_peek(p)->tag == TOK_CONS) {
        clape_advance(p);
        clape_pattern_t *head = malloc(sizeof(clape_pattern_t));
        *head = pat;
        clape_pattern_t *tail = malloc(sizeof(clape_pattern_t));
        *tail = clape_parse_pattern(p);
        return (clape_pattern_t){
            .tag = CLAPE_PATTERN_CONS,
            .u.cons = {.head = head, .tail = tail},
        };
    }
    return pat;
}

static clape_expr_t clape_parse_expr(clape_parser_t *p, clape_binding_power_t min_bp) {
    token_t *tok = clape_advance(p);
    clape_expr_t lhs;

    switch (tok->tag) {
        case TOK_INT:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .u.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_INT},
                        .u.ival = tok->u.ival,
                    },
            };
            break;
        case TOK_FLOAT:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .u.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_FLOAT},
                        .u.fval = tok->u.fval,
                    },
            };
            break;
        case TOK_TRUE:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .u.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .u.bval = true,
                    },
            };
            break;
        case TOK_FALSE:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .u.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .u.bval = false,
                    },
            };
            break;
        case TOK_STRING: {
            size_t len = strlen(tok->u.string);
            char *s = malloc(len + 1);
            memcpy(s, tok->u.string, len + 1);
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .u.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_STRING},
                        .u.sval = s,
                    },
            };
            break;
        }
        case TOK_CHAR:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIT,
                .u.lit =
                    (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_CHAR},
                        .u.cval = tok->u.cval,
                    },
            };
            break;
        case TOK_TYPE_INT:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_TYPE,
                .u.type_expr = {.type = {.tag = CLAPE_TYPE_INT}},
            };
            break;
        case TOK_TYPE_FLOAT:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_TYPE,
                .u.type_expr = {.type = {.tag = CLAPE_TYPE_FLOAT}},
            };
            break;
        case TOK_TYPE_BOOL:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_TYPE,
                .u.type_expr = {.type = {.tag = CLAPE_TYPE_BOOL}},
            };
            break;
        case TOK_TYPE_STRING:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_TYPE,
                .u.type_expr = {.type = {.tag = CLAPE_TYPE_STRING}},
            };
            break;
        case TOK_TYPE_CHAR:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_TYPE,
                .u.type_expr = {.type = {.tag = CLAPE_TYPE_CHAR}},
            };
            break;
        case TOK_TYPE_UNIT:
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_TYPE,
                .u.type_expr = {.type = {.tag = CLAPE_TYPE_UNIT}},
            };
            break;
        case TOK_NOT: {
            clape_expr_t *op = malloc(sizeof(clape_expr_t));
            *op = clape_parse_expr(p, CLAPE_BINDING_POWER_CALL);
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_UNARY,
                .u.unary = {.op = CLAPE_UNOP_NOT, .operand = op},
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
                fprintf(stderr, "'(' is only for lambda parameters and field definitions\n");
                exit(1);
            }

            clape_arr_t *params = clape_arr_create(sizeof(clape_param_t), 0);
            if (next->tag != TOK_RPAREN) {
                while (true) {
                    token_t *name_tok = clape_advance(p);
                    clape_advance(p);
                    clape_type_t param_type = clape_parse_type(p);
                    if (param_type.tag == CLAPE_TYPE_UNIT) {
                        fprintf(stderr, "Unit cannot be used as a parameter type\n");
                        exit(1);
                    }
                    clape_param_t param = {
                        .name = strdup(name_tok->u.identifier),
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
                .u.lambda = {.params = params, .return_type = return_type, .body = body_ptr},
            };
            break;
        }
        case TOK_LBRACKET: {
            // The `[` was already consumed at line 2022
            clape_arr_t *elems = clape_arr_create(sizeof(clape_expr_t), 0);
            if (clape_peek(p)->tag != TOK_RBRACKET) {
                while (true) {
                    clape_expr_t e = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);
                    clape_arr_append(sizeof(clape_expr_t), &elems, &e);
                    if (clape_peek(p)->tag == TOK_COMMA) {
                        clape_advance(p);
                    } else {
                        break;
                    }
                }
            }
            token_t *close = clape_advance(p);
            if (close->tag != TOK_RBRACKET) {
                fprintf(stderr, "Expected ']'\n");
                exit(1);
            }
            // If exactly one element and it's a type expr, treat as list type [T]
            if (elems->len == 1) {
                clape_expr_t *const only = ACCESS_ARR_AT(clape_expr_t, elems, 0);
                if (only->tag == CLAPE_EXPR_TYPE) {
                    clape_type_t *const inner = malloc(sizeof(clape_type_t));
                    *inner = clape_type_clone(&only->u.type_expr.type);
                    clape_free_expr(only);
                    free(elems);
                    lhs = (clape_expr_t){
                        .tag = CLAPE_EXPR_TYPE,
                        .u.type_expr = {.type = {.tag = CLAPE_TYPE_LIST, .u.element = inner}},
                    };
                    break;
                }
            }
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_LIST,
                .u.lst = {.elements = elems},
            };
            break;
        }
        case TOK_MATCH: {
            clape_expr_t *scrutinee = malloc(sizeof(clape_expr_t));
            *scrutinee = clape_parse_expr(p, CLAPE_BINDING_POWER_CALL);
            token_t *open = clape_advance(p);
            if (open->tag != TOK_LBRACE) {
                fprintf(stderr, "Expected '{' after match scrutinee\n");
                exit(1);
            }
            clape_arr_t *arms = clape_arr_create(sizeof(clape_match_arm_t), 0);
            while (clape_peek(p)->tag != TOK_RBRACE && clape_peek(p)->tag != TOK_EOF) {
                while (clape_peek(p)->tag == TOK_SEMICOLON) {
                    clape_advance(p);
                }
                clape_match_arm_t arm;
                arm.pattern = clape_parse_pattern(p);
                token_t *arrow = clape_advance(p);
                if (arrow->tag != TOK_FAT_ARROW) {
                    fprintf(stderr, "Expected '=>' in match arm\n");
                    exit(1);
                }
                arm.body = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);
                clape_arr_append(sizeof(clape_match_arm_t), &arms, &arm);
            }
            token_t *close_brace = clape_advance(p);
            if (close_brace->tag != TOK_RBRACE) {
                fprintf(stderr, "Expected '}'\n");
                exit(1);
            }
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_MATCH,
                .u.match_ = {.scrutinee = scrutinee, .arms = arms},
            };
            break;
        }
        case TOK_DOT: {
            token_t *field_tok = clape_advance(p);
            if (field_tok->tag != TOK_IDENTIFIER) {
                fprintf(stderr, "Expected field name after '.'\n");
                exit(1);
            }
            if (isupper(field_tok->u.identifier[0])) {
                // Sum constructor value: .Some(expr) or .None
                char *constructor = strdup(field_tok->u.identifier);
                if (clape_peek(p)->tag == TOK_LPAREN) {
                    clape_advance(p); // consume (
                    clape_expr_t *val = malloc(sizeof(clape_expr_t));
                    *val = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);
                    token_t *close = clape_advance(p);
                    if (close->tag != TOK_RPAREN) {
                        fprintf(stderr, "Expected ')' after sum constructor value\n");
                        exit(1);
                    }
                    lhs = (clape_expr_t){
                        .tag = CLAPE_EXPR_SUM,
                        .u.sum = {.constructor = constructor, .expr = val},
                    };
                } else {
                    lhs = (clape_expr_t){
                        .tag = CLAPE_EXPR_SUM,
                        .u.sum = {.constructor = constructor, .expr = NULL},
                    };
                }
            } else {
                // Product field value: .field_name(expr)
                token_t *open = clape_advance(p);
                if (open->tag != TOK_LPAREN) {
                    fprintf(stderr, "Expected '(' after field name in field construction\n");
                    exit(1);
                }
                clape_expr_t *val = malloc(sizeof(clape_expr_t));
                *val = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);
                token_t *close = clape_advance(p);
                if (close->tag != TOK_RPAREN) {
                    fprintf(stderr, "Expected ')' after field value\n");
                    exit(1);
                }
                lhs = (clape_expr_t){
                    .tag = CLAPE_EXPR_FIELD,
                    .u.field = {.name = strdup(field_tok->u.identifier), .value = val},
                };
            }
            break;
        }
        case TOK_LBRACE:
            lhs = clape_parse_block_body(p);
            break;
        case TOK_IDENTIFIER: {
            token_t *next = clape_peek(p);
            bool is_upper = isupper(tok->u.identifier[0]);
            if (next->tag == TOK_LPAREN && !is_upper) {
                // Product field definition: name(Type)
                clape_advance(p); // consume (
                clape_type_t field_type = clape_parse_type(p);
                token_t *close = clape_advance(p);
                if (close->tag != TOK_RPAREN) {
                    fprintf(stderr, "Expected ')' after field type\n");
                    exit(1);
                }
                clape_arr_t *fields = clape_arr_create(sizeof(clape_product_field_t), 0);
                clape_product_field_t field = {
                    .name = strdup(tok->u.identifier),
                    .type = field_type,
                };
                clape_arr_append(sizeof(clape_product_field_t), &fields, &field);
                lhs = (clape_expr_t){
                    .tag = CLAPE_EXPR_TYPE,
                    .u.type_expr =
                        {
                            .type = {.tag = CLAPE_TYPE_PRODUCT, .u.product = {.fields = fields}},
                        },
                };
            } else if (next->tag == TOK_LPAREN && is_upper) {
                // Sum type constructor with payload: Some(Int)
                clape_advance(p); // consume (
                clape_type_t inner = clape_parse_type(p);
                token_t *close = clape_advance(p);
                if (close->tag != TOK_RPAREN) {
                    fprintf(stderr, "Expected ')' after sum constructor type\n");
                    exit(1);
                }
                clape_arr_t *variants = clape_arr_create(sizeof(clape_sum_variant_t), 0);
                clape_sum_variant_t v = {
                    .name = strdup(tok->u.identifier),
                    .has_type = true,
                    .type = inner,
                };
                clape_arr_append(sizeof(clape_sum_variant_t), &variants, &v);
                lhs = (clape_expr_t){
                    .tag = CLAPE_EXPR_TYPE,
                    .u.type_expr =
                        {
                            .type = {.tag = CLAPE_TYPE_SUM, .u.sum = {.variants = variants}},
                        },
                };
            } else {
                // Check if this identifier is a generic parameter name
                bool is_generic_param = false;
                if (p->generic_names != NULL) {
                    for (size_t i = 0; i < p->generic_names->len; i++) {
                        char *const name = *ACCESS_ARR_AT(char *, p->generic_names, i);
                        if (strcmp(name, tok->u.identifier) != 0) {
                            continue;
                        }
                        lhs = (clape_expr_t){
                            .tag = CLAPE_EXPR_TYPE,
                            .u.type_expr = {.type =
                                                {
                                                    .tag = CLAPE_TYPE_GENERIC,
                                                    .u.generic = strdup(name),
                                                }},
                        };
                        is_generic_param = true;
                        break;
                    }
                }
                if (!is_generic_param) {
                    // Check if this identifier is a known type name
                    bool in_type_env = false;
                    for (struct clape_type_binding *b = p->type_env; b; b = b->next) {
                        if (strcmp(b->name, tok->u.identifier) == 0) {
                            in_type_env = true;
                            break;
                        }
                    }
                    if (in_type_env) {
                        for (struct clape_type_binding *b = p->type_env; b; b = b->next) {
                            if (strcmp(b->name, tok->u.identifier) != 0) {
                                continue;
                            }
                            if (clape_peek(p)->tag != TOK_LT) {
                                lhs = (clape_expr_t){
                                    .tag = CLAPE_EXPR_TYPE,
                                    .u.type_expr = {.type = clape_type_clone(&b->type_body)},
                                };
                                break;
                            }
                            if (!b->generic_params || b->generic_params->len == 0) {
                                fprintf(stderr, "Type '%s' is not generic\n", b->name);
                                exit(1);
                            }
                            clape_advance(p);
                            const size_t nparams = b->generic_params->len;
                            clape_generic_binding_t *genv = NULL;
                            for (size_t arg_index = 0; arg_index < nparams; arg_index++) {
                                const clape_type_t arg_type = clape_parse_type(p);
                                char *const pname = *ACCESS_ARR_AT(      //
                                    char *, b->generic_params, arg_index //
                                );
                                genv = clape_genv_prepend(genv, pname, arg_type);
                                if (arg_index + 1 < nparams) {
                                    token_t *const comma = clape_advance(p);
                                    if (comma->tag != TOK_COMMA) {
                                        fprintf(stderr, "Expected ',' in generic type arguments\n");
                                        exit(1);
                                    }
                                }
                            }
                            token_t *const close = clape_advance(p);
                            if (close->tag != TOK_GT) {
                                fprintf(stderr, "Expected '>' after generic type arguments\n");
                                exit(1);
                            }
                            clape_type_t type_body = clape_type_clone(&b->type_body);
                            clape_type_t result = clape_type_substitute(&type_body, genv);
                            clape_free_type(&type_body);
                            clape_genv_free(genv);
                            lhs = (clape_expr_t){
                                .tag = CLAPE_EXPR_TYPE,
                                .u.type_expr = {.type = result},
                            };
                        }
                    } else if (is_upper) {
                        // Standalone sum constructor without payload: None (in type definition)
                        clape_arr_t *variants = clape_arr_create(sizeof(clape_sum_variant_t), 0);
                        clape_sum_variant_t v = {
                            .name = strdup(tok->u.identifier),
                            .has_type = false,
                        };
                        clape_arr_append(sizeof(clape_sum_variant_t), &variants, &v);
                        lhs = (clape_expr_t){
                            .tag = CLAPE_EXPR_TYPE,
                            .u.type_expr =
                                {
                                    .type = {.tag = CLAPE_TYPE_SUM,
                                        .u.sum = {.variants = variants}},
                                },
                        };
                    } else {
                        lhs = (clape_expr_t){
                            .tag = CLAPE_EXPR_IDENT,
                            .u.ident = strdup(tok->u.identifier),
                        };
                    }
                }
            }
            break;
        }
        default:
            throw_err(stderr, "Unexpected token in expression: ", tok, true);
            exit(1);
    }

    while (true) {
        token_type_e next = clape_peek(p)->tag;

        if (next == TOK_IF && CLAPE_BINDING_POWER_IF > min_bp) {
            clape_expr_t *then_ptr = malloc(sizeof(clape_expr_t));
            *then_ptr = lhs;
            clape_advance(p);
            clape_expr_t cond = clape_parse_expr(p, CLAPE_BINDING_POWER_IF);
            token_t *else_tok = clape_advance(p);
            if (else_tok->tag != TOK_ELSE) {
                fprintf(stderr, "Expected 'else' after if condition\n");
                exit(1);
            }
            clape_expr_t else_branch = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);
            clape_expr_t *cond_ptr = malloc(sizeof(clape_expr_t));
            *cond_ptr = cond;
            clape_expr_t *else_ptr = malloc(sizeof(clape_expr_t));
            *else_ptr = else_branch;
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_IF,
                .u.if_ = {.condition = cond_ptr, .then_branch = then_ptr, .else_branch = else_ptr},
            };
            continue;
        }

        clape_binding_power_t cur_bp = clape_infix_bp(next);

        if (cur_bp <= min_bp && CLAPE_BINDING_POWER_CALL > min_bp &&
            (next == TOK_INT || next == TOK_FLOAT || next == TOK_TRUE || next == TOK_FALSE ||
                next == TOK_NOT || next == TOK_LPAREN || next == TOK_LBRACE ||
                next == TOK_LBRACKET || next == TOK_MATCH || next == TOK_IDENTIFIER ||
                next == TOK_STRING || next == TOK_CHAR || next == TOK_DOT || next == TOK_TYPE_INT ||
                next == TOK_TYPE_FLOAT || next == TOK_TYPE_BOOL || next == TOK_TYPE_STRING ||
                next == TOK_TYPE_CHAR || next == TOK_TYPE_UNIT)) {
            cur_bp = CLAPE_BINDING_POWER_CALL;
            clape_expr_t arg = clape_parse_expr(p, cur_bp);
            clape_expr_t *callee_ptr = malloc(sizeof(clape_expr_t));
            *callee_ptr = lhs;
            clape_expr_t *arg_ptr = malloc(sizeof(clape_expr_t));
            *arg_ptr = arg;
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_CALL,
                .u.call = {.callee = callee_ptr, .arg = arg_ptr},
            };
            continue;
        } else if (cur_bp <= min_bp) {
            break;
        }
        if (next == TOK_DOT) {
            token_t *lookahead = ACCESS_ARR_AT(token_t, p->tokens, p->pos + 1);
            if (lookahead->tag == TOK_IDENTIFIER && islower(lookahead->u.identifier[0])) {
                // Field access: expr.field_name
                clape_advance(p);
                token_t *field_tok = clape_advance(p);
                clape_expr_t *expr_ptr = malloc(sizeof(clape_expr_t));
                *expr_ptr = lhs;
                lhs = (clape_expr_t){
                    .tag = CLAPE_EXPR_FIELD_ACCESS,
                    .u.field_access = {.expr = expr_ptr, .name = strdup(field_tok->u.identifier)},
                };
                continue;
            }
            // Uppercase after DOT → consume DOT, parse sum constructor as call arg
            clape_advance(p);
            token_t *ctor_tok = clape_advance(p);
            char *constructor = strdup(ctor_tok->u.identifier);
            clape_expr_t *arg = malloc(sizeof(clape_expr_t));
            if (clape_peek(p)->tag == TOK_LPAREN) {
                clape_advance(p);
                clape_expr_t *val = malloc(sizeof(clape_expr_t));
                *val = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);
                token_t *close = clape_advance(p);
                if (close->tag != TOK_RPAREN) {
                    fprintf(stderr, "Expected ')' after sum constructor value\n");
                    exit(1);
                }
                *arg = (clape_expr_t){
                    .tag = CLAPE_EXPR_SUM,
                    .u.sum = {.constructor = constructor, .expr = val},
                };
            } else {
                *arg = (clape_expr_t){
                    .tag = CLAPE_EXPR_SUM,
                    .u.sum = {.constructor = constructor, .expr = NULL},
                };
            }
            clape_expr_t *callee_ptr = malloc(sizeof(clape_expr_t));
            *callee_ptr = lhs;
            lhs = (clape_expr_t){
                .tag = CLAPE_EXPR_CALL,
                .u.call = {.callee = callee_ptr, .arg = arg},
            };
            continue;
        }
        token_t *op_tok = clape_advance(p);
        clape_binop_e op;
        bool is_cons = false;
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
            case TOK_CONS:
                op = CLAPE_BINOP_CONS;
                cur_bp = CLAPE_BINDING_POWER_CONS;
                is_cons = true;
                break;
            case TOK_AMP:
                if (lhs.tag == CLAPE_EXPR_TYPE) {
                    // Parse-time type combine: lhs & rhs are product types
                    clape_expr_t rhs = clape_parse_expr(p, CLAPE_BINDING_POWER_FACTOR);
                    if (rhs.tag != CLAPE_EXPR_TYPE) {
                        fprintf(stderr, "Expected type expression after '&'\n");
                        exit(1);
                    }
                    clape_arr_t *combined = NULL;
                    if (lhs.u.type_expr.type.tag == CLAPE_TYPE_PRODUCT) {
                        combined = lhs.u.type_expr.type.u.product.fields;
                        lhs.u.type_expr.type.u.product.fields = NULL;
                    }
                    if (rhs.u.type_expr.type.tag == CLAPE_TYPE_PRODUCT) {
                        const size_t field_count = rhs.u.type_expr.type.u.product.fields->len;
                        for (size_t i = 0; i < field_count; i++) {
                            clape_product_field_t *const src = ACCESS_ARR_AT(                   //
                                clape_product_field_t, rhs.u.type_expr.type.u.product.fields, i //
                            );
                            bool found = false;
                            if (combined) {
                                for (size_t j = 0; j < combined->len; j++) {
                                    clape_product_field_t *const dest = ACCESS_ARR_AT( //
                                        clape_product_field_t, combined, j             //
                                    );
                                    if (strcmp(dest->name, src->name) != 0) {
                                        continue;
                                    }
                                    free(dest->name);
                                    clape_free_type(&dest->type);
                                    dest->name = strdup(src->name);
                                    dest->type = clape_type_clone(&src->type);
                                    found = true;
                                    break;
                                }
                            }
                            if (found) {
                                continue;
                            }
                            if (!combined) {
                                combined = clape_arr_create(sizeof(clape_product_field_t), 0);
                            }
                            clape_product_field_t field = {
                                .name = strdup(src->name),
                                .type = clape_type_clone(&src->type),
                            };
                            clape_arr_append(sizeof(clape_product_field_t), &combined, &field);
                        }
                    }
                    if (rhs.u.type_expr.type.u.product.fields) {
                        for (size_t i = 0; i < rhs.u.type_expr.type.u.product.fields->len; i++) {
                            clape_product_field_t *const field = ACCESS_ARR_AT(                 //
                                clape_product_field_t, rhs.u.type_expr.type.u.product.fields, i //
                            );
                            free(field->name);
                            clape_free_type(&field->type);
                        }
                        free(rhs.u.type_expr.type.u.product.fields);
                    }
                    lhs = (clape_expr_t){
                        .tag = CLAPE_EXPR_TYPE,
                        .u.type_expr =
                            {
                                .type = {.tag = CLAPE_TYPE_PRODUCT,
                                    .u.product = {.fields = combined}},
                            },
                    };
                    continue;
                }
                op = CLAPE_BINOP_PROD;
                cur_bp = CLAPE_BINDING_POWER_FACTOR;
                break;
            case TOK_PIPE:
                if (lhs.tag != CLAPE_EXPR_TYPE || lhs.u.type_expr.type.tag != CLAPE_TYPE_SUM) {
                    fprintf(stderr, "'|' requires sum types\n");
                    exit(1);
                }
                clape_expr_t rhs_pipe = clape_parse_expr(p, CLAPE_BINDING_POWER_FACTOR);
                if (rhs_pipe.tag != CLAPE_EXPR_TYPE ||
                    rhs_pipe.u.type_expr.type.tag != CLAPE_TYPE_SUM) {
                    fprintf(stderr, "Expected sum type after '|'\n");
                    exit(1);
                }
                clape_arr_t *combined_sum = lhs.u.type_expr.type.u.sum.variants;
                lhs.u.type_expr.type.u.sum.variants = NULL;
                for (size_t i = 0; i < rhs_pipe.u.type_expr.type.u.sum.variants->len; i++) {
                    clape_sum_variant_t *const src = ACCESS_ARR_AT(                      //
                        clape_sum_variant_t, rhs_pipe.u.type_expr.type.u.sum.variants, i //
                    );
                    if (combined_sum) {
                        for (size_t j = 0; j < combined_sum->len; j++) {
                            clape_sum_variant_t *const dest = ACCESS_ARR_AT( //
                                clape_sum_variant_t, combined_sum, j         //
                            );
                            if (strcmp(dest->name, src->name) != 0) {
                                continue;
                            }
                            fprintf(stderr, "Duplicate constructor '%s' in sum type\n", src->name);
                            exit(1);
                        }
                    }
                    if (!combined_sum) {
                        combined_sum = clape_arr_create(sizeof(clape_sum_variant_t), 0);
                    }
                    clape_sum_variant_t v = {
                        .name = strdup(src->name),
                        .has_type = src->has_type,
                    };
                    if (src->has_type)
                        v.type = clape_type_clone(&src->type);
                    clape_arr_append(sizeof(clape_sum_variant_t), &combined_sum, &v);
                }
                if (rhs_pipe.u.type_expr.type.u.sum.variants) {
                    for (size_t i = 0; i < rhs_pipe.u.type_expr.type.u.sum.variants->len; i++) {
                        clape_sum_variant_t *const variant = ACCESS_ARR_AT(                  //
                            clape_sum_variant_t, rhs_pipe.u.type_expr.type.u.sum.variants, i //
                        );
                        free(variant->name);
                        if (variant->has_type) {
                            clape_free_type(&variant->type);
                        }
                    }
                    free(rhs_pipe.u.type_expr.type.u.sum.variants);
                }
                lhs = (clape_expr_t){
                    .tag = CLAPE_EXPR_TYPE,
                    .u.type_expr =
                        {
                            .type = {.tag = CLAPE_TYPE_SUM, .u.sum = {.variants = combined_sum}},
                        },
                };
                continue;
            default:
                fprintf(stderr, "Unexpected infix operator\n");
                exit(1);
        }
        clape_expr_t rhs = clape_parse_expr(p, is_cons ? cur_bp - 1 : cur_bp);
        clape_expr_t *lhs_ptr = malloc(sizeof(clape_expr_t));
        *lhs_ptr = lhs;
        clape_expr_t *rhs_ptr = malloc(sizeof(clape_expr_t));
        *rhs_ptr = rhs;
        lhs = (clape_expr_t){
            .tag = CLAPE_EXPR_BINOP,
            .u.binop = {.op = op, .lhs = lhs_ptr, .rhs = rhs_ptr},
        };
    }
    return lhs;
}

static clape_stmt_t clape_parse_stmt(clape_parser_t *p) {
    token_t *first = clape_advance(p);
    if (first->tag == TOK_USE) {
        token_t *mod_tok = clape_advance(p);
        return (clape_stmt_t){
            .tag = CLAPE_STMT_USE,
            .u.use = {.module = strdup(mod_tok->u.identifier)},
        };
    }
    token_t *name_tok = clape_advance(p);

    // Parse optional generic params: <T, U, ...>
    clape_arr_t *generic_params = NULL;
    if (clape_peek(p)->tag == TOK_LT) {
        if (strcmp(name_tok->u.identifier, "_") == 0) {
            fprintf(stderr, "Anonymous bindings cannot have generic parameters\n");
            exit(1);
        }
        generic_params = clape_arr_create(sizeof(char *), 0);
        clape_advance(p); // consume <
        while (true) {
            token_t *const gen_tok = clape_advance(p);
            if (gen_tok->tag != TOK_IDENTIFIER) {
                fprintf(stderr, "Expected generic parameter name\n");
                exit(1);
            }
            char *gen_name = strdup(gen_tok->u.identifier);
            clape_arr_append(sizeof(char *), &generic_params, &gen_name);
            // Push onto generic_names array for RHS parsing
            if (!p->generic_names) {
                p->generic_names = clape_arr_create(sizeof(char *), 0);
            }
            char *gen_name_copy = strdup(gen_tok->u.identifier);
            clape_arr_append(sizeof(char *), &p->generic_names, &gen_name_copy);
            if (clape_peek(p)->tag == TOK_COMMA) {
                clape_advance(p);
            } else {
                break;
            }
        }
        token_t *const close = clape_advance(p);
        if (close->tag != TOK_GT) {
            fprintf(stderr, "Expected '>' after generic parameters\n");
            exit(1);
        }
    }

    // Consume the `=`
    clape_advance(p);

    clape_expr_t rhs = clape_parse_expr(p, CLAPE_BINDING_POWER_DEFAULT);

    // Pop generic_names
    if (p->generic_names) {
        for (size_t gi = 0; gi < p->generic_names->len; gi++) {
            free(*(char **)ACCESS_ARR_AT(char *, p->generic_names, gi));
        }
        free(p->generic_names);
        p->generic_names = NULL;
    }

    // If the RHS is a type expression, register a type binding
    if (rhs.tag == CLAPE_EXPR_TYPE) {
        clape_type_t type_body = rhs.u.type_expr.type;
        rhs.u.type_expr.type = (clape_type_t){.tag = CLAPE_TYPE_UNIT};
        struct clape_type_binding *const binding = malloc(sizeof(struct clape_type_binding));
        binding->name = strdup(name_tok->u.identifier);
        binding->generic_params = generic_params;
        binding->type_body = type_body;
        binding->next = p->type_env;
        p->type_env = binding;
        clape_expr_e tag = CLAPE_EXPR_PRODUCT_TYPE;
        if (type_body.tag == CLAPE_TYPE_SUM) {
            tag = CLAPE_EXPR_SUM_TYPE;
        }
        return (clape_stmt_t){
            .tag = CLAPE_STMT_LET,
            .u.let =
                {
                    .name = strdup(name_tok->u.identifier),
                    .expr = {.tag = tag, .u.product_type = {.fields = NULL}},
                },
        };
    }

    // For function bindings with generic params, attach them to the lambda expression
    if (generic_params != NULL && rhs.tag == CLAPE_EXPR_LAMBDA) {
        rhs.u.lambda.generic_params = generic_params;
    } else if (generic_params != NULL) {
        fprintf(stderr, "Generic parameters are only allowed on type and function definitions\n");
        exit(1);
    }

    clape_stmt_t stmt = {
        .tag = CLAPE_STMT_LET,
        .u.let =
            {
                .name = strdup(name_tok->u.identifier),
                .expr = rhs,
            },
    };
    return stmt;
}

static clape_expr_t clape_parse_block_body(clape_parser_t *p) {
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
    token_t *tok = clape_advance(p);
    if (tok->tag != TOK_RBRACE) {
        throw_err(stderr, "Expected '}' but got: ", tok, true);
        exit(1);
    }
    return (clape_expr_t){
        .tag = CLAPE_EXPR_BLOCK,
        .u.block = {.stmts = stmts, .return_expr = ret_expr},
    };
}

static clape_expr_t clape_parse_block(clape_parser_t *p) {
    token_t *tok = clape_advance(p);
    if (tok->tag != TOK_LBRACE) {
        throw_err(stderr, "Expected '{' but got: ", tok, true);
        exit(1);
    }
    return clape_parse_block_body(p);
}

// ----- Public API -----

bool clape_parse(clape_program_t *const program, clape_arr_t *const tokens) {
    clape_parser_t p = {.tokens = tokens, .pos = 0};
    program->statements = clape_arr_create(sizeof(clape_stmt_t), 0);
    p.generic_names = NULL;
    while (clape_peek(&p)->tag != TOK_EOF) {
        token_t *const tok = clape_peek(&p);
        if (tok->tag != TOK_LET && tok->tag != TOK_USE) {
            throw_err(stderr, "Expected 'let' or 'use' at start of statement, got: ", tok, true);
            return false;
        }
        clape_stmt_t stmt = clape_parse_stmt(&p);
        clape_arr_append(sizeof(clape_stmt_t), &program->statements, &stmt);
        while (clape_peek(&p)->tag == TOK_SEMICOLON) {
            clape_advance(&p);
        }
    }
    // Free generic names (should be empty at this point, but be safe)
    if (p.generic_names) {
        for (size_t i = 0; i < p.generic_names->len; i++) {
            free(*ACCESS_ARR_AT(char *, p.generic_names, i));
        }
        free(p.generic_names);
    }
    // Free type env
    struct clape_type_binding *binding = p.type_env;
    while (binding != NULL) {
        struct clape_type_binding *const next = binding->next;
        free(binding->name);
        if (binding->generic_params) {
            for (size_t i = 0; i < binding->generic_params->len; i++) {
                free(*ACCESS_ARR_AT(char *, binding->generic_params, i));
            }
            free(binding->generic_params);
        }
        clape_free_type(&binding->type_body);
        free(binding);
        binding = next;
    }
    return true;
}

static void clape_print_type(FILE *stream, clape_type_t *type) {
    switch (type->tag) {
        case CLAPE_TYPE_INT:
            fprintf(stream, "Int");
            break;
        case CLAPE_TYPE_FLOAT:
            fprintf(stream, "Float");
            break;
        case CLAPE_TYPE_BOOL:
            fprintf(stream, "Bool");
            break;
        case CLAPE_TYPE_STRING:
            fprintf(stream, "String");
            break;
        case CLAPE_TYPE_CHAR:
            fprintf(stream, "Char");
            break;
        case CLAPE_TYPE_UNIT:
            fprintf(stream, "Unit");
            break;
        case CLAPE_TYPE_LIST:
            fprintf(stream, "[T]");
            break;
        case CLAPE_TYPE_FUNC:
            fprintf(stream, "(");
            clape_print_type(stream, type->u.func.param);
            fprintf(stream, " -> ");
            clape_print_type(stream, type->u.func.ret);
            fprintf(stream, ")");
            break;
        case CLAPE_TYPE_GENERIC:
            fprintf(stream, "%s", type->u.generic);
            break;
        case CLAPE_TYPE_PRODUCT:
            for (size_t i = 0; i < type->u.product.fields->len; i++) {
                if (i > 0) {
                    fprintf(stream, " & ");
                }
                clape_product_field_t *f =
                    ACCESS_ARR_AT(clape_product_field_t, type->u.product.fields, i);
                fprintf(stream, "%s(", f->name);
                clape_print_type(stream, &f->type);
                fprintf(stream, ")");
            }
            break;
        case CLAPE_TYPE_SUM:
            for (size_t i = 0; i < type->u.sum.variants->len; i++) {
                if (i > 0) {
                    fprintf(stream, " | ");
                }
                clape_sum_variant_t *const variant = ACCESS_ARR_AT( //
                    clape_sum_variant_t, type->u.sum.variants, i    //
                );
                fprintf(stream, "%s", variant->name);
                if (variant->has_type) {
                    fprintf(stream, "(");
                    clape_print_type(stream, &variant->type);
                    fprintf(stream, ")");
                }
            }
            break;
    }
}

/// @brief Walk a type tree and substitute `CLAPE_TYPE_GENERIC` nodes with their concrete types
/// from the generic binding environment. Returns a new heap-allocated type; the caller owns it.
static clape_type_t clape_type_substitute(const clape_type_t *type,
    const clape_generic_binding_t *genv) {
    switch (type->tag) {
        case CLAPE_TYPE_GENERIC: {
            for (const clape_generic_binding_t *b = genv; b; b = b->next) {
                if (strcmp(b->name, type->u.generic) == 0) {
                    return clape_type_clone(&b->type);
                }
            }
            return clape_type_clone(type);
        }
        case CLAPE_TYPE_PRODUCT: {
            clape_arr_t *fields = NULL;
            if (type->u.product.fields) {
                fields = clape_arr_create(sizeof(clape_product_field_t), 0);
                for (size_t i = 0; i < type->u.product.fields->len; i++) {
                    clape_product_field_t *const src = ACCESS_ARR_AT(    //
                        clape_product_field_t, type->u.product.fields, i //
                    );
                    clape_product_field_t f = {
                        .name = strdup(src->name),
                        .type = clape_type_substitute(&src->type, genv),
                    };
                    clape_arr_append(sizeof(clape_product_field_t), &fields, &f);
                }
            }
            return (clape_type_t){
                .tag = CLAPE_TYPE_PRODUCT,
                .u.product = {.fields = fields},
            };
        }
        case CLAPE_TYPE_SUM: {
            clape_arr_t *variants = NULL;
            if (type->u.sum.variants) {
                variants = clape_arr_create(sizeof(clape_sum_variant_t), 0);
                for (size_t i = 0; i < type->u.sum.variants->len; i++) {
                    clape_sum_variant_t *const src = ACCESS_ARR_AT(  //
                        clape_sum_variant_t, type->u.sum.variants, i //
                    );
                    clape_sum_variant_t v = {
                        .name = strdup(src->name),
                        .has_type = src->has_type,
                    };
                    if (src->has_type) {
                        v.type = clape_type_substitute(&src->type, genv);
                    }
                    clape_arr_append(sizeof(clape_sum_variant_t), &variants, &v);
                }
            }
            return (clape_type_t){
                .tag = CLAPE_TYPE_SUM,
                .u.sum = {.variants = variants},
            };
        }
        case CLAPE_TYPE_LIST:
            if (type->u.element) {
                clape_type_t *const inner = malloc(sizeof(clape_type_t));
                *inner = clape_type_substitute(type->u.element, genv);
                return (clape_type_t){.tag = CLAPE_TYPE_LIST, .u.element = inner};
            }
            return (clape_type_t){.tag = CLAPE_TYPE_LIST, .u.element = NULL};

        case CLAPE_TYPE_FUNC: {
            clape_type_t *const param = malloc(sizeof(clape_type_t));
            *param = clape_type_substitute(type->u.func.param, genv);
            clape_type_t *const ret = malloc(sizeof(clape_type_t));
            *ret = clape_type_substitute(type->u.func.ret, genv);
            return (clape_type_t){
                .tag = CLAPE_TYPE_FUNC,
                .u.func = {.param = param, .ret = ret},
            };
        }
        default:
            return clape_type_clone(type);
    }
}

static void clape_print_expr(FILE *stream, clape_expr_t *expr) {
    switch (expr->tag) {
        case CLAPE_EXPR_LIT:
            switch (expr->u.lit.type.tag) {
                case CLAPE_TYPE_INT:
                    fprintf(stream, "%li", expr->u.lit.u.ival);
                    break;
                case CLAPE_TYPE_FLOAT:
                    fprintf(stream, "%g", expr->u.lit.u.fval);
                    break;
                case CLAPE_TYPE_BOOL:
                    fprintf(stream, "%s", expr->u.lit.u.bval ? "true" : "false");
                    break;
                case CLAPE_TYPE_STRING:
                    fprintf(stream, "\"%s\"", expr->u.lit.u.sval);
                    break;
                case CLAPE_TYPE_CHAR:
                    fprintf(stream, "'%c'", expr->u.lit.u.cval);
                    break;
                default:
                    fprintf(stream, "<value>");
                    break;
            }
            break;
        case CLAPE_EXPR_UNARY:
            fprintf(stream, "(not ");
            clape_print_expr(stream, expr->u.unary.operand);
            fprintf(stream, ")");
            break;
        case CLAPE_EXPR_IDENT:
            fprintf(stream, "%s", expr->u.ident);
            break;
        case CLAPE_EXPR_BINOP: {
            const char *op_str;
            switch (expr->u.binop.op) {
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
                case CLAPE_BINOP_CONS:
                    op_str = "::";
                    break;
                case CLAPE_BINOP_PROD:
                    op_str = "&";
                    break;
            }
            fprintf(stream, "(");
            clape_print_expr(stream, expr->u.binop.lhs);
            fprintf(stream, " %s ", op_str);
            clape_print_expr(stream, expr->u.binop.rhs);
            fprintf(stream, ")");
            break;
        }
        case CLAPE_EXPR_CALL:
            fprintf(stream, "(");
            clape_print_expr(stream, expr->u.call.callee);
            fprintf(stream, " ");
            clape_print_expr(stream, expr->u.call.arg);
            fprintf(stream, ")");
            break;
        case CLAPE_EXPR_LAMBDA:
            fprintf(stream, "<lambda>");
            break;
        case CLAPE_EXPR_IF:
            fprintf(stream, "(");
            clape_print_expr(stream, expr->u.if_.then_branch);
            fprintf(stream, " if ");
            clape_print_expr(stream, expr->u.if_.condition);
            fprintf(stream, " else ");
            clape_print_expr(stream, expr->u.if_.else_branch);
            fprintf(stream, ")");
            break;
        case CLAPE_EXPR_BLOCK:
            fprintf(stream, "<block>");
            break;
        case CLAPE_EXPR_LIST:
            fprintf(stream, "[");
            for (size_t i = 0; i < expr->u.lst.elements->len; i++) {
                if (i > 0) {
                    fprintf(stream, ", ");
                }
                clape_print_expr(stream, ACCESS_ARR_AT(clape_expr_t, expr->u.lst.elements, i));
            }
            fprintf(stream, "]");
            break;
        case CLAPE_EXPR_MATCH:
            fprintf(stream, "(match ");
            clape_print_expr(stream, expr->u.match_.scrutinee);
            fprintf(stream, " { ... })");
            break;
        case CLAPE_EXPR_PRODUCT_TYPE:
            fprintf(stream, "<product-type>");
            break;
        case CLAPE_EXPR_SUM_TYPE:
            fprintf(stream, "<sum-type>");
            break;
        case CLAPE_EXPR_SUM:
            fprintf(stream, ".%s", expr->u.sum.constructor);
            if (expr->u.sum.expr) {
                fprintf(stream, "(");
                clape_print_expr(stream, expr->u.sum.expr);
                fprintf(stream, ")");
            }
            break;
        case CLAPE_EXPR_FIELD:
            fprintf(stream, ".%s(", expr->u.field.name);
            clape_print_expr(stream, expr->u.field.value);
            fprintf(stream, ")");
            break;
        case CLAPE_EXPR_FIELD_ACCESS:
            fprintf(stream, "(");
            clape_print_expr(stream, expr->u.field_access.expr);
            fprintf(stream, ".%s)", expr->u.field_access.name);
            break;
        case CLAPE_EXPR_TYPE:
            clape_print_type(stream, &expr->u.type_expr.type);
            break;
    }
}

void clape_print_program(clape_program_t *const program) {
    for (size_t i = 0; i < program->statements->len; i++) {
        clape_stmt_t *stmt = ACCESS_ARR_AT(clape_stmt_t, program->statements, i);
        if (stmt->tag == CLAPE_STMT_LET) {
            fprintf(stdout, "(let %s = ", stmt->u.let.name);
            clape_print_expr(stdout, &stmt->u.let.expr);
            fprintf(stdout, ")\n");
        } else {
            fprintf(stdout, "(use %s)\n", stmt->u.use.module);
        }
    }
}

static void clape_free_pattern(clape_pattern_t *pat);

static clape_type_t clape_type_clone(const clape_type_t *const type) {
    switch (type->tag) {
        case CLAPE_TYPE_PRODUCT: {
            clape_arr_t *fields = NULL;
            if (type->u.product.fields) {
                fields = clape_arr_create(sizeof(clape_product_field_t), 0);
                for (size_t i = 0; i < type->u.product.fields->len; i++) {
                    clape_product_field_t *const src = ACCESS_ARR_AT(    //
                        clape_product_field_t, type->u.product.fields, i //
                    );
                    clape_product_field_t f = {
                        .name = strdup(src->name),
                        .type = clape_type_clone(&src->type),
                    };
                    clape_arr_append(sizeof(clape_product_field_t), &fields, &f);
                }
            }
            return (clape_type_t){
                .tag = CLAPE_TYPE_PRODUCT,
                .u.product = {.fields = fields},
            };
        }
        case CLAPE_TYPE_SUM: {
            clape_arr_t *variants = NULL;
            if (type->u.sum.variants) {
                variants = clape_arr_create(sizeof(clape_sum_variant_t), 0);
                for (size_t i = 0; i < type->u.sum.variants->len; i++) {
                    clape_sum_variant_t *const src = ACCESS_ARR_AT(  //
                        clape_sum_variant_t, type->u.sum.variants, i //
                    );
                    clape_sum_variant_t v = {
                        .name = strdup(src->name),
                        .has_type = src->has_type,
                    };
                    if (src->has_type) {
                        v.type = clape_type_clone(&src->type);
                    }
                    clape_arr_append(sizeof(clape_sum_variant_t), &variants, &v);
                }
            }
            return (clape_type_t){
                .tag = CLAPE_TYPE_SUM,
                .u.sum = {.variants = variants},
            };
        }
        case CLAPE_TYPE_FUNC: {
            clape_type_t *const param = malloc(sizeof(clape_type_t));
            *param = clape_type_clone(type->u.func.param);
            clape_type_t *const ret = malloc(sizeof(clape_type_t));
            *ret = clape_type_clone(type->u.func.ret);
            return (clape_type_t){
                .tag = CLAPE_TYPE_FUNC,
                .u.func = {.param = param, .ret = ret},
            };
        }
        case CLAPE_TYPE_LIST:
            if (type->u.element) {
                clape_type_t *const inner = malloc(sizeof(clape_type_t));
                *inner = clape_type_clone(type->u.element);
                return (clape_type_t){.tag = CLAPE_TYPE_LIST, .u.element = inner};
            }
            return (clape_type_t){.tag = CLAPE_TYPE_LIST, .u.element = NULL};

        case CLAPE_TYPE_GENERIC:
            return (clape_type_t){
                .tag = CLAPE_TYPE_GENERIC,
                .u.generic = strdup(type->u.generic),
            };

        default:
            return (clape_type_t){.tag = type->tag};
    }
}

static void clape_free_type(clape_type_t *const type) {
    switch (type->tag) {
        case CLAPE_TYPE_PRODUCT:
            if (type->u.product.fields) {
                for (size_t i = 0; i < type->u.product.fields->len; i++) {
                    clape_product_field_t *f = ACCESS_ARR_AT(            //
                        clape_product_field_t, type->u.product.fields, i //
                    );
                    free(f->name);
                    clape_free_type(&f->type);
                }
                free(type->u.product.fields);
            }
            break;
        case CLAPE_TYPE_SUM:
            if (type->u.sum.variants) {
                for (size_t i = 0; i < type->u.sum.variants->len; i++) {
                    clape_sum_variant_t *const variant = ACCESS_ARR_AT( //
                        clape_sum_variant_t, type->u.sum.variants, i    //
                    );
                    free(variant->name);
                    if (variant->has_type) {
                        clape_free_type(&variant->type);
                    }
                }
                free(type->u.sum.variants);
            }
            break;
        case CLAPE_TYPE_FUNC:
            clape_free_type(type->u.func.param);
            free(type->u.func.param);
            clape_free_type(type->u.func.ret);
            free(type->u.func.ret);
            break;
        case CLAPE_TYPE_LIST:
            if (type->u.element) {
                clape_free_type(type->u.element);
                free(type->u.element);
            }
            break;
        case CLAPE_TYPE_GENERIC:
            free(type->u.generic);
            break;
        default:
            break;
    }
}

static void clape_free_expr(clape_expr_t *expr) {
    switch (expr->tag) {
        case CLAPE_EXPR_LIT:
            if (expr->u.lit.type.tag == CLAPE_TYPE_STRING) {
                free(expr->u.lit.u.sval);
            }
            break;
        case CLAPE_EXPR_IDENT:
            free(expr->u.ident);
            break;
        case CLAPE_EXPR_UNARY:
            clape_free_expr(expr->u.unary.operand);
            free(expr->u.unary.operand);
            break;
        case CLAPE_EXPR_BINOP:
            clape_free_expr(expr->u.binop.lhs);
            clape_free_expr(expr->u.binop.rhs);
            free(expr->u.binop.lhs);
            free(expr->u.binop.rhs);
            break;
        case CLAPE_EXPR_CALL:
            clape_free_expr(expr->u.call.callee);
            clape_free_expr(expr->u.call.arg);
            free(expr->u.call.callee);
            free(expr->u.call.arg);
            break;
        case CLAPE_EXPR_LAMBDA:
            for (size_t i = 0; i < expr->u.lambda.params->len; i++) {
                free(ACCESS_ARR_AT(clape_param_t, expr->u.lambda.params, i)->name);
            }
            free(expr->u.lambda.params);
            clape_free_type(&expr->u.lambda.return_type);
            if (expr->u.lambda.generic_params != NULL) {
                for (size_t i = 0; i < expr->u.lambda.generic_params->len; i++) {
                    free(*ACCESS_ARR_AT(char *, expr->u.lambda.generic_params, i));
                }
                free(expr->u.lambda.generic_params);
            }
            clape_free_expr(expr->u.lambda.body);
            free(expr->u.lambda.body);
            break;
        case CLAPE_EXPR_IF:
            clape_free_expr(expr->u.if_.condition);
            free(expr->u.if_.condition);
            clape_free_expr(expr->u.if_.then_branch);
            free(expr->u.if_.then_branch);
            clape_free_expr(expr->u.if_.else_branch);
            free(expr->u.if_.else_branch);
            break;
        case CLAPE_EXPR_BLOCK:
            for (size_t i = 0; i < expr->u.block.stmts->len; i++) {
                clape_stmt_t *s = ACCESS_ARR_AT(clape_stmt_t, expr->u.block.stmts, i);
                free(s->u.let.name);
                clape_free_expr(&s->u.let.expr);
            }
            free(expr->u.block.stmts);
            if (expr->u.block.return_expr) {
                clape_free_expr(expr->u.block.return_expr);
                free(expr->u.block.return_expr);
            }
            break;
        case CLAPE_EXPR_LIST:
            for (size_t i = 0; i < expr->u.lst.elements->len; i++) {
                clape_expr_t *e = ACCESS_ARR_AT(clape_expr_t, expr->u.lst.elements, i);
                clape_free_expr(e);
            }
            free(expr->u.lst.elements);
            break;
        case CLAPE_EXPR_MATCH:
            clape_free_expr(expr->u.match_.scrutinee);
            free(expr->u.match_.scrutinee);
            for (size_t i = 0; i < expr->u.match_.arms->len; i++) {
                clape_match_arm_t *arm = ACCESS_ARR_AT(       //
                    clape_match_arm_t, expr->u.match_.arms, i //
                );
                clape_free_pattern(&arm->pattern);
                clape_free_expr(&arm->body);
            }
            free(expr->u.match_.arms);
            break;
        case CLAPE_EXPR_PRODUCT_TYPE:
            if (expr->u.product_type.fields) {
                for (size_t i = 0; i < expr->u.product_type.fields->len; i++) {
                    clape_product_field_t *f = ACCESS_ARR_AT(                 //
                        clape_product_field_t, expr->u.product_type.fields, i //
                    );
                    free(f->name);
                }
                free(expr->u.product_type.fields);
            }
            break;
        case CLAPE_EXPR_SUM_TYPE:
            if (expr->u.sum_type.variants) {
                for (size_t i = 0; i < expr->u.sum_type.variants->len; i++) {
                    clape_sum_variant_t *const variant = ACCESS_ARR_AT(   //
                        clape_sum_variant_t, expr->u.sum_type.variants, i //
                    );
                    free(variant->name);
                    if (variant->has_type) {
                        clape_free_type(&variant->type);
                    }
                }
                free(expr->u.sum_type.variants);
            }
            break;
        case CLAPE_EXPR_SUM:
            free(expr->u.sum.constructor);
            if (expr->u.sum.expr) {
                clape_free_expr(expr->u.sum.expr);
                free(expr->u.sum.expr);
            }
            break;
        case CLAPE_EXPR_FIELD:
            free(expr->u.field.name);
            clape_free_expr(expr->u.field.value);
            free(expr->u.field.value);
            break;
        case CLAPE_EXPR_FIELD_ACCESS:
            clape_free_expr(expr->u.field_access.expr);
            free(expr->u.field_access.expr);
            free(expr->u.field_access.name);
            break;
        case CLAPE_EXPR_TYPE:
            clape_free_type(&expr->u.type_expr.type);
            break;
    }
}

static void clape_free_pattern(clape_pattern_t *pat) {
    switch (pat->tag) {
        case CLAPE_PATTERN_ANY:
        case CLAPE_PATTERN_EMPTY_LIST:
            break;
        case CLAPE_PATTERN_VARIABLE:
            free(pat->u.variable);
            break;
        case CLAPE_PATTERN_LIT:
            if (pat->u.lit.type.tag == CLAPE_TYPE_STRING) {
                free(pat->u.lit.u.sval);
            }
            break;
        case CLAPE_PATTERN_CONS:
            clape_free_pattern(pat->u.cons.head);
            clape_free_pattern(pat->u.cons.tail);
            free(pat->u.cons.head);
            free(pat->u.cons.tail);
            break;
        case CLAPE_PATTERN_SUM:
            free(pat->u.sum.constructor);
            if (pat->u.sum.var) {
                free(pat->u.sum.var);
            }
            break;
    }
}

void clape_free_program(clape_program_t *const program) {
    if (!program->statements) {
        return;
    }
    for (size_t i = 0; i < program->statements->len; i++) {
        clape_stmt_t *const stmt = ACCESS_ARR_AT(clape_stmt_t, program->statements, i);
        if (stmt->tag == CLAPE_STMT_LET) {
            free(stmt->u.let.name);
            clape_free_expr(&stmt->u.let.expr);
        } else {
            free(stmt->u.use.module);
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

static clape_env_t *clape_match_value(clape_value_t val, clape_pattern_t *pat, clape_env_t *env) {
    switch (pat->tag) {
        case CLAPE_PATTERN_ANY:
            return env;
        case CLAPE_PATTERN_VARIABLE: {
            clape_env_t *binding = malloc(sizeof(clape_env_t));
            *binding = (clape_env_t){
                .name = strdup(pat->u.variable),
                .value = val,
                .next = env,
            };
            return binding;
        }
        case CLAPE_PATTERN_EMPTY_LIST:
            if ((val.type.tag == CLAPE_TYPE_LIST && val.u.list == NULL) ||
                (val.type.tag == CLAPE_TYPE_STRING && strlen(val.u.sval) == 0)) {
                return env;
            }
            return NULL;
        case CLAPE_PATTERN_LIT:
            if (val.type.tag != pat->u.lit.type.tag) {
                return NULL;
            }
            switch (val.type.tag) {
                case CLAPE_TYPE_INT:
                    return val.u.ival == pat->u.lit.u.ival ? env : NULL;
                case CLAPE_TYPE_FLOAT:
                    return val.u.fval == pat->u.lit.u.fval ? env : NULL;
                case CLAPE_TYPE_BOOL:
                    return val.u.bval == pat->u.lit.u.bval ? env : NULL;
                case CLAPE_TYPE_CHAR:
                    return val.u.cval == pat->u.lit.u.cval ? env : NULL;
                case CLAPE_TYPE_STRING:
                    return strcmp(val.u.sval, pat->u.lit.u.sval) == 0 ? env : NULL;
                default:
                    return NULL;
            }
        case CLAPE_PATTERN_SUM: {
            if (val.type.tag != CLAPE_TYPE_SUM)
                return NULL;
            if (strcmp(val.u.sum.constructor, pat->u.sum.constructor) != 0)
                return NULL;
            if (pat->u.sum.has_payload && !val.u.sum.value)
                return NULL;
            if (!pat->u.sum.has_payload && val.u.sum.value)
                return NULL;
            if (pat->u.sum.var) {
                clape_env_t *const binding = malloc(sizeof(clape_env_t));
                *binding = (clape_env_t){
                    .name = strdup(pat->u.sum.var),
                    .value = *val.u.sum.value,
                    .next = env,
                };
                return binding;
            }
            return env;
        }
        case CLAPE_PATTERN_CONS: {
            clape_value_t head_val;
            clape_value_t tail_val;
            if (val.type.tag == CLAPE_TYPE_LIST && val.u.list != NULL) {
                head_val = val.u.list->head;
                if (head_val.type.tag == CLAPE_TYPE_STRING) {
                    head_val.u.sval = strdup(head_val.u.sval);
                }
                if (head_val.type.tag == CLAPE_TYPE_LIST && head_val.u.list) {
                    for (clape_cons_t *n = head_val.u.list; n; n = n->tail) {
                        n->arc++;
                    }
                }
                tail_val = (clape_value_t){
                    .type = {.tag = CLAPE_TYPE_LIST},
                    .u.list = val.u.list->tail,
                };
                for (clape_cons_t *n = val.u.list->tail; n; n = n->tail) {
                    n->arc++;
                }
            } else if (val.type.tag == CLAPE_TYPE_STRING && strlen(val.u.sval) > 0) {
                head_val = (clape_value_t){
                    .type = {.tag = CLAPE_TYPE_CHAR},
                    .u.cval = val.u.sval[0],
                };
                tail_val = (clape_value_t){
                    .type = {.tag = CLAPE_TYPE_STRING},
                    .u.sval = strdup(val.u.sval + 1),
                };
            } else {
                return NULL;
            }
            clape_env_t *env1 = clape_match_value(head_val, pat->u.cons.head, env);
            if (env1 == NULL) {
                return NULL;
            }
            return clape_match_value(tail_val, pat->u.cons.tail, env1);
        }
    }
    return NULL;
}

static const char *clape_type_tag_name(clape_type_e tag) {
    switch (tag) {
        case CLAPE_TYPE_INT:
            return "Int";
        case CLAPE_TYPE_FLOAT:
            return "Float";
        case CLAPE_TYPE_BOOL:
            return "Bool";
        case CLAPE_TYPE_FUNC:
            return "Function";
        case CLAPE_TYPE_STRING:
            return "String";
        case CLAPE_TYPE_CHAR:
            return "Char";
        case CLAPE_TYPE_LIST:
            return "List";
        case CLAPE_TYPE_UNIT:
            return "Unit";
        case CLAPE_TYPE_PRODUCT:
            return "Product";
        case CLAPE_TYPE_SUM:
            return "Sum";
        case CLAPE_TYPE_GENERIC:
            return "Generic";
    }
    return "Unknown";
}

static const clape_generic_binding_t *clape_find_generic( //
    const clape_generic_binding_t *genv,                  //
    const char *name                                      //
) {
    for (; genv; genv = genv->next) {
        if (strcmp(genv->name, name) == 0) {
            return genv;
        }
    }
    return NULL;
}

static bool clape_type_is_compatible( //
    const clape_value_t *value,       //
    const clape_type_t *type,         //
    clape_generic_binding_t **genv    //
) {
    switch (type->tag) {
        case CLAPE_TYPE_GENERIC: {
            const clape_generic_binding_t *b =
                *genv ? clape_find_generic(*genv, type->u.generic) : NULL;
            if (b) {
                return clape_type_is_compatible(value, &b->type, genv);
            }
            // Lazily infer: bind generic to the value's concrete type
            *genv = clape_genv_prepend(*genv, type->u.generic, clape_type_clone(&value->type));
            return true;
        }
        case CLAPE_TYPE_INT:
        case CLAPE_TYPE_FLOAT:
        case CLAPE_TYPE_BOOL:
        case CLAPE_TYPE_STRING:
        case CLAPE_TYPE_CHAR:
        case CLAPE_TYPE_UNIT:
            return value->type.tag == type->tag;
        case CLAPE_TYPE_LIST:
            return value->type.tag == CLAPE_TYPE_LIST;
        case CLAPE_TYPE_FUNC:
            return value->type.tag == CLAPE_TYPE_FUNC;
        case CLAPE_TYPE_PRODUCT: {
            if (value->type.tag != CLAPE_TYPE_PRODUCT) {
                return false;
            }
            for (size_t i = 0; i < type->u.product.fields->len; i++) {
                clape_product_field_t *const declared = ACCESS_ARR_AT( //
                    clape_product_field_t, type->u.product.fields, i   //
                );
                bool found = false;
                for (size_t j = 0; j < value->u.product->len; j++) {
                    clape_product_value_t *const field = ACCESS_ARR_AT( //
                        clape_product_value_t, value->u.product, j      //
                    );
                    if (strcmp(field->name, declared->name) == 0) {
                        if (!clape_type_is_compatible(&field->value, &declared->type, genv)) {
                            return false;
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true;
        }
        case CLAPE_TYPE_SUM: {
            if (value->type.tag != CLAPE_TYPE_SUM) {
                return false;
            }
            for (size_t i = 0; i < type->u.sum.variants->len; i++) {
                clape_sum_variant_t *const variant = ACCESS_ARR_AT( //
                    clape_sum_variant_t, type->u.sum.variants, i    //
                );
                if (strcmp(variant->name, value->u.sum.constructor) != 0) {
                    continue;
                }
                if (variant->has_type && value->u.sum.value) {
                    return clape_type_is_compatible(value->u.sum.value, &variant->type, genv);
                }
                return variant->has_type == (value->u.sum.value != NULL);
            }
            return false;
        }
    }
    return false;
}

static clape_generic_binding_t *clape_genv_prepend( //
    clape_generic_binding_t *genv,                  //
    const char *name,                               //
    clape_type_t type                               //
) {
    clape_generic_binding_t *const binding = malloc(sizeof(clape_generic_binding_t));
    binding->name = strdup(name);
    binding->type = type;
    binding->next = genv;
    return binding;
}

static void clape_genv_free(clape_generic_binding_t *genv) {
    while (genv != NULL) {
        clape_generic_binding_t *const next = genv->next;
        free(genv->name);
        clape_free_type(&genv->type);
        free(genv);
        genv = next;
    }
}

static clape_generic_binding_t *clape_infer_generic_types( //
    const clape_type_t *declared,                          //
    const clape_value_t *value,                            //
    clape_generic_binding_t *genv                          //
) {
    switch (declared->tag) {
        case CLAPE_TYPE_GENERIC: {
            const clape_generic_binding_t *existing = clape_find_generic(genv, declared->u.generic);
            if (existing != NULL) {
                if (!clape_type_is_compatible(value, &existing->type, &genv)) {
                    fprintf(stderr, "Type error: conflicting generic inference for '%s'\n",
                        declared->u.generic);
                    exit(1);
                }
                return genv;
            }
            return clape_genv_prepend(genv, declared->u.generic, clape_type_clone(&value->type));
        }
        case CLAPE_TYPE_PRODUCT: {
            if (value->type.tag != CLAPE_TYPE_PRODUCT) {
                return genv;
            }
            for (size_t i = 0; i < declared->u.product.fields->len; i++) {
                clape_product_field_t *const field = ACCESS_ARR_AT(      //
                    clape_product_field_t, declared->u.product.fields, i //
                );
                for (size_t j = 0; j < value->u.product->len; j++) {
                    clape_product_value_t *const field_value = ACCESS_ARR_AT( //
                        clape_product_value_t, value->u.product, j            //
                    );
                    if (strcmp(field_value->name, field->name) == 0) {
                        genv = clape_infer_generic_types(&field->type, &field_value->value, genv);
                        break;
                    }
                }
            }
            return genv;
        }
        case CLAPE_TYPE_SUM: {
            if (value->type.tag != CLAPE_TYPE_SUM) {
                return genv;
            }
            for (size_t i = 0; i < declared->u.sum.variants->len; i++) {
                clape_sum_variant_t *const variant = ACCESS_ARR_AT(  //
                    clape_sum_variant_t, declared->u.sum.variants, i //
                );
                if (strcmp(variant->name, value->u.sum.constructor) != 0) {
                    continue;
                }
                if (variant->has_type && value->u.sum.value) {
                    return clape_infer_generic_types(&variant->type, value->u.sum.value, genv);
                }
                return genv;
            }
            return genv;
        }
        case CLAPE_TYPE_LIST: {
            if (value->type.tag != CLAPE_TYPE_LIST) {
                return genv;
            }
            for (clape_cons_t *n = value->u.list; n; n = n->tail) {
                genv = clape_infer_generic_types(declared, &n->head, genv);
            }
            return genv;
        }
        default:
            return genv;
    }
}

static clape_value_t clape_eval(clape_expr_t *expr, clape_env_t *env) {
    // Two helper macros to greatly reduce code duplication
#define CMP_NUMERIC(op)                                                                            \
    do {                                                                                           \
        bool _r = false;                                                                           \
        switch (lhs.type.tag) {                                                                    \
            case CLAPE_TYPE_INT:                                                                   \
                _r = lhs.u.ival op rhs.u.ival;                                                     \
                break;                                                                             \
            case CLAPE_TYPE_FLOAT:                                                                 \
                _r = lhs.u.fval op rhs.u.fval;                                                     \
                break;                                                                             \
            case CLAPE_TYPE_CHAR:                                                                  \
                _r = lhs.u.cval op rhs.u.cval;                                                     \
                break;                                                                             \
            default:                                                                               \
                fprintf(stderr, "Comparison requires Int, Float, or Char\n");                      \
                exit(1);                                                                           \
        }                                                                                          \
        return (clape_value_t){.type = {.tag = CLAPE_TYPE_BOOL}, .u.bval = _r};                    \
    } while (0)

#define CMP_EQUALITY(op)                                                                           \
    do {                                                                                           \
        bool _r = false;                                                                           \
        switch (lhs.type.tag) {                                                                    \
            case CLAPE_TYPE_INT:                                                                   \
                _r = lhs.u.ival op rhs.u.ival;                                                     \
                break;                                                                             \
            case CLAPE_TYPE_FLOAT:                                                                 \
                _r = lhs.u.fval op rhs.u.fval;                                                     \
                break;                                                                             \
            case CLAPE_TYPE_BOOL:                                                                  \
                _r = lhs.u.bval op rhs.u.bval;                                                     \
                break;                                                                             \
            case CLAPE_TYPE_STRING:                                                                \
                _r = strcmp(lhs.u.sval, rhs.u.sval) op 0;                                          \
                break;                                                                             \
            case CLAPE_TYPE_CHAR:                                                                  \
                _r = lhs.u.cval op rhs.u.cval;                                                     \
                break;                                                                             \
            default:                                                                               \
                fprintf(stderr, "Equality not supported for this type\n");                         \
                exit(1);                                                                           \
        }                                                                                          \
        return (clape_value_t){.type = {.tag = CLAPE_TYPE_BOOL}, .u.bval = _r};                    \
    } while (0)

    switch (expr->tag) {
        case CLAPE_EXPR_LIT: {
            clape_value_t v = expr->u.lit;
            if (v.type.tag == CLAPE_TYPE_STRING) {
                v.u.sval = strdup(v.u.sval);
            }
            return v;
        }
        case CLAPE_EXPR_IDENT: {
            for (clape_env_t *e = env; e; e = e->next) {
                if (strcmp(e->name, expr->u.ident) == 0) {
                    return e->value;
                }
            }
            fprintf(stderr, "Undefined variable: %s in env={", expr->u.ident);
            for (clape_env_t *e = env; e; e = e->next) {
                fprintf(stderr, "%s%s%s", e == env ? "" : ", ", e->name,
                    e->value.type.tag == CLAPE_TYPE_FUNC ? ":func" : "");
            }
            fprintf(stderr, "}\n");
            exit(1);
        }
        case CLAPE_EXPR_UNARY: {
            clape_value_t operand = clape_eval(expr->u.unary.operand, env);
            if (operand.type.tag != CLAPE_TYPE_BOOL) {
                fprintf(stderr, "not requires a Bool operand\n");
                exit(1);
            }
            return (clape_value_t){
                .type = {.tag = CLAPE_TYPE_BOOL},
                .u.bval = !operand.u.bval,
            };
        }
        case CLAPE_EXPR_BINOP: {
            clape_value_t lhs = clape_eval(expr->u.binop.lhs, env);
            clape_value_t rhs = clape_eval(expr->u.binop.rhs, env);

            clape_binop_e op = expr->u.binop.op;
            if (lhs.type.tag != rhs.type.tag && op != CLAPE_BINOP_CONS) {
                fprintf(stderr, "Type mismatch in binary operation\n");
                exit(1);
            }

            switch (op) {
                case CLAPE_BINOP_AND:
                case CLAPE_BINOP_OR:
                    if (lhs.type.tag != CLAPE_TYPE_BOOL || rhs.type.tag != CLAPE_TYPE_BOOL) {
                        fprintf(stderr, "and/or require Bool operands\n");
                        exit(1);
                    }
                    return (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_BOOL},
                        .u.bval = (op == CLAPE_BINOP_AND) ? (lhs.u.bval && rhs.u.bval)
                                                          : (lhs.u.bval || rhs.u.bval),
                    };
                case CLAPE_BINOP_ADD:
                    if (lhs.type.tag == CLAPE_TYPE_STRING && rhs.type.tag == CLAPE_TYPE_STRING) {
                        size_t llen = strlen(lhs.u.sval);
                        size_t rlen = strlen(rhs.u.sval);
                        char *result = malloc(llen + rlen + 1);
                        memcpy(result, lhs.u.sval, llen);
                        memcpy(result + llen, rhs.u.sval, rlen + 1);
                        return (clape_value_t){
                            .type = {.tag = CLAPE_TYPE_STRING},
                            .u.sval = result,
                        };
                    }
                    [[fallthrough]];
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
                                    r = lhs.u.ival + rhs.u.ival;
                                    break;
                                case CLAPE_BINOP_SUB:
                                    r = lhs.u.ival - rhs.u.ival;
                                    break;
                                case CLAPE_BINOP_MUL:
                                    r = lhs.u.ival * rhs.u.ival;
                                    break;
                                case CLAPE_BINOP_DIV:
                                    r = lhs.u.ival / rhs.u.ival;
                                    break;
                                default:
                                    exit(1);
                            }
                            return (clape_value_t){
                                .type = {.tag = CLAPE_TYPE_INT},
                                .u.ival = r,
                            };
                        }
                        case CLAPE_TYPE_FLOAT: {
                            double r;
                            switch (op) {
                                case CLAPE_BINOP_ADD:
                                    r = lhs.u.fval + rhs.u.fval;
                                    break;
                                case CLAPE_BINOP_SUB:
                                    r = lhs.u.fval - rhs.u.fval;
                                    break;
                                case CLAPE_BINOP_MUL:
                                    r = lhs.u.fval * rhs.u.fval;
                                    break;
                                case CLAPE_BINOP_DIV:
                                    r = lhs.u.fval / rhs.u.fval;
                                    break;
                                default:
                                    exit(1);
                            }
                            return (clape_value_t){
                                .type = {.tag = CLAPE_TYPE_FLOAT},
                                .u.fval = r,
                            };
                        }
                    }
                case CLAPE_BINOP_EQ:
                    CMP_EQUALITY(==);
                case CLAPE_BINOP_NE:
                    CMP_EQUALITY(!=);
                case CLAPE_BINOP_LT:
                    CMP_NUMERIC(<);
                case CLAPE_BINOP_LE:
                    CMP_NUMERIC(<=);
                case CLAPE_BINOP_GT:
                    CMP_NUMERIC(>);
                case CLAPE_BINOP_GE:
                    CMP_NUMERIC(>=);
                case CLAPE_BINOP_CONS: {
                    if (rhs.type.tag == CLAPE_TYPE_LIST) {
                        clape_cons_t *node = malloc(sizeof(clape_cons_t));
                        *node = (clape_cons_t){.head = lhs, .tail = rhs.u.list, .arc = 1};
                        for (clape_cons_t *n = rhs.u.list; n; n = n->tail) {
                            n->arc++;
                        }
                        return (clape_value_t){
                            .type = {.tag = CLAPE_TYPE_LIST},
                            .u.list = node,
                        };
                    }
                    if (rhs.type.tag == CLAPE_TYPE_STRING && lhs.type.tag == CLAPE_TYPE_CHAR) {
                        size_t slen = strlen(rhs.u.sval);
                        char *new_str = malloc(slen + 2);
                        new_str[0] = lhs.u.cval;
                        memcpy(new_str + 1, rhs.u.sval, slen + 1);
                        return (clape_value_t){
                            .type = {.tag = CLAPE_TYPE_STRING},
                            .u.sval = new_str,
                        };
                    }
                    fprintf(stderr, "Cons requires a list or string on the right\n");
                    exit(1);
                }
                case CLAPE_BINOP_PROD: {
                    clape_arr_t *result = clape_arr_create(sizeof(clape_product_value_t), 0);
                    // A's fields go first (left side)
                    if (lhs.type.tag == CLAPE_TYPE_PRODUCT) {
                        for (size_t i = 0; i < lhs.u.product->len; i++) {
                            clape_product_value_t *f = ACCESS_ARR_AT(   //
                                clape_product_value_t, lhs.u.product, i //
                            );
                            clape_product_value_t pf = {.name = strdup(f->name), .value = f->value};
                            clape_arr_append(sizeof(clape_product_value_t), &result, &pf);
                        }
                    }
                    // Then B's fields: right side overrides left
                    if (rhs.type.tag == CLAPE_TYPE_PRODUCT) {
                        for (size_t i = 0; i < rhs.u.product->len; i++) {
                            clape_product_value_t *f = ACCESS_ARR_AT(   //
                                clape_product_value_t, rhs.u.product, i //
                            );
                            bool dup = false;
                            for (size_t j = 0; j < result->len; j++) {
                                clape_product_value_t *pf = ACCESS_ARR_AT( //
                                    clape_product_value_t, result, j       //
                                );
                                if (strcmp(pf->name, f->name) == 0) {
                                    pf->value = f->value;
                                    dup = true;
                                    break;
                                }
                            }
                            if (!dup) {
                                clape_product_value_t pf = {
                                    .name = strdup(f->name),
                                    .value = f->value,
                                };
                                clape_arr_append(sizeof(clape_product_value_t), &result, &pf);
                            }
                        }
                    }
                    return (clape_value_t){
                        .type = {.tag = CLAPE_TYPE_PRODUCT},
                        .u.product = result,
                    };
                }
            }
#undef CMP_NUMERIC
#undef CMP_EQUALITY
            __builtin_unreachable();
        }
        case CLAPE_EXPR_IF: {
            clape_value_t cond = clape_eval(expr->u.if_.condition, env);
            if (cond.type.tag != CLAPE_TYPE_BOOL) {
                fprintf(stderr, "if condition must be Bool\n");
                exit(1);
            }
            if (cond.u.bval) {
                return clape_eval(expr->u.if_.then_branch, env);
            }
            return clape_eval(expr->u.if_.else_branch, env);
        }
        case CLAPE_EXPR_LAMBDA: {
            clape_fn_t *fn = malloc(sizeof(clape_fn_t));
            *fn = (clape_fn_t){
                .is_builtin = false,
                .params = expr->u.lambda.params,
                .return_type = expr->u.lambda.return_type,
                .body = expr->u.lambda.body,
                .builtin_fn = NULL,
                .next_param_index = 0,
                .closure = env,
                .generic_params = expr->u.lambda.generic_params,
                .genv = NULL,
            };
            return (clape_value_t){
                .type = {.tag = CLAPE_TYPE_FUNC},
                .u.fn = fn,
            };
        }
        case CLAPE_EXPR_BLOCK: {
            clape_env_t *block_env = env;
            for (size_t i = 0; i < expr->u.block.stmts->len; i++) {
                clape_stmt_t *s = ACCESS_ARR_AT(clape_stmt_t, expr->u.block.stmts, i);
                clape_value_t val = clape_eval(&s->u.let.expr, block_env);

                if (strcmp(s->u.let.name, "_") == 0) {
                    continue;
                }
                // Create new binding
                clape_env_t *const binding = malloc(sizeof(clape_env_t));
                *binding = (clape_env_t){
                    .name = strdup(s->u.let.name),
                    .value = val,
                    .next = block_env,
                };
                if (val.type.tag == CLAPE_TYPE_FUNC && !val.u.fn->is_builtin) {
                    // Make the function see itself in its closure
                    clape_env_t *const self_binding = malloc(sizeof(clape_env_t));
                    *self_binding = *binding;
                    val.u.fn->closure = self_binding;
                }
                block_env = binding;
            }
            if (expr->u.block.return_expr) {
                return clape_eval(expr->u.block.return_expr, block_env);
            }
            return (clape_value_t){.type = {.tag = CLAPE_TYPE_UNIT}};
        }
        case CLAPE_EXPR_LIST: {
            clape_cons_t *list = NULL;
            for (size_t i = expr->u.lst.elements->len; i > 0; i--) {
                clape_expr_t *e = ACCESS_ARR_AT(clape_expr_t, expr->u.lst.elements, i - 1);
                clape_value_t elem = clape_eval(e, env);
                clape_cons_t *node = malloc(sizeof(clape_cons_t));
                *node = (clape_cons_t){.head = elem, .tail = list, .arc = 1};
                list = node;
            }
            return (clape_value_t){
                .type = {.tag = CLAPE_TYPE_LIST},
                .u.list = list,
            };
        }
        case CLAPE_EXPR_MATCH: {
            clape_value_t scrutinee = clape_eval(expr->u.match_.scrutinee, env);
            for (size_t i = 0; i < expr->u.match_.arms->len; i++) {
                clape_match_arm_t *arm = ACCESS_ARR_AT(clape_match_arm_t, expr->u.match_.arms, i);
                clape_env_t *arm_env = clape_match_value(scrutinee, &arm->pattern, env);
                if (arm_env) {
                    return clape_eval(&arm->body, arm_env);
                }
            }
            fprintf(stderr, "Non-exhaustive match\n");
            exit(1);
        }
        case CLAPE_EXPR_PRODUCT_TYPE:
        case CLAPE_EXPR_SUM_TYPE:
            return (clape_value_t){.type = {.tag = CLAPE_TYPE_UNIT}};
        case CLAPE_EXPR_SUM: {
            clape_value_t *payload = NULL;
            if (expr->u.sum.expr) {
                payload = malloc(sizeof(clape_value_t));
                *payload = clape_eval(expr->u.sum.expr, env);
            }
            return (clape_value_t){
                .type = {.tag = CLAPE_TYPE_SUM},
                .u.sum = {.constructor = strdup(expr->u.sum.constructor), .value = payload},
            };
        }
        case CLAPE_EXPR_FIELD: {
            const clape_value_t val = clape_eval(expr->u.field.value, env);
            clape_arr_t *fields = clape_arr_create(sizeof(clape_product_value_t), 0);
            clape_product_value_t pv = {
                .name = strdup(expr->u.field.name),
                .value = val,
            };
            clape_arr_append(sizeof(clape_product_value_t), &fields, &pv);
            return (clape_value_t){
                .type = {.tag = CLAPE_TYPE_PRODUCT},
                .u.product = fields,
            };
        }
        case CLAPE_EXPR_FIELD_ACCESS: {
            const clape_value_t obj = clape_eval(expr->u.field_access.expr, env);
            if (obj.type.tag != CLAPE_TYPE_PRODUCT) {
                fprintf(stderr, "Field access requires a product value\n");
                exit(1);
            }
            for (size_t i = 0; i < obj.u.product->len; i++) {
                clape_product_value_t *f = ACCESS_ARR_AT(clape_product_value_t, obj.u.product, i);
                if (strcmp(f->name, expr->u.field_access.name) == 0) {
                    return f->value;
                }
            }
            fprintf(stderr, "No such field '%s' in product\n", expr->u.field_access.name);
            exit(1);
        }
        case CLAPE_EXPR_TYPE:
            return (clape_value_t){.type = {.tag = CLAPE_TYPE_UNIT}};
        case CLAPE_EXPR_CALL: {
            const clape_value_t callee = clape_eval(expr->u.call.callee, env);
            const clape_value_t arg = clape_eval(expr->u.call.arg, env);
            if (callee.type.tag != CLAPE_TYPE_FUNC) {
                fprintf(stderr, "Attempted to call a non-function value\n");
                exit(1);
            }
            clape_fn_t *const fn = callee.u.fn;

            if (fn->is_builtin) {
                return fn->builtin_fn(arg);
            }

            const size_t idx = fn->next_param_index;
            clape_param_t *const param = ACCESS_ARR_AT(clape_param_t, fn->params, idx);

            clape_generic_binding_t *genv = fn->genv;
            genv = clape_infer_generic_types(&param->type, &arg, genv);

            if (!clape_type_is_compatible(&arg, &param->type, &genv)) {
                fprintf(stderr, "Type error: parameter '%s' expected %s, got %s\n", param->name,
                    clape_type_tag_name(param->type.tag), clape_type_tag_name(arg.type.tag));
                clape_genv_free(genv);
                exit(1);
            }

            clape_env_t *const new_closure = malloc(sizeof(clape_env_t));
            *new_closure = (clape_env_t){
                .name = strdup(param->name),
                .value = arg,
                .next = fn->closure,
            };

            if (idx + 1 == fn->params->len) {
                const clape_value_t result = clape_eval(fn->body, new_closure);
                if (!clape_type_is_compatible(&result, &fn->return_type, &genv)) {
                    fprintf(stderr, "Type error: function returned %s, expected %s\n",
                        clape_type_tag_name(result.type.tag),
                        clape_type_tag_name(fn->return_type.tag));
                    clape_genv_free(genv);
                    exit(1);
                }
                clape_genv_free(genv);
                return result;
            }
            clape_fn_t *const partial = malloc(sizeof(clape_fn_t));
            *partial = (clape_fn_t){
                .is_builtin = false,
                .params = fn->params,
                .return_type = fn->return_type,
                .body = fn->body,
                .builtin_fn = NULL,
                .next_param_index = idx + 1,
                .closure = new_closure,
                .generic_params = NULL,
                .genv = genv,
            };
            return (clape_value_t){
                .type = {.tag = CLAPE_TYPE_FUNC},
                .u.fn = partial,
            };
        }
    }
    return (clape_value_t){.type = {.tag = CLAPE_TYPE_UNIT}};
}

static void clape_print_value_inner(clape_value_t v, int depth) {
    switch (v.type.tag) {
        case CLAPE_TYPE_INT:
            printf("%li", v.u.ival);
            break;
        case CLAPE_TYPE_FLOAT:
            printf("%g", v.u.fval);
            break;
        case CLAPE_TYPE_BOOL:
            printf("%s", v.u.bval ? "true" : "false");
            break;
        case CLAPE_TYPE_CHAR:
            if (depth == 0) {
                printf("%c", v.u.cval);
            } else {
                printf("'%c'", v.u.cval);
            }
            break;
        case CLAPE_TYPE_STRING:
            if (depth == 0) {
                printf("%s", v.u.sval);
            } else {
                printf("\"%s\"", v.u.sval);
            }
            break;
        case CLAPE_TYPE_UNIT:
            if (depth > 0) {
                // Print nothing, used for printing empty lines
                printf("Unit");
            }
            break;
        case CLAPE_TYPE_FUNC:
            printf("<function>");
            break;
        case CLAPE_TYPE_GENERIC:
            printf("<generic>");
            break;
        case CLAPE_TYPE_LIST: {
            printf("[");
            bool first = true;
            for (clape_cons_t *node = v.u.list; node; node = node->tail) {
                if (!first) {
                    printf(", ");
                }
                first = false;
                clape_print_value_inner(node->head, depth + 1);
            }
            printf("]");
            break;
        }
        case CLAPE_TYPE_PRODUCT: {
            for (size_t i = 0; i < v.u.product->len; i++) {
                if (i > 0) {
                    printf(" & ");
                }
                clape_product_value_t *f = ACCESS_ARR_AT(clape_product_value_t, v.u.product, i);
                printf(".%s(", f->name);
                clape_print_value_inner(f->value, depth + 1);
                printf(")");
            }
            break;
        }
        case CLAPE_TYPE_SUM: {
            printf(".%s", v.u.sum.constructor);
            if (v.u.sum.value) {
                printf("(");
                clape_print_value_inner(*v.u.sum.value, depth + 1);
                printf(")");
            }
            break;
        }
    }
}

static clape_value_t clape_builtin_print(clape_value_t arg) {
    clape_print_value_inner(arg, 0);
    printf("\n");
    return (clape_value_t){.type = {.tag = CLAPE_TYPE_UNIT}};
}

static void clape_free_list_value(clape_cons_t *list) {
    while (list != NULL) {
        clape_cons_t *const next = list->tail;
        if (--list->arc == 0) {
            if (list->head.type.tag == CLAPE_TYPE_STRING) {
                free(list->head.u.sval);
            } else if (list->head.type.tag == CLAPE_TYPE_LIST) {
                clape_free_list_value(list->head.u.list);
            }
            free(list);
        }
        list = next;
    }
}

static void clape_env_free(clape_env_t *env) {
    while (env != NULL) {
        clape_env_t *const next = env->next;
        free(env->name);
        if (env->value.type.tag == CLAPE_TYPE_FUNC) {
            free(env->value.u.fn);
        } else if (env->value.type.tag == CLAPE_TYPE_STRING) {
            free(env->value.u.sval);
        } else if (env->value.type.tag == CLAPE_TYPE_LIST) {
            clape_free_list_value(env->value.u.list);
        } else if (env->value.type.tag == CLAPE_TYPE_PRODUCT) {
            for (size_t i = 0; i < env->value.u.product->len; i++) {
                clape_product_value_t *f = ACCESS_ARR_AT(          //
                    clape_product_value_t, env->value.u.product, i //
                );
                free(f->name);
                if (f->value.type.tag == CLAPE_TYPE_STRING) {
                    free(f->value.u.sval);
                } else if (f->value.type.tag == CLAPE_TYPE_LIST) {
                    clape_free_list_value(f->value.u.list);
                }
            }
            free(env->value.u.product);
        } else if (env->value.type.tag == CLAPE_TYPE_SUM) {
            free(env->value.u.sum.constructor);
            if (env->value.u.sum.value) {
                if (env->value.u.sum.value->type.tag == CLAPE_TYPE_STRING) {
                    free(env->value.u.sum.value->u.sval);
                } else if (env->value.u.sum.value->type.tag == CLAPE_TYPE_LIST) {
                    clape_free_list_value(env->value.u.sum.value->u.list);
                }
                free(env->value.u.sum.value);
            }
        }
        free(env);
        env = next;
    }
}

void clape_interpret(clape_program_t *const program) {
    clape_env_t *env = NULL;

    for (size_t i = 0; i < program->statements->len; i++) {
        clape_stmt_t *const stmt = ACCESS_ARR_AT(clape_stmt_t, program->statements, i);

        switch (stmt->tag) {
            case CLAPE_STMT_LET: {
                const clape_value_t val = clape_eval(&stmt->u.let.expr, env);
                const bool is_type = val.type.tag == CLAPE_TYPE_UNIT;
                const bool is_product_type = is_type //
                    && stmt->u.let.expr.tag == CLAPE_EXPR_PRODUCT_TYPE;
                const bool is_sum_type = is_type //
                    && stmt->u.let.expr.tag == CLAPE_EXPR_SUM_TYPE;
                if (is_product_type || is_sum_type) {
                    // Type binding: no runtime value
                    break;
                }
                if (strcmp(stmt->u.let.name, "_") == 0) {
                    break;
                }
                clape_env_t *const binding = malloc(sizeof(clape_env_t));
                *binding = (clape_env_t){
                    .name = strdup(stmt->u.let.name),
                    .value = val,
                    .next = env,
                };
                env = binding;
                // Update closure to include this binding, enabling recursion.
                // Prepend self-binding to preserve existing captured variables.
                if (val.type.tag == CLAPE_TYPE_FUNC && !val.u.fn->is_builtin) {
                    clape_env_t *self = malloc(sizeof(clape_env_t));
                    *self = (clape_env_t){
                        .name = strdup(stmt->u.let.name),
                        .value = val,
                        .next = val.u.fn->closure,
                    };
                    val.u.fn->closure = self;
                }
                break;
            }
            case CLAPE_STMT_USE: {
                // Use statement
                if (strcmp(stmt->u.use.module, "Print") != 0) {
                    fprintf(stderr, "Unknown module: %s\n", stmt->u.use.module);
                    exit(1);
                }
                clape_fn_t *const print_fn = malloc(sizeof(clape_fn_t));
                *print_fn = (clape_fn_t){
                    .is_builtin = true,
                    .params = NULL,
                    .return_type = {.tag = CLAPE_TYPE_UNIT},
                    .body = NULL,
                    .builtin_fn = clape_builtin_print,
                    .next_param_index = 0,
                    .closure = NULL,
                };
                clape_env_t *const binding = malloc(sizeof(clape_env_t));
                *binding = (clape_env_t){
                    .name = strdup("print"),
                    .value =
                        (clape_value_t){
                            .type = {.tag = CLAPE_TYPE_FUNC},
                            .u.fn = print_fn,
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
