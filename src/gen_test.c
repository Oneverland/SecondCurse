#include <stdlib.h>
#include <stdio.h>
#include "lcg.h"
#include "minesweeper.h"
#include <string.h>
#include <time.h>

static void free_generated_rows(Game *game, int allocated_rows) {
    if (game == NULL || game->field == NULL) {
        return;
    }
    for (int i = 0; i < allocated_rows; i++) {
        free(game->field[i]);
    }
    free(game->field);
    game->field = NULL;
}

int gen_old(Game *game, int height, int width, int density) {
    if (game == NULL || height <= 0 || width <= 0 || density <= 0) {
        return -1;
    }
    free_field(game);
    game->field = NULL;
    game->height = height;
    game->width = width;
    game->density = density;
    game->mine_count = 0;
    game->state = GAME_RUNNING;
    lcgs();
    game->field = malloc(height * sizeof(Cell *));
    if (game->field == NULL) {
        game->state = GAME_ERROR;
        return -1;
    }
    for (int i = 0; i < height; i++) {
        game->field[i] = malloc(width * sizeof(Cell));
        if (game->field[i] == NULL) {
            free_generated_rows(game, i);
            game->state = GAME_ERROR;
            return -1;
        }
    }
    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            game->field[i][k].isOpen = 0;
            game->field[i][k].isFlag = 0;
            if (lcg_easy() % density == 0) {
                game->mine_count++;
                game->field[i][k].isMine = 1;
            } else {
                game->field[i][k].isMine = 0;
            }
        }
    } // дооптимизировать и анализировать. нужно за один проход и инициализировать, и мины вокруг считать. Проанализировать это в виде тестов на больших полях и замере (ну то есть первый вариант замерить, второй вариант замерить). и раздел в отчет добавить. 
    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            game->field[i][k].minesAround = 0;
            if (game->field[i][k].isMine != 1) {
                if ((i - 1 >= 0) && (k - 1 >= 0) && (game->field[i - 1][k - 1].isMine == 1)) {game->field[i][k].minesAround++;}
                if ((i - 1 >= 0) && (game->field[i - 1][k].isMine == 1)) {game->field[i][k].minesAround++;}
                if ((i - 1 >= 0) && (k + 1 < width) && (game->field[i - 1][k + 1].isMine == 1)) {game->field[i][k].minesAround++;}
                if ((k - 1 >= 0) && (game->field[i][k - 1].isMine == 1)) {game->field[i][k].minesAround++;}
                if ((k + 1 < width) && (game->field[i][k + 1].isMine == 1)) {game->field[i][k].minesAround++;}
                if ((i + 1 < height) && (k - 1 >= 0) && (game->field[i + 1][k - 1].isMine == 1)) {game->field[i][k].minesAround++;}
                if ((i + 1 < height) && (game->field[i + 1][k].isMine == 1)) {game->field[i][k].minesAround++;}
                if ((i + 1 < height) && (k + 1 < width) && (game->field[i + 1][k + 1].isMine == 1)) {game->field[i][k].minesAround++;}
            }
        }
    }
    return 0;
}

int gen_new(Game *game, int height, int width, int density) {
    if (game == NULL || height <= 0 || width <= 0 || density <= 0) {
        return -1;
    }
    free_field(game);
    game->field = NULL;
    game->height = height;
    game->width = width;
    game->density = density;
    game->mine_count = 0;
    game->state = GAME_RUNNING;
    lcgs();
    game->field = calloc(height, sizeof(Cell*));
    if (game->field == NULL) {
        game->state = GAME_ERROR;
        return -1;
    }
    for (int i = 0; i < height; i++) {
        game->field[i] = calloc(width, sizeof(Cell));
        if (game->field[i] == NULL) {
            free_generated_rows(game, i);
            game->state = GAME_ERROR;
            return -1;
        }
    }
    const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            if (lcg_easy() % density == 0) {
                game->mine_count++;
                game->field[i][k].isMine = 1;
                for (int j = 0; j < 8; j++) {
                    int nx = i + directions[j][0];
                    int ny = k + directions[j][1];
                    if (nx < 0 || nx >= game->height || ny < 0 || ny >= game->width || game->field[nx][ny].isMine == 1) {
                        continue;
                    }
                    game->field[nx][ny].minesAround++;
                }
                game->field[i][k].minesAround = 0;
            } else {
                game->field[i][k].isMine = 0;
            }
        }
    }
    return 0;
}



