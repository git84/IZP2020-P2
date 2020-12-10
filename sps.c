#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    int cell_size;
    char *data;
} t_cell; // Cell of the table

typedef struct {
    int rows;
    int cols;
    t_cell *cells;
} t_table; // Table structure

typedef struct {
    const char *delims;
    const char *file_name;
    const char *cmd_string;
} t_config; // Application configuration derived from args

typedef struct {
    int start_row;
    int start_col;
    int end_row;
    int end_col;
} t_select;


void get_config(t_config *config, int argc, char **argv);
void init_table(const t_config *config,t_table *tbl);
bool is_delim(const t_config *config, char c);
t_cell* get_cell(t_table *table, int row, int col);
void parse_table(t_table *tbl, const t_config *config);
void dump_table(t_table *tbl);
void insert_rows(t_table *tbl, int before, int count);
void insert_cols(t_table *tbl, int before, int count);
void delete_rows(t_table *tbl, int after, int count);
void delete_cols(t_table *tbl, int after, int count);
t_select parse_select(const char *cmd_string, t_table *tbl);
void run_commands(const t_config *config, t_table *tbl);
void write_buf(t_table *tbl, const char buf[], int row, int col);
void write_table(t_table *tbl, const t_config *config);


int main(int argc, char **argv) {
    t_config app_config;
    get_config(&app_config, argc, argv);

    t_table table;
    init_table(&app_config, &table);
    parse_table(&table, &app_config);
    //dump_table(&table);
    //printf("\n \n");

    run_commands(&app_config, &table);
    //dump_table(&table);

    write_table(&table, &app_config);

    return 0;
}

void get_config(t_config *config, int argc, char **argv) {
    config->delims = NULL;
    config->file_name = NULL;
    config->cmd_string = NULL;

    if(argc < 3) {
        return;
    }

    if(strcmp(argv[1], "-d") == 0) {
        if(argc < 5) {
            return;
        }
        config->delims = argv[2];
        config->cmd_string = argv[3];
        config->file_name = argv[4];
    } else {
        config->cmd_string = argv[1];
        config->file_name = argv[2];
    }
}

void init_table(const t_config *config,t_table *tbl) {
    tbl->rows = 0;
    tbl->cols = 0;
    tbl->cells = NULL;

    FILE *fp = fopen(config->file_name, "r");
    if(fp==NULL) {
        return;
    }

    bool in_parentheses = false;
    char prev = 0;
    int max_cols = 0;
    int current_cols = 0;
    int rows = 0;

    int c;
    while((c = fgetc(fp)) != EOF) {
        if(in_parentheses) {
            if(c == '"' && prev != '\\')
                in_parentheses = false;
        } else if(c == '"' && prev != '\\') {
            in_parentheses = true;
        } else if(is_delim(config, c) && prev != '\\') {
            current_cols++;
        } else if(c == '\n') {
            rows++;
            current_cols++;
            if(current_cols > max_cols) {
                max_cols = current_cols;
            }
            current_cols = 0;
        }
        prev = c;
    }

    fclose(fp);

    tbl->rows = rows;
    tbl->cols = max_cols;
    //printf("rows %d cols %d\n",rows ,max_cols);

    tbl->cells = malloc(sizeof(t_cell) * rows * max_cols);
    if(tbl->cells == NULL) {
        return;
    }
    memset(tbl->cells, 0, sizeof(t_cell) * rows * max_cols);

}

bool is_delim(const t_config *config, char c) {
    if(config->delims == NULL) {
        return c == ' ';
    }

    const char *delim = config->delims;
    while(*delim != '\0') {
        if(c == *delim){
            return true;
        }
        delim++;

    }
    return false;
}

t_cell* get_cell(t_table *table, int row, int col){
    return &table->cells[row * table->cols + col];
}

void parse_table(t_table *tbl, const t_config *config){
    FILE *fp = fopen(config->file_name, "r");
    if(fp==NULL) {
        return;
    }

    bool in_parentheses = false;
    char prev = 0;
    int col = 0;
    int row = 0;
    char buffer[1000];
    int buffer_pos = 0;

    int c;
    while((c = fgetc(fp)) != EOF) {
        if(c == '"' && prev != '\\'){
            buffer[buffer_pos] = c;
            buffer_pos++;
            if(in_parentheses) {
                in_parentheses = false;
            } else {
                in_parentheses = true;
            }
        } else if((is_delim(config, c) && prev != '\\' && !in_parentheses) || c == '\n') {
            t_cell *cell = get_cell(tbl, row, col);
            cell->cell_size = buffer_pos;
            cell->data = malloc(buffer_pos * sizeof(char));
            if(cell->data == NULL){
                return;
            }
            memcpy(cell->data, buffer, buffer_pos);
            buffer_pos = 0;
            if(c == '\n'){
                col = 0;
                row++;
            } else {
                col++;
            }
        } else {
            buffer[buffer_pos] = c;
            buffer_pos++;
        }

        prev = c;
        if(buffer_pos > 1000){
            return;
        }
    }

    fclose(fp);

}

