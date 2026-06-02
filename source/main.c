#include "CSV_proc.h"

void highlight_error (int argc, char *argv[], int target, int highlight_width) {
    int spaces = 0;
    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "%s ", argv[i]);
        if (i < target) {
            spaces += (strlen(argv[i]) + 1);
        }
    }
    fprintf(stderr, "\n");
    for (int i =  0; i < spaces; i++) {
        fprintf(stderr, " ");
    }
    for (int i = 0; i < highlight_width; i++) {
        fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");
}

int main (int argc, char *argv[]) {
    if (argc == 1) {
        printf("Empty call. Run \"./tp --help\" to get more information.\n");
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0) {
        printf("\nFlags and what to specify after them:\n\n");
        printf("-i   Input CSV: path / name\n");
        printf("-o   Output CSV: path / name\n");
        printf("\nAdditional arguments:\n\n");
        printf("-sep What should be considered as separator - C (comma is default) or S (semicolon)\n");
        printf("-ord How to organize table - pass 3 args in this format:\n");
        printf("     [order]    [target]   [number]\n");
        printf("     asc / des  row / col  number\n");
        printf("\nPay attention:\n");
        printf("If you want to provide a path after -i -o,\nplease make sure the path DOES NOT contain a space character.\n");
        printf("If it does, wrap the path with double quotes.\n");
        printf("\nExample of usage:\n./tp -i \"C:/Space in path/data.csv\" -o res.csv -ord asc col 1\n");
        return 0;
    }

    FILE* input_table = NULL; 
    FILE* output_table = NULL;
    char sep = ',';
    char order = 'n';
    char target;
    int number;
    bool i_passed = false; 
    bool o_passed = false;

    int a = 1; 
    while (a < argc) {

        bool i_arg   = (strcmp(argv[a], "-i") == 0);
        bool o_arg   = (strcmp(argv[a], "-o") == 0);
        bool sep_arg = (strcmp(argv[a], "-sep") == 0);
        bool ord_arg = (strcmp(argv[a], "-ord") == 0);

        if (i_arg || o_arg || sep_arg || ord_arg) {

            if (a + 1 >= argc) {
                highlight_error(argc, argv, a, strlen(argv[a]));
                fprintf(stderr, "Parameter err: Missing argument after flag '%s'.\n", argv[a]);
                if (input_table) fclose(input_table);
                if (output_table) fclose(output_table);
                return -1;
            }

            bool next_is_flag = (strcmp(argv[a+1], "-i") == 0)   ||
                                (strcmp(argv[a+1], "-o") == 0)   ||
                                (strcmp(argv[a+1], "-sep") == 0) ||
                                (strcmp(argv[a+1], "-ord") == 0);
            if (next_is_flag) {
                highlight_error(argc, argv, a, (strlen(argv[a]) + strlen(argv[a+1]) + 1));
                fprintf(stderr, "Parameter err: 2 flags in a row - missing argument for '%s'.\n", argv[a]);
                if (input_table) fclose(input_table);
                if (output_table) fclose(output_table);
                return -1;
            }

            if (argv[a+1][0] == '-') {
                highlight_error(argc, argv, a+1, strlen(argv[a+1]));
                fprintf(stderr, "Parameter err: Unrecognizable flag '%s'.\n", argv[a+1]);
                if (input_table) fclose(input_table);
                if (output_table) fclose(output_table);
                return -1;
            }

            if (i_arg) {

                input_table = fopen(argv[a+1], "r");
                if (!input_table) {
                    fprintf(stderr, "Infile err: No such file '%s' or failed to open it.\n", argv[a+1]);
                    return -1;
                }
                printf("Input file '%s' opened successfully.\n", argv[a+1]);
                a += 2;
                i_passed = true;

            } else if (o_arg) {

                output_table = fopen(argv[a+1], "w");
                if (!output_table) {
                    fprintf(stderr, "Outfile err: Failed to open / create file '%s'.\n", argv[a+1]);
                    return -1;
                }
                printf("Output file '%s' opened / created successfully.\n", argv[a+1]);
                a += 2;
                o_passed = true;

            } else if (sep_arg) {

                if (argv[a+1][0] == 'C' || argv[a+1][0] == 'c') {
                    sep = ',';
                    printf("Separator is comma.\n");
                    a += 2;
                } else if (argv[a+1][0] == 'S' || argv[a+1][0] == 's') {
                    sep = ';';
                    printf("Separator is semicolon.\n");
                    a += 2;
                } else {
                    fprintf(stderr, "Sep err: Separator can be only C (comma) or S (semicolon).\n");
                    if (input_table) fclose(input_table);
                    if (output_table) fclose(output_table);
                    return -1;
                }

            } else if (ord_arg) {

                if (a + 3 >= argc) {
                    highlight_error(argc, argv, a, strlen(argv[a]));
                    fprintf(stderr, "Parameter err: Missing argument after flag '%s'.\n", argv[a]);
                    if (input_table) fclose(input_table);
                    if (output_table) fclose(output_table);
                    return -1;
                }

                a += 1; 
                if (strcmp(argv[a], "asc") == 0) {
                    order = 'a';
                } else if (strcmp(argv[a], "des") == 0) {
                    order = 'd';
                } else {
                    highlight_error(argc, argv, a, strlen(argv[a]));
                    fprintf(stderr, "Parameter err: Check order argument after flag '-ord'.\n");
                    if (input_table) fclose(input_table);
                    if (output_table) fclose(output_table);
                    return -1;
                }

                a += 1; 
                if (strcmp(argv[a], "row") == 0) {
                    target = 'r';
                } else if (strcmp(argv[a], "col") == 0) {
                    target = 'c';
                } else {
                    highlight_error(argc, argv, a, strlen(argv[a]));
                    fprintf(stderr, "Parameter err: Check target argument after flag '-ord'.\n");
                    if (input_table) fclose(input_table);
                    if (output_table) fclose(output_table);
                    return -1;
                }

                a += 1; 
                char *endptr;
                number = (int)strtol(argv[a], &endptr, 10);
                if (argv[a] != endptr && *endptr == '\0') {
                    a += 1; 
                    printf("Order passed successfully.\n");
                } else {
                    highlight_error(argc, argv, a+1, strlen(argv[a+1]));
                    fprintf(stderr, "Parameter err: Check number argument after flag '-ord'.\n");
                    if (input_table) fclose(input_table);
                    if (output_table) fclose(output_table);
                    return -1;
                }
            }

        } else {
            highlight_error(argc, argv, a, strlen(argv[a]));
            fprintf(stderr, "Parameter err: Unrecognizable flag or argument '%s'.\n", argv[a]);
            if (input_table) fclose(input_table);
            if (output_table) fclose(output_table);
            return -1;
        }
    }

    if (i_passed && o_passed) {
        printf("Arguments passed successfully!\nProcessing...\n");
        Table* table = create_empty_table();
        read_CSV(input_table, table, sep);
        eval_table(table);

        int errors_found = 0;
        for (int r = 0; r < table->rows; r++) {
            for (int c = 0; c < table->cols; c++) {
                if (table->grid[r][c].err != ERR_NONE) {
                    errors_found++;
                }
            }
        }
        
        if (errors_found == 0) {
            printf("Computed without errors.\n");
        } else {
            printf("Computed with %d error(s):\n", errors_found);
            for (int r = 0; r < table->rows; r++) {
                for (int c = 0; c < table->cols; c++) {
                    ErrCode ec = table->grid[r][c].err;
                    if (ec != ERR_NONE) {
                        char cell_name[16];
                        coords_to_link(r, c, cell_name);
                        char* err_desc = "Unknown error";
                        switch(ec) {
                            case ERR_CYDE: err_desc = "Cyclic dependence (#CYDE)"; break;
                            case ERR_DIZE: err_desc = "Division by zero (#DIZE)"; break;
                            case ERR_UNFO: err_desc = "Unexpected format / Syntax error (#UNFO)"; break;
                            case ERR_FNAM: err_desc = "Unknown function name (#FNAM)"; break;
                            case ERR_ICRD: err_desc = "Invalid coordinate / Out of bounds (#ICRD)"; break;
                            default: break;
                        }
                        printf("! Error: cell %s: %s\n", cell_name, err_desc);
                    }
                }
            }
        }

        if (order != 'n') {
            printf("Sorting...\n");
            sort_table(table, order, target, number);
        }
        write_CSV(output_table, table, sep);
        printf("Done.\n");
        free_table(table);
        
    } else {
        fprintf(stderr, "Cannot start.\nMissing required arguments. Please pass both arguments '-i' and '-o'\n");
    }

    if (input_table) fclose(input_table);
    if (output_table) fclose(output_table);
    return 0;
}