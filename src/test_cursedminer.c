#include <stdio.h>
#include <stdlib.h>
#include "/Users/oneverland/C_Projects/readyToUse/lcg.c"

#define HEIGHT 16
#define WIDTH 16
#define DENSITY 7


int count_of_mine = 0;


typedef struct cursedminer {
    int isOpen;
    int isMine;
    int isFlag;
    int minesAround;
} Cell;


typedef enum {
    NONE,
    OPEN,
    FLAG
} Action;


typedef struct {
    Action action;
    int x;
    int y;
} Hint;


int gen(Cell pole[HEIGHT][WIDTH]) {
    count_of_mine = 0;
    for (int i = 0; i < HEIGHT; i++) {
        for (int k = 0; k < WIDTH; k++) {
            pole[i][k].isOpen = 0;
            pole[i][k].isFlag = 0;
            if (lcg_easy() % DENSITY == 0) {
                count_of_mine++;
                pole[i][k].isMine = 1;
            } else {
                pole[i][k].isMine = 0;
            }
        }
    }

    for (int i = 0; i < HEIGHT; i++) {
        for (int k = 0; k < WIDTH; k++) {
            pole[i][k].minesAround = 0;
            if (pole[i][k].isMine != 1) {
                if ((i - 1 >= 0) && (k - 1 >= 0) && (pole[i-1][k-1].isMine == 1)) {pole[i][k].minesAround++;}
                if ((i - 1 >= 0) && (pole[i-1][k].isMine == 1)) {pole[i][k].minesAround++;}
                if ((i - 1 >= 0) && (k + 1 < WIDTH) && (pole[i-1][k+1].isMine == 1)) {pole[i][k].minesAround++;}
                if ((k - 1 >= 0) && (pole[i][k-1].isMine == 1)) {pole[i][k].minesAround++;}
                if ((k + 1 < WIDTH) && (pole[i][k+1].isMine == 1)) {pole[i][k].minesAround++;}
                if ((i + 1 < HEIGHT) && (k - 1 >= 0) && (pole[i+1][k-1].isMine == 1)) {pole[i][k].minesAround++;}
                if ((i + 1 < HEIGHT) && (pole[i+1][k].isMine == 1)) {pole[i][k].minesAround++;}
                if ((i + 1 < HEIGHT) && (k + 1 < WIDTH) && (pole[i+1][k+1].isMine == 1)) {pole[i][k].minesAround++;}
            }
        }
    }
    return 0;
}


void print_field(Cell pole[HEIGHT][WIDTH]) {
    printf("00  ");
    for (int i = 0; i < WIDTH; i++) {printf("%.2d  ", i+1);}
    printf("\n");
    for (int i = 0; i < HEIGHT; i++) {
        printf("%.2d ", i+1);
        for (int k = 0; k < WIDTH; k++) {
            if (pole[i][k].isOpen == 1) {
                printf("[%d] ", pole[i][k].minesAround);
            } 
            // else if (pole[i][k].isMine == 1) { // показать мины
            //     printf("[X] ");
            // } 
            else if (pole[i][k].isFlag == 1) {
                printf("[F] ");
            } else {
                printf("[ ] ");
            }
        }
        printf("\n");
    }
}


void set_flag(int x, int y, Cell pole[HEIGHT][WIDTH]) {
    if ((x >= 0) && (x < HEIGHT) && (y >= 0) && (y < WIDTH)) {
        if (pole[x][y].isFlag == 0) {
            if (pole[x][y].isOpen == 1) {return;}
            pole[x][y].isFlag = 1;
        } else {
            pole[x][y].isFlag = 0;
        }
    }
    int opened_cells = 0;
    for (int i = 0; i < HEIGHT; i++) {
        for (int k = 0; k < WIDTH; k++) {
            opened_cells += pole[i][k].isOpen;
        }
    }
    if ((HEIGHT * WIDTH - count_of_mine) == opened_cells) {
        printf("NAH NO WAY U WIN\n");
    }
}

