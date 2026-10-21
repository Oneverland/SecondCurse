#include "minesweeper.h"
#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 900
#define MIN_BOARD_SIZE 5
#define MAX_BOARD_SIZE 30
#define MIN_DENSITY 3
#define MAX_DENSITY 12
#define HISTORY_LIMIT 5
#define HISTORY_FILE "/Users/oneverland/C_Projects/curse/src/game_history.txt"

typedef enum {
    SCREEN_MENU,
    SCREEN_SETTINGS,
    SCREEN_CREDITS,
    SCREEN_PLAYING
} Screen;

typedef struct {
    int width;
    int height;
    int density;
} GameSettings;

typedef struct {
    float cellSize;
    Rectangle boardRect;
} BoardLayout;

typedef struct {
    char date[16];
    char time[16];
    char result[32];
    int progress_percent;
    int width;
    int height;
} GameHistoryEntry;

typedef struct {
    GameHistoryEntry entries[HISTORY_LIMIT];
    int count;
} GameHistory;

static Font g_ui_font = {0};
static bool g_has_custom_font = false;

static int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static int min_int(int a, int b) {
    return (a < b) ? a : b;
}

static int build_font_codepoints(int *codepoints, int capacity) {
    int count = 0;
    for (int cp = 32; cp <= 126 && count < capacity; cp++) {
        codepoints[count++] = cp;
    }
    if (count < capacity) {codepoints[count++] = 0x0401;}
    for (int cp = 0x0410; cp <= 0x044F && count < capacity; cp++) {
        codepoints[count++] = cp;
    }
    if (count < capacity) {codepoints[count++] = 0x0451;}
    return count;
}

static Font load_ui_font(void) {
    static const char *candidates[] = {
        "assets/fonts/DejaVuSans.ttf",
        "../assets/fonts/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/System/Library/Fonts/Supplemental/Times New Roman.ttf"
    };
    int codepoints[160];
    int count = build_font_codepoints(codepoints, 160);
    int question_index;
    for (unsigned int i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        Font font = LoadFontEx(candidates[i], 48, codepoints, count);
        if (!IsFontValid(font)) {
            continue;
        }
        question_index = GetGlyphIndex(font, '?');
        if (GetGlyphIndex(font, 0x0410) != question_index) {
            g_has_custom_font = true;
            return font;
        }
        UnloadFont(font);
    }
    return GetFontDefault();
}

static float measure_text_width(const char *text, int font_size) {
    return MeasureTextEx(g_ui_font, text, (float)font_size, 1.0f).x;
}

static void draw_text_ui(const char *text, int x, int y, int font_size, Color color) {
    DrawTextEx(g_ui_font, text, (Vector2){(float)x, (float)y}, (float)font_size, 1.0f, color);
}

