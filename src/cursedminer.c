#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "lcg.h"
#include <time.h>

#define HEIGHT 16
#define WIDTH 16
#define DENSITY 30


int count_of_mine = 0;


typedef struct cursedminer {
    int isOpen;
    int isMine;
    int isFlag;
    int minesAround;
} Cell;


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
            else if (pole[i][k].isMine == 1) {
                printf("[X] ");
            } 
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

int main() {
    lcgs();
    Cell field[HEIGHT][WIDTH];
    gen(field);
    char command[30];
    char *token = NULL, *saveptr = NULL;
    int x = -1, y = -1;
    while (true) {
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
        } else if (!strcmp(token, "exit")) {
            exit(0);
        }
    } 
    return 0;
}

