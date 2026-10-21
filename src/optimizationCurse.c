#include "minesweeper.h"
#include <stdio.h>
#include <stdlib.h>
#include "lcg.h"

int count_opened_cells(const Game *game) {
    int opened_cells = 0;
    for (int i = 0; i < game->height; i++) {
        for (int k = 0; k < game->width; k++) {
            opened_cells += game->field[i][k].isOpen;
        }
    }
    return opened_cells;
}
// можно в поле game хранить количество открытых безопасных клеточек и инкрементировать в open
static void update_win_state(Game *game) {
    int safe_cells = game->height * game->width - game->mine_count;
    if (count_opened_cells(game) == safe_cells) {
        game->state = GAME_WIN;
    }
}

static void open_zero_area(Game *game, int x, int y) {
    if (x < 0 || x >= game->height || y < 0 || y >= game->width) {
        return;
    }
    if (game->field[x][y].isOpen == 1 || game->field[x][y].isFlag == 1) {
        return;
    }
    game->field[x][y].isOpen = 1;
    if (game->field[x][y].isMine == 1) {
        game->state = GAME_LOSE;
        return;
    }
    if (game->field[x][y].minesAround != 0) {
        return;
    }
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            open_zero_area(game, x + dx, y + dy);
        }
    }
}

int gen(Game *game, int height, int width, int density) {
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

void free_field(Game *game) {
    if (game == NULL || game->field == NULL) {
        return;
    }
    free(game->field[0]);
    free(game->field);
    game->field = NULL;
}

GameState set_flag(Game *game, int x, int y) {
    if (game == NULL || game->field == NULL) {
        return GAME_ERROR;
    }
    if (game->state != GAME_RUNNING) {
        return game->state;
    }
    if (x >= 0 && x < game->height && y >= 0 && y < game->width) {
        if (game->field[x][y].isOpen == 0) {
            game->field[x][y].isFlag = !game->field[x][y].isFlag;
        }
    }
    update_win_state(game);
    return game->state;
} // он тут не нужен, тк нельзя выиграть за счёт поставленных флагов. либо менять выигрышную ситуацию

GameState open_cell(Game *game, int x, int y) {
    if (game == NULL || game->field == NULL) {
        return GAME_ERROR;
    }
    if (game->state != GAME_RUNNING) {
        return game->state;
    }
    if (x < 0 || x >= game->height || y < 0 || y >= game->width) {
        return game->state;
    }
    if (game->field[x][y].isOpen == 1 || game->field[x][y].isFlag == 1) {
        return game->state;
    }
    if (game->field[x][y].isMine == 1) {
        game->field[x][y].isOpen = 1;
        game->state = GAME_LOSE;
        return game->state;
    }
    open_zero_area(game, x, y);
    if (game->state == GAME_RUNNING) {
        update_win_state(game); // инкрементирование открытых безопасных клеточек
    }
    return game->state;
}

Hint giveHint(const Game *game) {
    static const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    if (game == NULL || game->field == NULL) {
        return (Hint){NONE, -1, -1};
    }
    for (int x = 0; x < game->height; x++) {
        for (int y = 0; y < game->width; y++) {
            if (game->field[x][y].isOpen == 0 || game->field[x][y].minesAround == 0) {
                continue;
            }
            int flags = 0;
            int hidden = 0;
            int hidden_x[8];
            int hidden_y[8];
            for (int i = 0; i < 8; i++) {
                int nx = x + directions[i][0];
                int ny = y + directions[i][1];
                if (nx < 0 || nx >= game->height || ny < 0 || ny >= game->width) {
                    continue;
                }
                if (game->field[nx][ny].isFlag == 1) {
                    flags++;
                } else if (game->field[nx][ny].isOpen == 0) {
                    hidden_x[hidden] = nx;
                    hidden_y[hidden] = ny;
                    hidden++;
                }
            }
            if (hidden == 0) {
                continue;
            }
            if (flags == game->field[x][y].minesAround) {
                return (Hint){OPEN, hidden_x[0] + 1, hidden_y[0] + 1};
            }
            if (flags + hidden == game->field[x][y].minesAround) {
                return (Hint){FLAG, hidden_x[0] + 1, hidden_y[0] + 1};
            }
        }
    }
    return (Hint){NONE, -1, -1};
} // усовершенствовать/оптимизировать


// надо анализировать каждую функцию, какая из них дольше всех работает,
// чтобы начать с неё оптимизацию

int main () {
    printf("d");
    return 0;
}





// cc optimizationCurse.c ../readyToUse/lcg.c -o optimizationCurse && ./optimizationCurse