void open_cell(int x, int y, Cell pole[HEIGHT][WIDTH]) {
    if ((x >= 0) && (x < HEIGHT) && (y >= 0) && (y < WIDTH)) {
        pole[x][y].isOpen = 1;
        pole[x][y].isFlag = 0;
        if (pole[x][y].isMine == 1) {
            printf("U LOSE HAHAHAHA\n");
            exit(0);
        } else if (pole[x][y].minesAround == 0) {
            if ((x - 1 >= 0) && (y - 1 >= 0) && (pole[x-1][y-1].isOpen == 0)) {open_cell(x-1, y-1, pole);}
            if ((x - 1 >= 0) && (pole[x-1][y].isOpen == 0)) {open_cell(x-1, y, pole);}
            if ((x - 1 >= 0) && (y + 1 < WIDTH) && (pole[x-1][y+1].isOpen == 0)) {open_cell(x-1, y+1, pole);}
            if ((y - 1 >= 0) && (pole[x][y-1].isOpen == 0)) {open_cell(x, y-1, pole);}
            if ((y + 1 < WIDTH) && (pole[x][y+1].isOpen == 0)) {open_cell(x, y+1, pole);}
            if ((x + 1 < HEIGHT) && (y - 1 >= 0) && (pole[x+1][y-1].isOpen == 0)) {open_cell(x+1, y-1, pole);}
            if ((x + 1 < HEIGHT) && (pole[x+1][y].isOpen == 0)) {open_cell(x+1, y, pole);}
            if ((x + 1 < HEIGHT) && (y + 1 < WIDTH) && (pole[x+1][y+1].isOpen == 0)) {open_cell(x+1, y+1, pole);}
        }
        int opened_cells = 0;
        for (int i = 0; i < HEIGHT; i++) {
            for (int k = 0; k < WIDTH; k++) {
                opened_cells += pole[i][k].isOpen;
            }
        }
        if ((HEIGHT * WIDTH - count_of_mine) == opened_cells) {
            printf("NAH NO WAY U WIN\n");
            exit(0);
        }
    }
}


Hint giveHint(const Cell pole[HEIGHT][WIDTH]) {
    static const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    for (int x = 0; x < HEIGHT; x++) {
        for (int y = 0; y < WIDTH; y++) {
            if (pole[x][y].isOpen == 0 || pole[x][y].minesAround == 0) {
                continue;
            }
            int flags = 0;
            int hidden = 0;
            int hidden_x[8];
            int hidden_y[8];
            for (int i = 0; i < 8; i++) {
                int nx = x + directions[i][0];
                int ny = y + directions[i][1];
                if (nx < 0 || nx >= HEIGHT || ny < 0 || ny >= WIDTH) {
                    continue;
                }
                if (pole[nx][ny].isFlag == 1) {
                    flags++;
                } else if (pole[nx][ny].isOpen == 0) {
                    hidden_x[hidden] = nx;
                    hidden_y[hidden] = ny;
                    hidden++;
                }
            }
            if (hidden == 0) {
                continue;
            }
            if (flags == pole[x][y].minesAround) {
                return (Hint){OPEN, hidden_x[0] + 1, hidden_y[0] + 1};
            }
            if (flags + hidden == pole[x][y].minesAround) {
                return (Hint){FLAG, hidden_x[0] + 1, hidden_y[0] + 1};
            }
        }
    }
    return (Hint){NONE, -1, -1};
}


// визуала навалить, RTX :) отслеживание мыши.

int main() {
    lcgs();
    int height, width, density;
    // продолжить проработку выбора параметров генерации поля
    Cell field[HEIGHT][WIDTH];
    gen(field);
    char command[30];
    char *token = NULL, *saveptr = NULL;
    int x = -1, y = -1;
    while (1) {
        x = -1, y = -1;
        print_field(field);
        if (fgets(command, sizeof(command), stdin) == NULL) {break;}
        token = strtok_r(command, " \n", &saveptr);
        if (token == NULL) {
            continue;
        }
        if (!strcmp(token, "flag")) {
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                x = atoi(token) - 1;
            }
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                y = atoi(token) - 1;
            }
            set_flag(x, y, field);
        } else if (!strcmp(token, "open")) {
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                x = atoi(token) - 1;
            }
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                y = atoi(token) - 1;
            }
            open_cell(x, y, field);
        } else if (!strcmp(token, "hint")) {
            Hint h = giveHint(field);
            if (h.action == FLAG) {
                printf("Place flag on %d %d cell\n", h.x, h.y);
            } else if (h.action == OPEN) {
                printf("u can open %d %d cell\n", h.x, h.y);
            } else {
                printf("There is zero safe ways\n");
            }
        } else if (!strcmp(token, "exit")) {
            exit(0);
        }
    } 
    return 0;
}