void dump_table(t_table *tbl) {
    printf("rows %d cols %d\n", tbl->rows, tbl->cols);
    for(int i = 0; i < tbl->rows; i++) {
        for(int j = 0; j < tbl->cols; j++) {
            char dump[1001] = {0};
            t_cell *cell = get_cell(tbl, i, j);
            memcpy(dump, cell->data, cell->cell_size);
            printf("%d:%s\t", j, dump);
        }
        printf("\n");
    }
}

void insert_rows(t_table *tbl, int before, int count) {
    tbl->cells = realloc(tbl->cells, sizeof(t_cell) * (tbl->rows + count) * tbl->cols);
    if(tbl->cells == NULL) {
        return;
    }

    t_cell *cut = tbl->cells + (before * tbl->cols);
    t_cell *dest = cut + (count * tbl->cols);
    memmove(dest, cut, (tbl->rows - before) * tbl->cols * sizeof(t_cell));
    memset(cut, 0, count * tbl->cols * sizeof(t_cell));
    tbl->rows = tbl->rows + count;
}

void insert_cols(t_table *tbl, int before, int count) {
    tbl->cells = realloc(tbl->cells, sizeof(t_cell) * (tbl->rows) * (tbl->cols + count));
    if(tbl->cells == NULL) {
        return;
    }

    t_cell *row_start = tbl->cells;
    for(int i = 0; i < tbl->rows; i++) {
        t_cell *cut = row_start + before;
        memmove(cut + count, cut, (((tbl->cols - before) + ((tbl->rows - i - 1) * tbl->cols))) * sizeof(t_cell));
        memset(cut, 0, count * sizeof(t_cell));
        row_start = row_start + tbl->cols + count;
    }
    tbl->cols = tbl->cols + count;
}

void delete_rows(t_table *tbl, int after, int count) {
    t_cell *cut = tbl->cells + (after * tbl->cols);
    t_cell *dest = cut + (count * tbl->cols);
    memset(cut, 0, count * tbl->cols * sizeof(t_cell));
    memmove(cut, dest, (tbl->rows - (after + count)) * tbl->cols * sizeof(t_cell));

    tbl->rows = tbl->rows - count;
}

void delete_cols(t_table *tbl, int after, int count) {
    t_cell *row_start = tbl->cells;
    for(int i = 0; i < tbl->rows; i++) {
        t_cell *cut = row_start + after;
        memset(cut, 0, count * sizeof(t_cell));
        memmove(cut, cut+count, (((tbl->cols - count) + ((tbl->rows - i - 1) * tbl->cols))) * sizeof(t_cell));
        row_start = row_start + (tbl->cols - count);
    }
    tbl->cols = tbl->cols - count;
}

t_select parse_select(const char *cmd_string, t_table *tbl) {
    t_select sel;
    sel.start_row = -1;
    sel.start_col = -1;
    sel.end_row = -1;
    sel.end_col = -1;

    if(sscanf(cmd_string, "[%d,%d,%d,%d]", &sel.start_row, &sel.start_col, &sel.end_row, &sel.end_col) == 4) {
        sel.start_row--;
        sel.start_col--;
        sel.end_row--;
        sel.end_col--;
        return sel;
    }

    if(sscanf(cmd_string, "[%d,%d]", &sel.start_row, &sel.start_col) == 2) {
        sel.start_row--;
        sel.start_col--;
        sel.end_col = sel.start_col;
        sel.end_row = sel.start_row;
        return sel;
    }

    if(sscanf(cmd_string, "[_,%d]", &sel.start_col) == 1) {
        sel.start_col--;
        sel.start_row = 0;
        sel.end_row = tbl->rows - 1;
        sel.end_col = sel.start_col;
        return sel;
    }

    if(sscanf(cmd_string, "[%d,_]", &sel.start_row) == 1) {
        sel.start_row--;
        sel.start_col = 0;
        sel.end_col = tbl->cols - 1;
        sel.end_row = sel.start_row;
        return sel;
    }

    if(strncmp("[_,_]", cmd_string, 5) == 0) {
        sel.start_row = 0;
        sel.start_col = 0;
        sel.end_row = tbl->rows - 1;
        sel.end_col = tbl->cols - 1;
        return sel;
    }
    return sel;

}

