#include "CSV_proc.h"

Token GetToken(Parser_t* parser) {
    if (!parser->input) return (Token){TOKEN_EOF, 0.0, ""};
    const char* str = parser->input;
    while (str[parser->pos] != '\0' && (str[parser->pos] == ' ')) parser->pos++;

    char curr = str[parser->pos];
    if (curr == '\0') return (Token){TOKEN_EOF, 0.0, ""};

    if (isdigit((unsigned char)curr) || curr == '.') {
        char* endptr;
        double val = strtod(&str[parser->pos], &endptr);
        if (endptr == &str[parser->pos]) return (Token){TOKEN_ERROR, 0.0, ""};
        parser->pos = endptr - str;
        return (Token){NUMBER_TOKEN, val, ""};
    }

    if (isalpha((unsigned char)curr)) {
        int start = parser->pos;
        while (isalnum((unsigned char)str[parser->pos])) parser->pos++;

        int len = parser->pos - start;
        if (len > 31) len = 31;

        Token t;
        memset(&t, 0, sizeof(Token));
        strncpy(t.name, &str[start], len);
        t.name[len] = '\0';
        for (int i = 0; t.name[i]; i++) t.name[i] = toupper((unsigned char)t.name[i]);

        int temp_pos = parser->pos;
        while (str[temp_pos] == ' ') temp_pos++;

        if (str[temp_pos] == '(') {
            t.type = FUNC;
        } else {
            t.type = LINK;
        }
        return t;
    }

    parser->pos++;
    switch (curr) {
        case '+': return (Token){PLUS, 0.0, ""};
        case '-': return (Token){MINUS, 0.0, ""};
        case '*': return (Token){MULT, 0.0, ""};
        case '/': return (Token){DIV, 0.0, ""};
        case '(': return (Token){LEFT_BRACKET, 0.0, ""};
        case ')': return (Token){RIGHT_BRACKET, 0.0, ""};
        case ':': return (Token){COLON, 0.0, ""};
        case ',': return (Token){COMMA, 0.0, ""};
        default: return (Token){TOKEN_ERROR, 0.0, ""};
    }
}

Token PeekToken(Parser_t* parser) {
    return parser->stored_token;
}

Token ReadToken(Parser_t* parser) {
    Token curr = parser->stored_token;
    parser->stored_token = GetToken(parser);
    return curr;
}

void link_to_coords(char* link, int* out_row, int* out_col) {
    int i = 0;
    int col = 0;
    while (isalpha((unsigned char)link[i])) {
        col = col * 26 + (toupper((unsigned char)link[i]) - 'A' + 1);
        i++;
    }
    *out_col = col - 1;
    *out_row = atoi(&link[i]) - 1;
}

void coords_to_link(int r, int c, char* out_buf) {
    int temp_c = c;
    int idx = 0;
    char col_str[16];
    while (temp_c >= 0) {
        col_str[idx++] = (temp_c % 26) + 'A';
        temp_c = (temp_c / 26) - 1;
    }
    col_str[idx] = '\0';
    
    for (int i = 0; i < idx / 2; i++) {
        char tmp = col_str[i];
        col_str[i] = col_str[idx - 1 - i];
        col_str[idx - 1 - i] = tmp;
    }
    sprintf(out_buf, "%s%d", col_str, r + 1);
}

