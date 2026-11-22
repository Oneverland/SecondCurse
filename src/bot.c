#include "minesweeper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcg.h"

Hint giveHintPlus(const Game *game);

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
    Hint step = giveHintPlus(game);
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
                return -1;
            }
        }
    } else {
        while (game->state == GAME_RUNNING) {
            int result = bot_step_one(game, V);
            if (result == -1) {
                return -1;
            }
        }
    }
    return 0;
}



Hint giveHintPlus(const Game *game) {
    typedef struct {
        size_t cells[8];
        int count;
        int mines;
    } Constraint;
    static const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    Hint result = {NONE, -1, -1};
    if (game == NULL || game->field == NULL ||
        game->height <= 0 || game->width <= 0) {
        return result;
    }
    for (int x = 0; x < game->height; x++) {
        if (game->field[x] == NULL) {
            return result;
        }
    }
    const size_t max_size = (size_t)-1;
    if ((size_t)game->height > max_size / (size_t)game->width) {
        return result;
    }
    size_t capacity = (size_t)game->height * (size_t)game->width;
    if (capacity > max_size / sizeof(Constraint)) {
        return result;
    }
    Constraint *constraints = malloc(capacity * sizeof(*constraints));
    if (constraints == NULL) {
        return result;
    }
    size_t count = 0;
    Constraint solution = {{0}, 0, 0};

    /* Build constraints from visible clues, accounting for existing flags. */
    for (int x = 0; x < game->height; x++) {
        for (int y = 0; y < game->width; y++) {
            if (!game->field[x][y].isOpen) {
                continue;
            }
            Constraint clue = {{0}, 0, game->field[x][y].minesAround};
            if (clue.mines < 0 || clue.mines > 8 || game->field[x][y].isFlag) {
                goto cleanup;
            }
            for (int k = 0; k < 8; k++) {
                int nx = x + directions[k][0], ny = y + directions[k][1];
                if (nx < 0 || nx >= game->height || ny < 0 || ny >= game->width) {
                    continue;
                }
                if (game->field[nx][ny].isFlag) {
                    clue.mines--;
                } else if (!game->field[nx][ny].isOpen) {
                    /* Direction order keeps the cell indices sorted. */
                    clue.cells[clue.count++] = (size_t)nx * (size_t)game->width + (size_t)ny;
                }
            }
            if (clue.mines < 0 || clue.mines > clue.count) {
                goto cleanup;
            }
            if (clue.count > 0) {
                constraints[count++] = clue;
            }
        }
    }

    /* Process derived constraints too, until no new constraints remain. */
    for (size_t i = 0; i < count; i++) {
        Constraint current = constraints[i];
        if (current.mines == 0 || current.mines == current.count) {
            solution = current;
            goto cleanup;
        }
        for (size_t j = 0; j < i; j++) {
            /* Copies stay valid if realloc moves the array. */
            Constraint other = constraints[j];
            /* Split into intersection, current-only and other-only cells. */
            Constraint parts[3] = {0};
            int a = 0, b = 0;
            while (a < current.count || b < other.count) {
                if (a < current.count && b < other.count &&
                    current.cells[a] == other.cells[b]) {
                    parts[0].cells[parts[0].count++] = current.cells[a++];
                    b++;
                } else if (b == other.count ||
                           (a < current.count && current.cells[a] < other.cells[b])) {
                    parts[1].cells[parts[1].count++] = current.cells[a++];
                } else {
                    parts[2].cells[parts[2].count++] = other.cells[b++];
                }
            }
            if (parts[0].count == 0) {
                continue;
            }
            /* Bound the mine count in the intersection using both clues. */
            int lower = 0, upper = parts[0].count;
            if (current.mines - parts[1].count > lower) {
                lower = current.mines - parts[1].count;
            }
            if (other.mines - parts[2].count > lower) {
                lower = other.mines - parts[2].count;
            }
            if (current.mines < upper) upper = current.mines;
            if (other.mines < upper) upper = other.mines;
            if (lower > upper) {
                goto cleanup;
            }
            if (lower != upper) {
                continue;
            }
            /* Exact intersection counts also cover the original Subset Rule. */
            parts[0].mines = lower;
            parts[1].mines = current.mines - lower;
            parts[2].mines = other.mines - lower;
            for (int p = 0; p < 3; p++) {
                Constraint derived = parts[p];
                if (derived.count == 0) {
                    continue;
                }
                if (derived.mines == 0 || derived.mines == derived.count) {
                    solution = derived;
                    goto cleanup;
                }
                size_t k = 0;
                for (; k < count; k++) {
                    if (constraints[k].count == derived.count &&
                        memcmp(constraints[k].cells, derived.cells,
                               (size_t)derived.count * sizeof(derived.cells[0])) == 0) {
                        if (constraints[k].mines != derived.mines) {
                            goto cleanup;
                        }
                        break;
                    }
                }
                if (k != count) {
                    continue;
                }
                if (count == capacity) {
                    if (capacity > max_size / sizeof(*constraints) / 2) {
                        goto cleanup;
                    }
                    Constraint *grown = realloc(constraints, capacity * 2 * sizeof(*constraints));
                    if (grown == NULL) {
                        goto cleanup;
                    }
                    constraints = grown;
                    capacity *= 2;
                }
                constraints[count++] = derived;
            }
        }
    }

cleanup:
    if (solution.count > 0) {
        size_t cell = solution.cells[0];
        result = (Hint){solution.mines == 0 ? OPEN : FLAG,
                        (int)(cell / (size_t)game->width) + 1,
                        (int)(cell % (size_t)game->width) + 1};
    }
    free(constraints);
    return result;
}