static bool ui_button(Rectangle rect, const char *label, Color base, Color hover, Color text_color) {
    Vector2 mouse = GetMousePosition();
    bool is_hovered = CheckCollisionPointRec(mouse, rect);
    Color fill = is_hovered ? hover : base;
    DrawRectangleRounded(rect, 0.25f, 10, fill);
    DrawRectangleRoundedLinesEx(rect, 0.25f, 10, 2.0f, Fade(BLACK, 0.25f));
    int font_size = 28;
    int text_width = (int)measure_text_width(label, font_size);
    draw_text_ui(label, (int)(rect.x + (rect.width - text_width) / 2.0f),
        (int)(rect.y + (rect.height - font_size) / 2.0f), font_size, text_color);
    return is_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void draw_centered_text(const char *text, int y, int font_size, Color color) {
    int text_width = (int)measure_text_width(text, font_size);
    draw_text_ui(text, (GetScreenWidth() - text_width) / 2, y, font_size, color);
}


static int calculate_progress_percent(const Game *game) {
    int total_cells;
    int opened_cells;
    if (game == NULL || game->field == NULL || game->height <= 0 || game->width <= 0) {
        return 0;
    }
    if (game->state == GAME_WIN) {
        return 100;
    }
    total_cells = game->height * game->width;
    opened_cells = count_opened_cells(game);
    return (opened_cells * 100 + total_cells / 2) / total_cells;
}

static void load_history(GameHistory *history) {
    FILE *file;
    char line[128];
    history->count = 0;
    file = fopen(HISTORY_FILE, "r");
    if (file == NULL) {
        return;
    }
    while (history->count < HISTORY_LIMIT && fgets(line, sizeof(line), file) != NULL) {
        GameHistoryEntry entry = {0};
        if (sscanf(line, "%15[^|]|%15[^|]|%31[^|]|%d|%d|%d",
                entry.date, entry.time, entry.result, &entry.progress_percent, &entry.width, &entry.height) == 6) {
            history->entries[history->count++] = entry;
        } else if (sscanf(line, "%15[^|]|%15[^|]|%31[^|]|%d",
                entry.date, entry.time, entry.result, &entry.progress_percent) == 4) {
            history->entries[history->count++] = entry;
        }
    }
    fclose(file);
}

static void save_history(const GameHistory *history) {
    FILE *file = fopen(HISTORY_FILE, "w");
    if (file == NULL) {
        return;
    }
    for (int i = 0; i < history->count; i++) {
        fprintf(file, "%s|%s|%s|%d|%d|%d\n",
            history->entries[i].date,
            history->entries[i].time,
            history->entries[i].result,
            history->entries[i].progress_percent,
            history->entries[i].width,
            history->entries[i].height);
    }
    fclose(file);
}

static void append_history(GameHistory *history, const Game *game) {
    GameHistoryEntry entry = {0};
    time_t now = time(NULL);
    struct tm *local_now = localtime(&now);
    int keep_count;
    if (history == NULL || game == NULL) {
        return;
    }
    if (local_now != NULL) {
        strftime(entry.date, sizeof(entry.date), "%d.%m.%Y", local_now);
        strftime(entry.time, sizeof(entry.time), "%H:%M:%S", local_now);
    } else {
        snprintf(entry.date, sizeof(entry.date), "--.--.----");
        snprintf(entry.time, sizeof(entry.time), "--:--:--");
    }
    snprintf(entry.result, sizeof(entry.result), "%s", game->state == GAME_WIN ? "ПОБЕДА" : "ПОРАЖЕНИЕ");
    entry.progress_percent = calculate_progress_percent(game);
    entry.width = game->width;
    entry.height = game->height;
    keep_count = min_int(history->count, HISTORY_LIMIT - 1);
    for (int i = keep_count; i > 0; i--) {
        history->entries[i] = history->entries[i - 1];
    }
    history->entries[0] = entry;
    history->count = min_int(history->count + 1, HISTORY_LIMIT);
    save_history(history);
}

static void draw_history_panel(const GameHistory *history, Color ink) {
    Rectangle panel = {(GetScreenWidth() - 760) / 2.0f, 700, 760, 170};
    Color line_color = Fade(ink, 0.16f);
    DrawRectangleRounded(panel, 0.05f, 12, Fade(RAYWHITE, 0.68f));
    DrawRectangleRoundedLinesEx(panel, 0.05f, 12, 2.0f, Fade(BLACK, 0.10f));
    draw_text_ui("Последние 5 игр", (int)panel.x + 24, (int)panel.y + 16, 26, ink);
    if (history->count == 0) {
        draw_text_ui("Пока нет завершённых игр", (int)panel.x + 24, (int)panel.y + 64, 22, Fade(ink, 0.72f));
        return;
    }
    for (int i = 0; i < history->count; i++) {
        char row[128];
        int row_y = (int)panel.y + 54 + i * 22;
        snprintf(row, sizeof(row), "%s, %s, %s %d%% %s",
            history->entries[i].date,
            history->entries[i].time,
            history->entries[i].result,
            history->entries[i].progress_percent,
            (history->entries[i].width > 0 && history->entries[i].height > 0) ? "" : "--x--");

        if (history->entries[i].width > 0 && history->entries[i].height > 0) {
            snprintf(row, sizeof(row), "%s, %s, %s %d%% %dx%d",
                history->entries[i].date,
                history->entries[i].time,
                history->entries[i].result,
                history->entries[i].progress_percent,
                history->entries[i].width,
                history->entries[i].height);
        }
        draw_text_ui(row, (int)panel.x + 24, row_y, 20, Fade(ink, 0.86f));
        if (i + 1 < history->count) {
            DrawLine((int)panel.x + 22, row_y + 24, (int)(panel.x + panel.width - 22), row_y + 24, line_color);
        }
    }
}

static Color number_color(int value) {
    switch (value) {
        case 1: return BLUE;
        case 2: return DARKGREEN;
        case 3: return RED;
        case 4: return DARKBLUE;
        case 5: return MAROON;
        case 6: return DARKPURPLE;
        case 7: return BROWN;
        case 8: return DARKGRAY;
        default: return BLACK;
    }
}

static BoardLayout get_board_layout(const Game *game) {
    const float margin = 40.0f;
    const float top_panel = 120.0f;
    float available_width = (float)GetScreenWidth() - margin * 2.0f;
    float available_height = (float)GetScreenHeight() - top_panel - margin;
    float cell_by_width = available_width / (float)game->width;
    float cell_by_height = available_height / (float)game->height;
    float cell_size = (cell_by_width < cell_by_height) ? cell_by_width : cell_by_height;
    if (cell_size > 42.0f) {
        cell_size = 42.0f;
    }
    if (cell_size < 18.0f) {
        cell_size = 18.0f;
    }
    float board_width = cell_size * game->width;
    float board_height = cell_size * game->height;
    float board_x = ((float)GetScreenWidth() - board_width) / 2.0f;
    float board_y = top_panel + (((float)GetScreenHeight() - top_panel) - board_height) / 2.0f;
    BoardLayout layout = {
        .cellSize = cell_size,
        .boardRect = (Rectangle){board_x, board_y, board_width, board_height}
    };
    return layout;
}

static void draw_background(void) {
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(),
        (Color){236, 224, 255, 255}, (Color){191, 171, 236, 255});
}

