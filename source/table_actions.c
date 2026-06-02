#include "CSV_proc.h"

int sort_index;
int sort_order; 

char* str_dupl(char* str) {
    if (!str) return NULL;
    int len = strlen(str) + 1;
    char* copy = malloc(sizeof(char) * len);
    if (copy) memcpy(copy, str, len);
    return copy;
}

Table* create_empty_table() {
    Table* t = (Table*)malloc(sizeof(Table));
    t->rows = 0;
    t->cols = 0;
    t->grid = NULL;
    return t;
}

void free_table(Table* table) {
    if (!table) return;
    for (int r = 0; r < table->rows; r++) {
        for (int c = 0; c < table->cols; c++) {
            if (table->grid[r][c].raw_text) {
                free(table->grid[r][c].raw_text);
            }
        }
        free(table->grid[r]);
    }
    free(table->grid);
    free(table);
}

char* readline(FILE* stream) {
    int cap = 128;
    int len = 0;
    char* buf = malloc(sizeof(char) * cap);
    if (!buf) return NULL;
    buf[0] = '\0';

    while (1) { 
        if (!fgets(buf + len, cap - len, stream)) {
            if (len == 0) { 
                free(buf);
                return NULL;
            }
            break; 
        }

        len = strlen(buf); 
        if (len > 0 && buf[len-1] == '\n') {
            while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
                buf[--len] = '\0';
            }
            break; 
        }

        cap *= 2;
        char* temp = realloc(buf, cap);
        if (!temp) {
            free(buf);
            return NULL;
        }
        buf = temp;
    }

    return buf;
}

void read_CSV(FILE* stream, Table* table, char separator) {
    char* line = NULL;
    while ((line = readline(stream)) != NULL) {
        Cell** ext_grid = realloc(table->grid, sizeof(Cell*) * (table->rows + 1));
        if (!ext_grid) { free(line); return; }
        table->grid = ext_grid;

        int row_cap = 10; 
        Cell* new_row = malloc(sizeof(Cell) * row_cap);
        int cols_found = 0;
        char* subline = line;
        char* symb = line;

        while (1) {
            if (*symb == separator || *symb == '\0') {
                char orig = *symb;
                *symb = '\0';

                if (cols_found >= row_cap) {
                    row_cap *= 2;
                    new_row = realloc(new_row, sizeof(Cell) * row_cap);
                }

                new_row[cols_found].raw_text = str_dupl(subline);
                new_row[cols_found].num_val = 0.0;
                new_row[cols_found].state = UNVISITED;
                new_row[cols_found].err = ERR_NONE;

                cols_found++;
                if (orig == '\0') break; 
                subline = symb + 1;
            }
            symb++;
        }

        if (cols_found > table->cols) {

            int old_cols = table->cols;
            table->cols = cols_found;
            for (int r = 0; r < table->rows; r++) {
                table->grid[r] = realloc(table->grid[r], sizeof(Cell) * table->cols);
                for (int c = old_cols; c < table->cols; c++) {
                    table->grid[r][c].raw_text = str_dupl("");
                    table->grid[r][c].num_val = 0.0;
                    table->grid[r][c].state = UNVISITED;
                    table->grid[r][c].err = ERR_NONE;
                    table->grid[r][c].type = EMPTY;
                }
            }

        } else if (cols_found < table->cols) {

            new_row = realloc(new_row, sizeof(Cell) * table->cols);
            for (int c = cols_found; c < table->cols; c++) {
                new_row[c].raw_text = str_dupl("");
                new_row[c].num_val = 0.0;
                new_row[c].state = UNVISITED;
                new_row[c].err = ERR_NONE;
                new_row[c].type = EMPTY;
            }
        }

        table->grid[table->rows] = new_row;
        table->rows++;
        free(line);
    }
}

void write_CSV(FILE* stream, Table* table, char sep) {
    for (int r = 0; r < table->rows; r++) {
        for (int c = 0; c < table->cols; c++) {
            Cell* cell = &table->grid[r][c];
            if (cell->err != ERR_NONE) {
                switch (cell->err) {
                    case ERR_CYDE: fprintf(stream, "#CYDE"); break;
                    case ERR_DIZE: fprintf(stream, "#DIZE"); break;
                    case ERR_UNFO: fprintf(stream, "#UNFO"); break;
                    case ERR_FNAM: fprintf(stream, "#FNAM"); break;
                    case ERR_ICRD: fprintf(stream, "#ICRD"); break;
                    default:       fprintf(stream, "#ERR"); break;
                }
            } else if (cell->type == EMPTY && strlen(cell->raw_text) == 0) {
            } else if (cell->type == TEXT) {
                fprintf(stream, "%s", cell->raw_text);
            } else {
                fprintf(stream, "%.10g", cell->num_val);
            }

            if (c < table->cols - 1) {
                fprintf(stream, "%c", sep);
            }
        }
        fprintf(stream, "\n");
    }
}

int cell_cmp (const Cell* a, const Cell* b) {
    if (a->state == VISITED && b->state == VISITED && a->err == ERR_NONE && b->err == ERR_NONE) {
        if (a->type == NUMBER && b->type == NUMBER) {
            if (a->num_val > b->num_val) return 1 * sort_order;
            if (a->num_val < b->num_val) return (-1) * sort_order;
            return 0;
        }
        if (a->type == TEXT && b->type == TEXT) {
            return (strcmp(a->raw_text, b->raw_text) * sort_order);
        }
    }
    return 0;
}

int row_cmp (const void* a, const void* b) {
    Cell* r1 = *(Cell**)a;
    Cell* r2 = *(Cell**)b;
    return cell_cmp(&r1[sort_index], &r2[sort_index]);
}

void sort_rows_by_c(Table* table, int c_index, char order) {
    sort_index = c_index;
    sort_order = (order == 'a') ? (1) : (-1);
    qsort(table->grid, table->rows, sizeof(Cell*), row_cmp);
}

void sort_cols_by_r(Table* table, int r_index, char order) {
    sort_order = (order == 'a') ? (1) : (-1);
    for (int e = 0; e < table->cols - 1; e++) { 
        for (int c = 0; c < table->cols - 1 - e; c++) { 
            Cell* curr = &table->grid[r_index][c];
            Cell* next = &table->grid[r_index][c+1];
            if (cell_cmp(curr, next) > 0) { 
                for (int r = 0; r < table->rows; r++) {
                    Cell temp = table->grid[r][c];
                    table->grid[r][c] = table->grid[r][c+1];
                    table->grid[r][c+1] = temp;
                }
            }
        }
    }
}

void sort_table(Table* table, char order, char target, int num) {
    int index = num - 1;
    if (target == 'c') {
        if (index < 0 || index >= table->cols) return;
        sort_rows_by_c(table, index, order);
    }
    if (target == 'r') {
        if (index < 0 || index >= table->rows) return;
        sort_cols_by_r(table, index, order);
    }
}