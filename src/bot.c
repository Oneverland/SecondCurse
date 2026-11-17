#include "minesweeper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcg.h"

void print_field(const Game *game, int reveal_mines) {
    printf("00  ");
    for (int i = 0; i < game->width; i++) {
        printf("%.2d  ", i + 1);
    }
    printf("\n");
    for (int i = 0; i < game->height; i++) {
        printf("%.2d ", i + 1);
        for (int k = 0; k < game->width; k++) {
            if (game->field[i][k].isOpen == 1) {
                printf("[%d] ", game->field[i][k].minesAround);
            } else if (reveal_mines && game->field[i][k].isMine == 1) {
                printf("[X] ");
            } else if (game->field[i][k].isFlag == 1) {
                printf("[F] ");
            } else {
                printf("[ ] ");
            }
        }
        printf("\n");
    }
}


int bot_step_one(Game *game, int V) {
    Hint step = giveHint(game);
    step.x--;
    step.y--;
    if (V == 1) {printf("%s %d %d\n", step.action == FLAG ? "FLAG" : "OPEN", step.x, step.y);}
    if (step.action == FLAG) {
        set_flag(game, step.x, step.y);
        if (V == 1) {print_field(game, 0);}
    } else if (step.action == OPEN) {
        open_cell(game, step.x, step.y);
        if (V == 1) {print_field(game, 0);}
    } else {
        printf("breakpoint, can't move forward\n");
        if (V == 1) {print_field(game, 0);}
        return -1;
    }
    return 0;
}

int bot_steps(Game *game, int x, int V) {
    if (x <= 0) return -1;

    if (x < 50) {
        while (x != 0) {
            int result = bot_step_one(game, V);
            x--;
            if (result == -1 && game->state != GAME_RUNNING) {
                printf("problem in loop \"<50\"\n");
                return -1;
            }
        }
    } else {
        while (game->state == GAME_RUNNING) {
            int result = bot_step_one(game, V);
            if (result == -1) {
                printf("problem in loop \"while(1)\"\n");
                return -1;
            }
        }
    }
    return 0;
}
// сделал первую версию, не умеет пока ничего кроме решения пошагово (может попасть в тупик не из-за рандома, нужен апгрейд giveHint)
// 
int main(void) {
    lcgs();
    Game game = {0};
    char command[64];
    char *token = NULL;
    char *saveptr = NULL;
    int x = -1;
    int V = -1;
    if (gen(&game, 16, 16, 7) != 0) {
        fprintf(stderr, "Failed to create game field.\n");
        return 1;
    }
    while (open_cell(&game, 8, 8) == GAME_LOSE || count_opened_cells(&game) == 1) {
        printf("unluck...\n");
        free_field(&game);
        if (gen(&game, 16, 16, 7) != 0) {
            fprintf(stderr, "Failed to create game field.\n");
            return 1;
        }
    }
    while (1) {
        // print_field(&game, game.state == GAME_LOSE);
        if (game.state == GAME_WIN) {
            printf("NAH NO WAY U WIN\n");
            break;
        }
        if (game.state == GAME_LOSE) {
            printf("U LOSE HAHAHAHA\n");
            break;
        }
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        saveptr = NULL;
        token = strtok_r(command, " \n", &saveptr);
        if (token == NULL) {
            continue;
        }
        x = -1;
        V = -1;
        if (!strcmp(token, "step")) {
            char *cm = strdup(token);
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                V = atoi(token);
            }
            bot_step_one(&game, V);
        } else if (!strcmp(token, "steps")) {
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                x = atoi(token);
            }
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                V = atoi(token);
            }
            bot_steps(&game, x, V);
        } else if (!strcmp(token, "exit")) {
            break;
        }
    }

    free_field(&game);
    return 0;
}

// cc bot.c minesweeper.c lcg.c -o ../build/bot
// ../build/bot