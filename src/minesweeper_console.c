#include "minesweeper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main() {
    Game game = {0};
    char command[64];
    char *token = NULL;
    char *saveptr = NULL;
    int x = -1;
    int y = -1;
    if (gen(&game, 16, 16, 7) != 0) {
        fprintf(stderr, "Failed to create game field.\n");
        return 1;
    }
    while (1) {
        print_field(&game, game.state == GAME_LOSE);
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
        y = -1;
        if (!strcmp(token, "flag") || !strcmp(token, "open")) {
            char *cm = strdup(token);
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                x = atoi(token) - 1;
            }
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                y = atoi(token) - 1;
            }
            if (!strcmp(cm, "flag")) {
                set_flag(&game, x, y);
            } else {
                open_cell(&game, x, y);
            }
        } else if (!strcmp(token, "hint")) {
            Hint h = giveHint(&game);
            if (h.action == FLAG) {
                printf("Place flag on %d %d cell\n", h.x, h.y);
            } else if (h.action == OPEN) {
                printf("u can open %d %d cell\n", h.x, h.y);
            } else {
                printf("There is zero safe ways\n");
            }
        } else if (!strcmp(token, "exit")) {
            break;
        }
    }

    free_field(&game);
    return 0;
}
// cc minesweeper_console.c minesweeper.c ../readyToUse/lcg.c -o minesweeper_console
// ./minesweeper_console