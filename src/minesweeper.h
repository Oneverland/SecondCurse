#ifndef MINESWEEPER_H
#define MINESWEEPER_H

typedef struct cursedminer {
    int isOpen;
    int isMine;
    int isFlag;
    int minesAround;
} Cell; // упаковка структуры в один байт, для первых трех полей достаточно 3 бита, для последнего 4 (диапазон 0-8), 
        // для этого нужно будет переписать почти всё, тк придется работать побитово, однако экономия памяти в 16 раз
        // ЛИБО легче просто uint8_t использовать для каждой клеточки, будет экономия в 4 раза, а читаемость кода останется легкой
        
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

typedef enum {
    GAME_RUNNING,
    GAME_WIN,
    GAME_LOSE,
    GAME_ERROR
} GameState;

typedef struct {
    Cell **field;
    int height;
    int width;
    int density;
    int mine_count;
    GameState state;
} Game;

int count_opened_cells(const Game *game);
int gen(Game *game, int height, int width, int density);
void free_field(Game *game);
GameState set_flag(Game *game, int x, int y);
GameState open_cell(Game *game, int x, int y);
Hint giveHint(const Game *game);

#endif
