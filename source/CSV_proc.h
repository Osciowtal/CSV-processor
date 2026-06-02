#ifndef CSV_proc_H
#define CSV_proc_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

typedef enum {
    ERR_NONE,
    ERR_CYDE,    // #CYDE - Cyclic dependence
    ERR_DIZE,    // #DIZE - Division by zero
    ERR_UNFO,    // #UNFO - Unexpected format / Syntax error
    ERR_FNAM,    // #FNAM - Unknown function name
    ERR_ICRD     // #ICRD - Invalid coordinate / Out of bounds
} ErrCode;

typedef enum {
    EMPTY,
    NUMBER,
    TEXT,
    FORMULA
} Type;

typedef enum {
    UNVISITED,
    VISITING,
    VISITED
} State;

typedef struct {
    Type type;
    State state;
    ErrCode err;
    char *raw_text;
    double num_val;
} Cell;

typedef struct {
    Cell **grid;
    int rows;
    int cols;
} Table;

typedef enum {
    NUMBER_TOKEN,
    LINK, FUNC, COLON, COMMA,
    MINUS, PLUS, DIV, MULT,
    LEFT_BRACKET, RIGHT_BRACKET,
    TOKEN_EOF, TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    double value;
    char name[32];
} Token;

typedef struct {
    char* input;
    int pos;
    Token stored_token;
    Table* table;
    ErrCode err;
} Parser_t;

// table_actions: basic actions
char* str_dupl(char* str);
Table* create_empty_table();
void free_table(Table* table);
char* readline(FILE* stream);
void read_CSV(FILE* stream, Table* table, char separator);
void write_CSV(FILE* stream, Table* table, char sep);
// table_actions: sorting
int cell_cmp (const Cell* a, const Cell* b);
int row_cmp (const void* a, const void* b);
void sort_rows_by_c(Table* table, int c_index, char order);
void sort_cols_by_r(Table* table, int r_index, char order);
void sort_table(Table* table, char order, char target, int num);

// cell_parsing: lexing
Token GetToken(Parser_t* parser);
Token PeekToken(Parser_t* parser);
Token ReadToken(Parser_t* parser);
// cell_parsing: parsing
double ParseAtom(Parser_t *parser);
double ParseMonome(Parser_t *parser);
double ParseExpr(Parser_t *parser);
// cell_parsing: evaluating
double eval_cell(Table* table, int r, int c);
double eval_range(Table* table, char* func, int r1, int c1, int r2, int c2, ErrCode* out_err);
void eval_table(Table* table);
// cell_parsing: cell place representation
void link_to_coords(char* link, int* out_row, int* out_col);
void coords_to_link(int r, int c, char* out_buf);

#endif