static void start_new_game(Game *game, const GameSettings *settings) {
    if (game->field != NULL) {
        free_field(game);
    }
    gen(game, settings->height, settings->width, settings->density);
}

static void draw_game_board(const Game *game, Hint hint, double hint_until) {
    BoardLayout layout = get_board_layout(game);
    bool show_hint = GetTime() < hint_until && hint.action != NONE;
    int hint_x = hint.x - 1;
    int hint_y = hint.y - 1;

    for (int row = 0; row < game->height; row++) {
        for (int col = 0; col < game->width; col++) {
            Rectangle cell = {
                layout.boardRect.x + col * layout.cellSize,
                layout.boardRect.y + row * layout.cellSize,
                layout.cellSize,
                layout.cellSize
            };
            bool is_hint_cell = show_hint && row == hint_x && col == hint_y;
            Color fill = (Color){140, 154, 175, 255};
            Color border = Fade(BLACK, 0.25f);
            if (game->field[row][col].isOpen == 1) {
                fill = (Color){238, 242, 247, 255};
            } else if (game->field[row][col].isFlag == 1) {
                fill = (Color){214, 93, 93, 255};
            } else if (game->state == GAME_LOSE && game->field[row][col].isMine == 1) {
                fill = (Color){56, 60, 72, 255};
            }
            if (is_hint_cell) {
                fill = (hint.action == FLAG) ? (Color){247, 190, 94, 255} : (Color){109, 208, 171, 255};
                border = Fade(BLACK, 0.45f);
            }
            DrawRectangleRec(cell, fill);
            DrawRectangleLinesEx(cell, 1.2f, border);
            if (game->field[row][col].isOpen == 1) {
                if (game->field[row][col].minesAround > 0) {
                    char text[2] = {(char)('0' + game->field[row][col].minesAround), '\0'};
                    int font_size = (int)(layout.cellSize * 0.55f);
                    int text_width = (int)measure_text_width(text, font_size);
                    draw_text_ui(text,
                        (int)(cell.x + (cell.width - text_width) / 2.0f),
                        (int)(cell.y + (cell.height - font_size) / 2.0f),
                        font_size,
                        number_color(game->field[row][col].minesAround));
                }
            } else if (game->field[row][col].isFlag == 1) {
                int font_size = (int)(layout.cellSize * 0.55f);
                int text_width = (int)measure_text_width("F", font_size);
                draw_text_ui("F", (int)(cell.x + (cell.width - text_width) / 2.0f),
                    (int)(cell.y + (cell.height - font_size) / 2.0f), font_size, RAYWHITE);
            } else if (game->state == GAME_LOSE && game->field[row][col].isMine == 1) {
                float radius = layout.cellSize * 0.22f;
                DrawCircleV((Vector2){cell.x + cell.width / 2.0f, cell.y + cell.height / 2.0f}, radius, RED);
            }
        }
    }
}