void run_commands(const t_config *config, t_table *tbl) {
    const char *cmd_str = config->cmd_string;
    t_select sel;

    while (*cmd_str != 0) {
        int row;
        int col;
        if(strncmp("irow", cmd_str, 4) == 0) {
            insert_rows(tbl, sel.start_row, 1);
        } else if(strncmp("arow", cmd_str, 4) == 0) {
            insert_rows(tbl, sel.end_row + 1, 1);
        } else if(strncmp("drow", cmd_str, 4) == 0) {
            delete_rows(tbl, sel.start_row, sel.end_row - sel.start_row + 1);
        } else if(strncmp("icol", cmd_str, 4) == 0) {
            insert_cols(tbl, sel.start_col, 1);
        } else if(strncmp("acol", cmd_str, 4) == 0) {
            insert_cols(tbl, sel.end_col + 1, 1);
        } else if(strncmp("dcol", cmd_str, 4) == 0) {
            delete_cols(tbl, sel.start_col, sel.end_col - sel.start_col + 1);
        } else if(sscanf(cmd_str, "sum [%d,%d]", &row, &col) == 2) {
            row--;
            col--;
            double sum = 0;
            for(int i = sel.start_row; i <= sel.end_row; i++) {
                for(int j = sel.start_col; j <= sel.end_col; j++) {
                    char buf[1001] = {0};
                    strncpy(buf, get_cell(tbl,i,j)->data, get_cell(tbl,i,j)->cell_size);
                    sum += strtod(buf, 0);
                }
            }
            char buf[100];
            snprintf(buf, 100, "%g", sum);
            write_buf(tbl, buf, row, col);
        } else if(sscanf(cmd_str, "avg [%d,%d]", &row, &col) == 2) {
            row--;
            col--;
            double avg;
            double sum = 0;
            for(int i = sel.start_row; i <= sel.end_row; i++) {
                for(int j = sel.start_col; j <= sel.end_col; j++) {
                    char buf[1001] = {0};
                    strncpy(buf, get_cell(tbl,i,j)->data, get_cell(tbl,i,j)->cell_size);
                    sum += strtod(buf, 0);
                }
            }
            avg = sum / (double)((sel.end_row - sel.start_row + 1) * (sel.end_col - sel.start_col + 1));
            char buf[100];
            snprintf(buf, 100, "%g", avg);
            write_buf(tbl, buf, row, col);

        } else if(sscanf(cmd_str, "count [%d,%d]", &row, &col) == 2) {
            row--;
            col--;
            int count = 0;
            for(int i = sel.start_row; i <= sel.end_row; i++) {
                for(int j = sel.start_col; j <= sel.end_col; j++) {
                    if(get_cell(tbl, i, j)->cell_size != 0) {
                        count++;
                    }
                }
            }
            char buf[100];
            snprintf(buf, 100, "%d", count);
            write_buf(tbl, buf, row, col);

        } else if(sscanf(cmd_str, "len [%d,%d]", &row, &col) == 2) {
            row--;
            col--;
            int size = get_cell(tbl, sel.start_row, sel.start_col)->cell_size;
            char buf[100];
            snprintf(buf, 100, "%d", size);
            write_buf(tbl, buf, row, col);

        } else if(sscanf(cmd_str, "swap [%d,%d]", &row, &col) == 2) {
            row--;
            col--;
            t_cell *cell1 = get_cell(tbl, sel.start_row, sel.start_col);
            t_cell *cell2 = get_cell(tbl, row, col);
            int size = cell1->cell_size;
            char *data = cell1->data;
            cell1->cell_size = cell2->cell_size;
            cell1->data = cell2->data;
            cell2->cell_size = size;
            cell2->data = data;

        } else if(strncmp(cmd_str, "set ", 4) == 0) {
            cmd_str += 4;
            char buf[1001];
            int buf_pos = 0;
            while(*cmd_str != ';' && *cmd_str != 0 && buf_pos < 1001) {
                buf[buf_pos] = *cmd_str;
                cmd_str++;
                buf_pos++;
            }
            buf[buf_pos] = 0;
            write_buf(tbl, buf, sel.start_row, sel.start_col);
        } else if(strncmp(cmd_str, "clear", 5) == 0) {
            for(int i = sel.start_row; i <= sel.end_row; i++) {
                for(int j = sel.start_col; j <= sel.end_col; j++) {
                    get_cell(tbl, i, j)->cell_size = 0;
                }
            }
        } else {
            sel = parse_select(cmd_str, tbl);

            if(sel.start_col == -1) {
                return;
            }

            if(sel.end_row > tbl->rows - 1) {
                insert_rows(tbl, tbl->rows, sel.end_row - tbl->rows);
            }

            if(sel.end_col > tbl->cols - 1) {
                insert_cols(tbl, tbl->cols, sel.end_col - tbl->cols);
            }
        }

        while(*cmd_str != ';' && *cmd_str != '\0') {
            cmd_str++;
        }

        if (*cmd_str == '\0') {
            return;
        }

        cmd_str++;
    }
}

void write_buf(t_table *tbl, const char buf[], int row, int col) {
    t_cell *cell = get_cell(tbl, row, col);

    cell->cell_size = strlen(buf);
    cell->data = malloc(sizeof(char) * cell->cell_size);
    strncpy(cell->data, buf, cell->cell_size);
}

void write_table(t_table *tbl, const t_config *config){
    FILE *fp = fopen(config->file_name, "w");
    if(fp==NULL) {
        return;
    }

    char delim = ' ';
    if(config->delims != NULL) {
        delim = *config->delims;
    }

    for(int i = 0; i < tbl->rows; i++) {
        for(int j = 0; j < tbl->cols; j++) {
            t_cell *cell = get_cell(tbl, i, j);
            fwrite(cell->data, 1, cell->cell_size, fp);
            if(j != tbl->cols - 1)  {
                fwrite(&delim, 1, 1, fp);
            }
        }
        fwrite("\n", 1, 1, fp);
    }

    fclose(fp);
}