int gen_new1(Game *game, int height, int width, int density) {
    if (game == NULL || height <= 0 || width <= 0 || density <= 0) {
        return -1;
    }
    free_field(game);
    game->field = NULL;
    game->height = height;
    game->width = width;
    game->density = density;
    game->mine_count = 0;
    game->state = GAME_RUNNING;
    lcgs();
    game->field = calloc(height, sizeof(Cell*));
    if (game->field == NULL) {
        game->state = GAME_ERROR;
        return -1;
    }
    Cell *data = calloc(height * width, sizeof(Cell));
    if (game->field == NULL || data == NULL) {
        free(game->field);
        game->field = NULL;
        game->state = GAME_ERROR;
        return -1;
    }
    for (int i = 0; i < height; i++) {
        game->field[i] = data + i * width;
    }
    const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            if (lcg_easy() % density == 0) {
                game->mine_count++;
                game->field[i][k].isMine = 1;
                for (int j = 0; j < 8; j++) {
                    int nx = i + directions[j][0];
                    int ny = k + directions[j][1];
                    if (nx < 0 || nx >= game->height || ny < 0 || ny >= game->width || game->field[nx][ny].isMine == 1) {
                        continue;
                    }
                    game->field[nx][ny].minesAround++;
                }
                game->field[i][k].minesAround = 0;
            } else {
                game->field[i][k].isMine = 0;
            }
        }
    }
    return 0;
}


static void print_field(const Game *game, int reveal_mines) {
    printf("00  ");
    for (int i = 0; i < game->width; i++) {
        printf("%.2d  ", i + 1);
    }
    printf("\n");
    for (int i = 0; i < game->height; i++) {
        printf("%.2d ", i + 1);
        for (int k = 0; k < game->width; k++) {
            if (game->field[i][k].isOpen == 1) {
                // printf("[%d] ", game->field[i][k].minesAround);
                printf("[ ] ");
            } else if (reveal_mines && game->field[i][k].isMine == 1) {
                printf("[X] ");
            } else if (game->field[i][k].isFlag == 1) {
                printf("[F] ");
            } else {
                // printf("[ ] ");
                printf("[%d] ", game->field[i][k].minesAround);
            }
        }
        printf("\n");
    }
}

int main() {
    Game game = {0};
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 1000; i++) {
        gen_old(&game, 1000, 100, 10);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double alltime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%f\n", alltime);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 1000; i++) {
        gen_new(&game, 1000, 100, 10);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    alltime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%f\n", alltime);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 1000; i++) {
        gen_new1(&game, 1000, 100, 10);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    alltime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%f\n", alltime);

    // char command[64];
    // char *token = NULL;
    // char *saveptr = NULL;
    // int x = -1;
    // int y = -1;
    // if (gen_new(&game, 8, 6, 7) != 0) {
    //     fprintf(stderr, "Failed to create game field.\n");
    //     return 1;
    // }
    // while (1) {
    //     print_field(&game, 1);
    //     if (game.state == GAME_WIN) {
    //         printf("NAH NO WAY U WIN\n");
    //         break;
    //     }
    //     if (game.state == GAME_LOSE) {
    //         printf("U LOSE HAHAHAHA\n");
    //         break;
    //     }
    //     if (fgets(command, sizeof(command), stdin) == NULL) {
    //         break;
    //     }
    //     saveptr = NULL;
    //     token = strtok_r(command, " \n", &saveptr);
    //     if (token == NULL) {
    //         continue;
    //     }
    //     x = -1;
    //     y = -1;
    //     if (!strcmp(token, "flag") || !strcmp(token, "open")) {
    //         char *cm = strdup(token);
    //         token = strtok_r(NULL, " \n", &saveptr);
    //         if (token != NULL) {
    //             x = atoi(token) - 1;
    //         }
    //         token = strtok_r(NULL, " \n", &saveptr);
    //         if (token != NULL) {
    //             y = atoi(token) - 1;
    //         }
    //         if (!strcmp(cm, "flag")) {
    //             set_flag(&game, x, y);
    //         } else {
    //             open_cell(&game, x, y);
    //         }
    //     } else if (!strcmp(token, "exit")) {
    //         break;
    //     }
    // }

    free_field(&game);
    return 0;
}


//  [X] [!] [!]
//  [!] [.] [ ]
//  [ ] [ ] [ ]

//  [!] [!] [!]
//  [!] [X] [ ]
//  [ ] [ ] [ ]
// cc gen_test.c minesweeper.c ../readyToUse/lcg.c -o gen_test