static void draw_settings_value_row(const char *label, int value, Rectangle minus_rect, Rectangle plus_rect, int y) {
    char buffer[64];
    draw_text_ui(label, 340, y, 30, (Color){42, 57, 79, 255});
    DrawRectangleRounded((Rectangle){520, (float)y - 8.0f, 160, 50}, 0.22f, 10, (Color){244, 247, 252, 255});
    DrawRectangleRoundedLinesEx((Rectangle){520, (float)y - 8.0f, 160, 50}, 0.22f, 10, 2.0f, Fade(BLACK, 0.18f));
    snprintf(buffer, sizeof(buffer), "%d", value);
    draw_text_ui(buffer, (int)(585 - measure_text_width(buffer, 28) / 2.0f), y + 4, 28, (Color){36, 48, 68, 255});
    ui_button(minus_rect, "-", (Color){218, 226, 238, 255}, (Color){200, 210, 224, 255}, BLACK);
    ui_button(plus_rect, "+", (Color){218, 226, 238, 255}, (Color){200, 210, 224, 255}, BLACK);
}

int main(void) {
    const Color ink = (Color){32, 44, 62, 255};
    const Color blue = (Color){88, 142, 214, 255};
    const Color blue_hover = (Color){69, 122, 196, 255};
    const Color sand = (Color){243, 198, 103, 255};
    const Color sand_hover = (Color){232, 182, 78, 255};
    const char *credits_lines[] = {
        "Cursed Miner",
        "СПБПУ Институт компьютерных наук и кибербезопасности",
        "Выполнено Соболевым Павлом Алексеевичем и", "Ивановым Александром Павловичем. Группа 5131001/50602",
        "Санкт-Петербург 2026"
    };
    Screen screen = SCREEN_MENU;
    Game game = {0};
    GameHistory history = {0};
    GameSettings settings = {16, 16, 7};
    Hint active_hint = {NONE, -1, -1};
    double hint_until = 0.0;
    double game_started_at = 0.0;
    bool game_result_saved = false;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Cursed Miner");
    SetTargetFPS(60);
    g_ui_font = load_ui_font();
    SetTextureFilter(g_ui_font.texture, TEXTURE_FILTER_BILINEAR);
    load_history(&history);

    while (!WindowShouldClose()) {
        if ((game.state == GAME_WIN || game.state == GAME_LOSE) && !game_result_saved) {
            append_history(&history, &game);
            game_result_saved = true;
        }

        BeginDrawing();
        ClearBackground(BLANK);
        draw_background();

        if (screen == SCREEN_MENU) {
            draw_centered_text("cursed miner", 100, 56, ink);
            draw_centered_text("a graphical minesweeper built on C", 168, 24, Fade(ink, 0.75f));
            Rectangle start_rect = {(GetScreenWidth() - 280) / 2.0f, 280, 280, 64};
            Rectangle settings_rect = {(GetScreenWidth() - 280) / 2.0f, 366, 280, 64};
            Rectangle credits_rect = {(GetScreenWidth() - 280) / 2.0f, 452, 280, 64};
            Rectangle exit_rect = {(GetScreenWidth() - 280) / 2.0f, 538, 280, 64};
            if (ui_button(start_rect, "Start Game", blue, blue_hover, RAYWHITE)) {
                start_new_game(&game, &settings);
                active_hint = (Hint){NONE, -1, -1};
                hint_until = 0.0;
                game_started_at = GetTime();
                game_result_saved = false;
                screen = (game.state == GAME_ERROR) ? SCREEN_MENU : SCREEN_PLAYING;
            }
            if (ui_button(settings_rect, "Settings", sand, sand_hover, ink)) {
                screen = SCREEN_SETTINGS;
            }
            if (ui_button(credits_rect, "Credits", (Color){174, 205, 187, 255}, (Color){153, 190, 168, 255}, ink)) {
                screen = SCREEN_CREDITS;
            }
            if (ui_button(exit_rect, "Exit", (Color){213, 118, 118, 255}, (Color){194, 97, 97, 255}, RAYWHITE)) {
                break;
            }
            char summary[128];
            snprintf(summary, sizeof(summary), "Current preset: %dx%d, mine divisor %d", settings.width, settings.height, settings.density);
            draw_centered_text(summary, 660, 24, Fade(ink, 0.75f));
            draw_history_panel(&history, ink);
        } else if (screen == SCREEN_SETTINGS) {
            draw_centered_text("Settings", 80, 52, ink);
            draw_centered_text("Mine divisor works like probability 1 / density. Lower means more mines.", 150, 24, Fade(ink, 0.75f));
            Rectangle card = {240, 210, GetScreenWidth() - 480.0f, 360};
            DrawRectangleRounded(card, 0.05f, 12, Fade(RAYWHITE, 0.65f));
            DrawRectangleRoundedLinesEx(card, 0.05f, 12, 2.0f, Fade(BLACK, 0.12f));
            Rectangle width_minus = {710, 252, 52, 42};
            Rectangle width_plus = {780, 252, 52, 42};
            Rectangle height_minus = {710, 332, 52, 42};
            Rectangle height_plus = {780, 332, 52, 42};
            Rectangle density_minus = {710, 412, 52, 42};
            Rectangle density_plus = {780, 412, 52, 42};
            draw_settings_value_row("Board width", settings.width, width_minus, width_plus, 260);
            draw_settings_value_row("Board height", settings.height, height_minus, height_plus, 340);
            draw_settings_value_row("Mine divisor", settings.density, density_minus, density_plus, 420);
            if (CheckCollisionPointRec(GetMousePosition(), width_minus) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                settings.width = clamp_int(settings.width - 1, MIN_BOARD_SIZE, MAX_BOARD_SIZE);
            }
            if (CheckCollisionPointRec(GetMousePosition(), width_plus) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                settings.width = clamp_int(settings.width + 1, MIN_BOARD_SIZE, MAX_BOARD_SIZE);
            }
            if (CheckCollisionPointRec(GetMousePosition(), height_minus) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                settings.height = clamp_int(settings.height - 1, MIN_BOARD_SIZE, MAX_BOARD_SIZE);
            }
            if (CheckCollisionPointRec(GetMousePosition(), height_plus) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                settings.height = clamp_int(settings.height + 1, MIN_BOARD_SIZE, MAX_BOARD_SIZE);
            }
            if (CheckCollisionPointRec(GetMousePosition(), density_minus) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                settings.density = clamp_int(settings.density - 1, MIN_DENSITY, MAX_DENSITY);
            }
            if (CheckCollisionPointRec(GetMousePosition(), density_plus) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                settings.density = clamp_int(settings.density + 1, MIN_DENSITY, MAX_DENSITY);
            }
            {
                int approx_mines = (settings.width * settings.height) / settings.density;
                char info[128];
                snprintf(info, sizeof(info), "Approximate mine count: about %d", approx_mines);
                draw_centered_text(info, 620, 24, Fade(ink, 0.75f));
            }
            if (ui_button((Rectangle){360, 700, 220, 58}, "Back", sand, sand_hover, ink)) {
                screen = SCREEN_MENU;
            }
            if (ui_button((Rectangle){700, 700, 220, 58}, "Start With These", blue, blue_hover, RAYWHITE)) {
                start_new_game(&game, &settings);
                active_hint = (Hint){NONE, -1, -1};
                hint_until = 0.0;
                game_started_at = GetTime();
                game_result_saved = false;
                screen = (game.state == GAME_ERROR) ? SCREEN_SETTINGS : SCREEN_PLAYING;
            }
        } else if (screen == SCREEN_CREDITS) {
            draw_centered_text("Credits", 100, 54, ink);
            DrawRectangleRounded((Rectangle){210, 210, GetScreenWidth() - 420.0f, 330}, 0.05f, 12, Fade(RAYWHITE, 0.7f));
            DrawRectangleRoundedLinesEx((Rectangle){210, 210, GetScreenWidth() - 420.0f, 330}, 0.05f, 12, 2.0f, Fade(BLACK, 0.14f));
            for (int i = 0; i < 5; i++) {
                draw_centered_text(credits_lines[i], 260 + i * 56, i == 0 ? 34 : 24, i == 0 ? ink : Fade(ink, 0.82f));
            }
            if (ui_button((Rectangle){(GetScreenWidth() - 220) / 2.0f, 620, 220, 58}, "Back", sand, sand_hover, ink)) {
                screen = SCREEN_MENU;
            }
        } else if (screen == SCREEN_PLAYING) {
            Rectangle menu_rect = {32, 28, 150, 48};
            Rectangle restart_rect = {202, 28, 150, 48};
            Rectangle hint_rect = {372, 28, 150, 48};
            BoardLayout layout = get_board_layout(&game);
            Vector2 mouse = GetMousePosition();
            DrawRectangleRounded((Rectangle){20, 18, GetScreenWidth() - 40.0f, 70}, 0.18f, 12, Fade(RAYWHITE, 0.68f));
            DrawRectangleRoundedLinesEx((Rectangle){20, 18, GetScreenWidth() - 40.0f, 70}, 0.18f, 12, 2.0f, Fade(BLACK, 0.12f));
            if (ui_button(menu_rect, "Menu", sand, sand_hover, ink)) {
                active_hint = (Hint){NONE, -1, -1};
                hint_until = 0.0;
                screen = SCREEN_MENU;
            }
            if (ui_button(restart_rect, "Restart", blue, blue_hover, RAYWHITE)) {
                start_new_game(&game, &settings);
                active_hint = (Hint){NONE, -1, -1};
                hint_until = 0.0;
                game_started_at = GetTime();
                game_result_saved = false;
            }
            if (ui_button(hint_rect, "Hint", (Color){174, 205, 187, 255}, (Color){153, 190, 168, 255}, ink)) {
                active_hint = giveHint(&game);
                hint_until = GetTime() + 2.5;
            }
            {
                char status_text[128];
                const char *state_label = "Running";
                if (game.state == GAME_WIN) {
                    state_label = "Victory";
                } else if (game.state == GAME_LOSE) {
                    state_label = "Defeat";
                } else if (game.state == GAME_ERROR) {
                    state_label = "Error";
                }
                snprintf(status_text, sizeof(status_text), "%dx%d  |  mine divisor %d  |  mines %d  |  %s",
                    game.width, game.height, game.density, game.mine_count, state_label);
                draw_text_ui(status_text, 550, 28, 24, ink);
                snprintf(status_text, sizeof(status_text), "Time %.0fs", GetTime() - game_started_at);
                draw_text_ui(status_text, 550, 54, 22, Fade(ink, 0.8f));
            }
            if (game.state == GAME_RUNNING && CheckCollisionPointRec(mouse, layout.boardRect)) {
                int col = (int)((mouse.x - layout.boardRect.x) / layout.cellSize);
                int row = (int)((mouse.y - layout.boardRect.y) / layout.cellSize);
                if (row >= 0 && row < game.height && col >= 0 && col < game.width) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        open_cell(&game, row, col);
                        active_hint = (Hint){NONE, -1, -1};
                        hint_until = 0.0;
                    } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                        set_flag(&game, row, col);
                    }
                }
            }
            draw_game_board(&game, active_hint, hint_until);
            if (game.state == GAME_WIN || game.state == GAME_LOSE) {
                Rectangle panel = {(GetScreenWidth() - 420) / 2.0f, (GetScreenHeight() - 190) / 2.0f, 420, 190};
                DrawRectangleRounded(panel, 0.08f, 12, Fade(RAYWHITE, 0.92f));
                DrawRectangleRoundedLinesEx(panel, 0.08f, 12, 2.0f, Fade(BLACK, 0.18f));
                draw_centered_text(game.state == GAME_WIN ? "You cleared the field" : "Boom. You hit a mine", (int)panel.y + 32, 34, ink);
                draw_centered_text(game.state == GAME_WIN ? "Nice work." : "Try another run.", (int)panel.y + 78, 24, Fade(ink, 0.8f));
                if (ui_button((Rectangle){panel.x + 36, panel.y + 122, 150, 46}, "Menu", sand, sand_hover, ink)) {
                    screen = SCREEN_MENU;
                }
                if (ui_button((Rectangle){panel.x + 234, panel.y + 122, 150, 46}, "Restart", blue, blue_hover, RAYWHITE)) {
                    start_new_game(&game, &settings);
                    active_hint = (Hint){NONE, -1, -1};
                    hint_until = 0.0;
                    game_started_at = GetTime();
                    game_result_saved = false;
                }
            }
        }
        EndDrawing();
    }
    free_field(&game);
    if (g_has_custom_font) {
        UnloadFont(g_ui_font);
    }
    CloseWindow();
    return 0;
}
// cc main.c minesweeper.c lcg.c -I/opt/homebrew/include -L/opt/homebrew/lib -lraylib -framework IOKit -framework Cocoa -framework OpenGL -o ../build/cursedminer_game