double eval_range(Table* table, char* func, int r1, int c1, int r2, int c2, ErrCode* out_err) {
    bool F_sum = strcmp(func, "SUM") == 0;
    bool F_avg = strcmp(func, "AVG") == 0;
    bool F_min = strcmp(func, "MIN") == 0;
    bool F_max = strcmp(func, "MAX") == 0;
    bool F_mul = strcmp(func, "MUL") == 0;

    if (!F_sum && !F_avg && !F_min && !F_max && !F_mul) {
        *out_err = ERR_FNAM;
        return NAN;
    }

    double res = 0.0;
    int count = 0;
    int start_r = r1 < r2 ? r1 : r2;
    int end_r = r1 > r2 ? r1 : r2;
    int start_c = c1 < c2 ? c1 : c2;
    int end_c = c1 > c2 ? c1 : c2;

    if (F_min) res = INFINITY;
    if (F_max) res = -INFINITY;
    if (F_mul) res = 1.0;

    for (int r = start_r; r <= end_r; r++) {
        for (int c = start_c; c <= end_c; c++) {
            double val = eval_cell(table, r, c);
            
            if (table->grid[r][c].err != ERR_NONE) {
                *out_err = table->grid[r][c].err;
                return NAN;
            }

            count++;
            if (F_sum || F_avg) {
                res += val;
            } else if (F_mul) {
                res *= val;
            } else if (F_min) {
                if (val < res) res = val;
            } else if (F_max) {
                if (val > res) res = val;
            }
        }
    }

    if (F_avg) {
        return (count > 0) ? (res / count) : (0.0);
    }
    return res;
}

double ParseAtom(Parser_t *parser) {
    if (parser->err != ERR_NONE) return NAN;
    Token temp = PeekToken(parser);
    switch (temp.type) {

        case MINUS: {
            ReadToken(parser);
            return -ParseAtom(parser);
        }

        case LEFT_BRACKET: {
            ReadToken(parser); 
            double res = ParseExpr(parser);
            if (parser->err != ERR_NONE) return NAN;
            Token close = ReadToken(parser); 
            if (close.type != RIGHT_BRACKET) {
                parser->err = ERR_UNFO;
                return NAN;
            }
            return res;
        }

        case NUMBER_TOKEN: {
            ReadToken(parser);
            return temp.value;
        }

        case LINK: {
            ReadToken(parser);
            int r, c;
            link_to_coords(temp.name, &r, &c);
            
            if (r < 0 || r >= parser->table->rows || c < 0 || c >= parser->table->cols) {
                parser->err = ERR_ICRD;
                return NAN;
            }

            double val = eval_cell(parser->table, r, c);
            
            if (parser->table->grid[r][c].err != ERR_NONE) {
                parser->err = parser->table->grid[r][c].err;
                return NAN;
            }
            return val;
        }

        case FUNC: { // 'funcname' + '(' + 'ARG1' + ':' + 'ARG2' + ')'
            Token function = ReadToken(parser); // funcname
            Token open = ReadToken(parser);     // (     
            if (open.type != LEFT_BRACKET) { parser->err = ERR_UNFO; return NAN; }
            Token arg1 = ReadToken(parser);     // ARG1
            if (arg1.type != LINK) { parser->err = ERR_UNFO; return NAN; }
            Token sep = ReadToken(parser);      // :
            if (sep.type != COLON) { parser->err = ERR_UNFO; return NAN; }
            Token arg2 = ReadToken(parser);     // ARG2
            if (arg2.type != LINK) { parser->err = ERR_UNFO; return NAN; }
            Token close = ReadToken(parser);    // )
            if (close.type != RIGHT_BRACKET) { parser->err = ERR_UNFO; return NAN; }

            int r1, r2, c1, c2;
            link_to_coords(arg1.name, &r1, &c1);
            link_to_coords(arg2.name, &r2, &c2);

            if (r1 < 0 || r1 >= parser->table->rows || c1 < 0 || c1 >= parser->table->cols ||
                r2 < 0 || r2 >= parser->table->rows || c2 < 0 || c2 >= parser->table->cols) {
                parser->err = ERR_ICRD;
                return NAN;
            }

            ErrCode range_err = ERR_NONE;
            double res = eval_range(parser->table, function.name, r1, c1, r2, c2, &range_err);
            if (range_err != ERR_NONE) {
                parser->err = range_err;
                return NAN;
            }
            return res;
        }

        default: {
            parser->err = ERR_UNFO;
            return NAN;
        }
    }
}