// сделал первую версию, не умеет пока ничего кроме решения пошагово (может попасть в тупик не из-за рандома, нужен апгрейд giveHint)
// добавить мультизапуск и подсчет статистики.
int test(int W, int H, int D) {
    Game game = {0};
    char command[64];
    char *token = NULL;
    char *saveptr = NULL;
    if (gen(&game, H, W, D) != 0) {
        return -1;
    }
    while (open_cell(&game, 8, 8) == GAME_LOSE || count_opened_cells(&game) == 1) {
p1:
        free_field(&game);
        if (gen(&game, H, W, D) != 0) {
            return -1;
        }
    }
    while (1) {
        // print_field(&game, game.state == GAME_LOSE);
        if (game.state == GAME_WIN) {
            free_field(&game);
            return 0;
        }
        if (game.state == GAME_LOSE) {
            free_field(&game);
            return -2;
        }
        if (bot_steps(&game, 51, 0) == -1) {
            free_field(&game);
            return 1;
        }
    }
}

void print_stats(int W, int H, int D, int count, int wins) {
    printf("Для поля с шириной %d, с высотой %d, плотностью %d получили:\nшанс появления небезопасной ситуации = %.0f%% \
        (побед/из: %d/%d)\n", W, H, D, (int) 100 * ((double) (count-wins)/count), wins, count);
}

int main(void) {
    lcgs();
    char command[64];
    char *token = NULL;
    char *saveptr = NULL;
    int W = -1;
    int H = -1;
    int D = -1;
    int count = -1;
    while (1) {
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        saveptr = NULL;
        token = strtok_r(command, " \n", &saveptr);
        if (token == NULL) {
            continue;
        }
        W = -1;
        H = -1;
        D = -1;
        count = -1;
        if (!strcmp(token, "test")) {
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                W = atoi(token);
            }
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                H = atoi(token);
            }
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                D = atoi(token);
            }
            token = strtok_r(NULL, " \n", &saveptr);
            if (token != NULL) {
                count = atoi(token);
            }
            int winnable = 0;
            for (int i = 0; i < count; i++) {
                int result = test(W, H, D);
                if (result == -1) return -1;
                if (result == -2) {
                    printf("WTF\n"); 
                    return -1;
                }
                if (result == 0) winnable++;
            }
            print_stats(W, H, D, count, winnable);
        } else if (!strcmp(token, "exit")) {
            break;
        }
    }
    return 0;
}

// cc bot.c minesweeper.c lcg.c -o ../build/bot
// ../build/bot