double ParseMonome(Parser_t *parser) {
    if (parser->err != ERR_NONE) return NAN;
    double res = ParseAtom(parser);
    if (parser->err != ERR_NONE) return NAN;
    
    Token temp = PeekToken(parser);
    while (temp.type == MULT || temp.type == DIV) {
        Token oper = ReadToken(parser);
        double next_val = ParseAtom(parser);
        if (parser->err != ERR_NONE) return NAN;
        
        if (oper.type == MULT) {
            res *= next_val;
        } else {
            if (next_val == 0.0) {
                parser->err = ERR_DIZE;
                return NAN;
            }
            res /= next_val;
        }
        temp = PeekToken(parser);
    }
    return res;
}

double ParseExpr(Parser_t *parser) {
    if (parser->err != ERR_NONE) return NAN;
    double res = ParseMonome(parser);
    if (parser->err != ERR_NONE) return NAN;
    
    Token temp = PeekToken(parser);
    while (temp.type == PLUS || temp.type == MINUS) {
        Token oper = ReadToken(parser);
        double next_val = ParseMonome(parser);
        if (parser->err != ERR_NONE) return NAN;
        
        if (oper.type == PLUS) {
            res += next_val;
        } else {
            res -= next_val;
        }
        temp = PeekToken(parser);
    }
    return res;
}

double eval_cell(Table* table, int r, int c) {
    if (!table || r < 0 || r >= table->rows || c < 0 || c >= table->cols) {
        return NAN;
    }
    Cell* cell = &table->grid[r][c];

    if (cell->state == VISITING) { 
        cell->err = ERR_CYDE;
        return NAN;
    }
    if (cell->state == VISITED) { 
        return cell->num_val;
    }

    cell->state = VISITING;
    char* raw_txt = cell->raw_text;
    if (!raw_txt) {
        cell->num_val = 0.0;
        cell->type = EMPTY;
        cell->err = ERR_NONE;
        cell->state = VISITED;
        return 0.0;
    }
    while (*raw_txt == ' ') raw_txt++;

    if (*raw_txt == '\0') { 
        cell->num_val = 0.0;
        cell->type = EMPTY;
        cell->err = ERR_NONE;
        cell->state = VISITED;

    } else if (*raw_txt == '=') { 
        Parser_t parser;
        parser.input = raw_txt + 1; 
        parser.pos = 0;
        parser.table = table;
        parser.err = ERR_NONE;
        parser.stored_token = GetToken(&parser);

        double res = ParseExpr(&parser);

        if (parser.err != ERR_NONE) {
            cell->num_val = NAN;
            cell->err = parser.err;
            cell->type = FORMULA;
            cell->state = VISITED;
        } else if (parser.stored_token.type != TOKEN_EOF) {
            cell->num_val = NAN;
            cell->err = ERR_UNFO;
            cell->type = FORMULA;
            cell->state = VISITED;
        } else { 
            cell->num_val = res;
            cell->err = ERR_NONE;
            cell->type = FORMULA;
            cell->state = VISITED;
        }

    } else {
        char* endptr;
        double val = strtod(raw_txt, &endptr);

        while (*endptr == ' ' || *endptr == '\r' || *endptr == '\n') endptr++;

        if (endptr != raw_txt && *endptr == '\0') { 
            cell->num_val = val;
            cell->type = NUMBER;
            cell->err = ERR_NONE;
            cell->state = VISITED;
        } else {
            cell->num_val = 0.0;
            cell->type = TEXT;
            cell->err = ERR_NONE;
            cell->state = VISITED;
        }
    }

    return cell->num_val;
}

void eval_table(Table* table) {
    if (!table) return;

    for (int r = 0; r < table->rows; r++) {
        for (int c = 0; c < table->cols; c++) {
            eval_cell(table, r, c);
        }
    }
}
