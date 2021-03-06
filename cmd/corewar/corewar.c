#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "colors.h"
#include "cpu.h"
#include "libasm.h"
#include "op.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#define RECT_BUFFER 10

#define GRAPH_RECT_X (0 + RECT_BUFFER)
#define GRAPH_RECT_Y (DEBUG_RECT_HEIGHT + DEBUG_RECT_X + RECT_BUFFER)
#define GRAPH_RECT_WIDTH DEBUG_RECT_WIDTH
#define GRAPH_RECT_HEIGHT 250

#define DEBUG_RECT_X (0 + RECT_BUFFER)
#define DEBUG_RECT_Y (0 + RECT_BUFFER)
#define DEBUG_RECT_WIDTH 1111
#define DEBUG_RECT_HEIGHT 825

#define OPEN_RECT_X (DEBUG_RECT_WIDTH + DEBUG_RECT_X + RECT_BUFFER)
#define OPEN_RECT_Y (EDIT_RECT_HEIGHT + EDIT_RECT_Y + RECT_BUFFER)
#define OPEN_RECT_WIDTH 300
#define OPEN_RECT_HEIGHT GRAPH_RECT_HEIGHT

#define EDIT_RECT_X OPEN_RECT_X
#define EDIT_RECT_Y (0 + RECT_BUFFER)
#define EDIT_RECT_WIDTH OPEN_RECT_WIDTH
#define EDIT_RECT_HEIGHT DEBUG_RECT_HEIGHT

#define CHAR_WIDTH 13

#define WINDOW_WIDTH (EDIT_RECT_WIDTH + EDIT_RECT_X + RECT_BUFFER)
#define WINDOW_HEIGHT (GRAPH_RECT_HEIGHT + GRAPH_RECT_Y + RECT_BUFFER)

#define MAX_VERTEX_BUFFER 512 * 2048  // 512 * 1024 increase to fix buggy ui
#define MAX_ELEMENT_BUFFER 128 * 2048 // 128 * 1024

// open_buf is the buffer for the pathname inside the open window.
char open_buf[PATH_MAX] = {"./tests/aff1.s"};

// edit_buf is the buffer in which the editor holds it's text
char edit_buf[4096] = {"beans haha lol."};

// the following macros are the bit masks for determining which windows to
// display.

// g_instruction_calls is the number of instruction calls in the current program.
int g_instruction_calls[NUM_OPS + 1] = {0};

// running keeps track of if the program should be stepped through
// automatically.
bool running = 0;

char plyrbuf[NAME_MAX + 4] = "none";

#define C_BLACK nk_rgba(0, 0, 0, 255)
#define C_WHITE nk_rgba(255, 255, 255, 255)
#define C_GREY nk_rgba(238, 238, 238, 255)
#define C_LIGHTGREY nk_rgba(245, 245, 245, 255)
#define C_YELLOW nk_rgba(255, 255, 234, 255)
#define C_DARKYELLOW nk_rgba(153, 153, 76, 255)
#define C_LIGHTBLUE nk_rgba(224, 235, 245, 255)
#define C_BLUE nk_rgba(85, 170, 170, 255)
#define C_GREEN nk_rgba(68, 136, 68, 255)
#define C_LIGHTGREEN nk_rgba(234, 255, 234, 255)
#define C_CLEAR nk_rgba(0, 0, 0, 0)

// set_color sets the colorscheme. it should only be called once.
static void set_color(struct nk_context *ctx) {
  struct nk_color table[NK_COLOR_COUNT];
  table[NK_COLOR_TEXT] = C_BLACK;
  table[NK_COLOR_WINDOW] = C_WHITE;
  table[NK_COLOR_HEADER] = C_LIGHTBLUE;
  table[NK_COLOR_BORDER] = C_GREY;
  table[NK_COLOR_BUTTON] = C_LIGHTGREY;
  table[NK_COLOR_BUTTON_HOVER] = C_GREY;
  table[NK_COLOR_BUTTON_ACTIVE] = C_GREY;
  table[NK_COLOR_TOGGLE] = C_GREY;
  table[NK_COLOR_TOGGLE_HOVER] = C_GREY;
  table[NK_COLOR_TOGGLE_CURSOR] = C_GREY;
  table[NK_COLOR_SELECT] = C_GREY;
  table[NK_COLOR_SELECT_ACTIVE] = C_GREY;
  table[NK_COLOR_SLIDER] = C_GREY;
  table[NK_COLOR_SLIDER_CURSOR] = C_GREY;
  table[NK_COLOR_SLIDER_CURSOR_HOVER] = C_GREY;
  table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = C_GREY;
  table[NK_COLOR_PROPERTY] = C_GREY;
  table[NK_COLOR_EDIT] = C_GREY;
  table[NK_COLOR_EDIT_CURSOR] = C_BLUE;
  table[NK_COLOR_COMBO] = C_GREY;
  table[NK_COLOR_CHART] = C_GREY;
  table[NK_COLOR_CHART_COLOR] = C_GREEN;
  table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = C_GREEN;
  table[NK_COLOR_SCROLLBAR] = C_GREY;
  table[NK_COLOR_SCROLLBAR_CURSOR] = C_BLUE;
  table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = C_BLUE;
  table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = C_BLUE;
  table[NK_COLOR_TAB_HEADER] = C_GREEN;
  nk_style_from_table(ctx, table);
}

// win_graph displays a graph of the frequency of each instruction call.
static void win_graph(struct nk_context *ctx, struct s_cpu *cpu) {
  (void)cpu;
  if (nk_begin(ctx, "Graph",
               nk_rect(GRAPH_RECT_X, GRAPH_RECT_Y, GRAPH_RECT_WIDTH,
                       GRAPH_RECT_HEIGHT),
               NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE |
                   NK_WINDOW_TITLE)) {
    nk_layout_row_dynamic(ctx, 150, 1);
    nk_chart_begin(ctx, NK_CHART_COLUMN, NUM_OPS, 0, 128);
    for (unsigned long i = 1; i <= NUM_OPS; i++) {
      nk_chart_push(ctx, g_instruction_calls[i] % 128);
    }
    nk_chart_end(ctx);
    nk_layout_row_dynamic(ctx, 20, NUM_OPS);
    for (unsigned long i = 1; i <= NUM_OPS; i++) {
      nk_label(ctx, g_op_tab[i].name, NK_TEXT_CENTERED);
    }
  }
  nk_end(ctx);
}

static const char *g_bytes_upper[256] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B",
    "0C", "0D", "0E", "0F", "10", "11", "12", "13", "14", "15", "16", "17",
    "18", "19", "1A", "1B", "1C", "1D", "1E", "1F", "20", "21", "22", "23",
    "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B",
    "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", "46", "47",
    "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", "50", "51", "52", "53",
    "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B",
    "6C", "6D", "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77",
    "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", "82", "83",
    "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B",
    "9C", "9D", "9E", "9F", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7",
    "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3",
    "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
    "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB",
    "CC", "CD", "CE", "CF", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
    "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3",
    "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
    "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB",
    "FC", "FD", "FE", "FF",
};

char *g_bytes_lower[256] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b",
    "0c", "0d", "0e", "0f", "10", "11", "12", "13", "14", "15", "16", "17",
    "18", "19", "1a", "1b", "1c", "1d", "1e", "1f", "20", "21", "22", "23",
    "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b",
    "3c", "3d", "3e", "3f", "40", "41", "42", "43", "44", "45", "46", "47",
    "48", "49", "4a", "4b", "4c", "4d", "4e", "4f", "50", "51", "52", "53",
    "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b",
    "6c", "6d", "6e", "6f", "70", "71", "72", "73", "74", "75", "76", "77",
    "78", "79", "7a", "7b", "7c", "7d", "7e", "7f", "80", "81", "82", "83",
    "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b",
    "9c", "9d", "9e", "9f", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af", "b0", "b1", "b2", "b3",
    "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
    "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb",
    "cc", "cd", "ce", "cf", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "d8", "d9", "da", "db", "dc", "dd", "de", "df", "e0", "e1", "e2", "e3",
    "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
    "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb",
    "fc", "fd", "fe", "ff",
};

// this makes it easier to change values and stuff
#define MAKE_PLAYER_COLOR(n, suffix, r_, g_, b_, a_)                           \
  static struct nk_color p##n##suffix = {                                      \
      .r = MIN(r_, 255), .g = MIN(g_, 255), .b = MIN(b_, 255), .a = a_};

#define MAKE_PROFILE(n, r_, g_, b_)                                            \
  MAKE_PLAYER_COLOR(n, c, r_, g_, b_, 255);                                    \
  MAKE_PLAYER_COLOR(n, m, r_, g_, b_, 64);                                     \
  static struct nk_color p##n##w = {.r = 0, .g = 0, .b = 0, .a = 255};         \
  if (p##n##w.r == 0 && p##n##w.g == 0 && p##n##w.b == 0) {                    \
    struct s_hsv hsv##n = to_hsv(r_, g_, b_);                                  \
    struct nk_color tmp_p##n##w = to_rgb(hsv##n.h - 30, hsv##n.s, hsv##n.v);   \
    p##n##w = tmp_p##n##w;                                                     \
  }

// teardown of allocated memory frees everything allocated in the cpu
void cpu_cleanup(struct s_cpu *cpu) {
  int ii;
  struct s_process *lst, *tofree;

  for (ii = 0; ii < MAX_PLAYERS; ++ii) {
    free(cpu->players[ii].name);
    free(cpu->players[ii].comment);
  }
  lst = cpu->processes;
  while (lst != 0) {
    tofree = lst;
    lst = lst->next;
    free(tofree);
  }
}

// win_debug displays the program and buttons to step through.
static void win_debug(struct nk_context *ctx, struct s_cpu *cpu) {
  if (nk_begin(ctx, "Debug",
               nk_rect(DEBUG_RECT_X, DEBUG_RECT_Y, DEBUG_RECT_WIDTH,
                       DEBUG_RECT_HEIGHT),
               NK_WINDOW_BORDER | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE |
                   NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {
    char buf[44];

    MAKE_PROFILE(1, 255, 50, 50);
    MAKE_PROFILE(2, 50, 129, 50);
    MAKE_PROFILE(3, 50, 129, 255);
    MAKE_PROFILE(4, 129, 50, 129);

    // default color for unknown player
    static struct nk_color defaultc = {.r = 213, .g = 198, .b = 182, .a = 255};

    // top buttons
    nk_layout_row_static(ctx, 30, 80, 10);
    if (cpu->processes == 0 && cpu->winner != -1 && cpu->clock != 0) { // &&
      // (cpu->num_checks != 0 || cpu->nbr_lives != 0)) {
      printf("Contestant %d, \"%s\", has won !\n", cpu->winner + 1,
             cpu->players[cpu->winner].name);
    }
    if (nk_button_label(ctx, "step")) {
      if (cpu->processes != 0)
        cpu->step(cpu);
    }
    if (nk_button_label(ctx, "walk")) {
      if (cpu->processes != 0) {
        int cont = 0;
        while (cont == 0 && cpu->processes) {
          cont = cpu->step(cpu);
        }
      }
    }
    // run button
    char *text;
    if (running)
      text = "stop";
    else
      text = "run";
    if (nk_button_label(ctx, text)) {
      running = !running;
      glfwSetTime(1);
    }
    static int slider = 1;
    nk_slider_int(ctx, 1, &slider, 20, 1);

    // reset button
    if (nk_button_label(ctx, "reset")) {
      cpu_cleanup(cpu);
      *cpu = new_cpu();
      ft_bzero(g_mem_colors, MEM_SIZE * sizeof(*g_mem_colors));
      glfwSetTime(1);
      running = 0;
      bzero(g_instruction_calls, sizeof(g_instruction_calls));
    }

    double trash;
    double time = modf(glfwGetTime(), &trash);
    time *= 100;
    if (cpu->processes != 0 && running) {
      for (int ii = slider; ii; ii--) {
        cpu->step(cpu);
      }
    }

    // top stats
    // snprintf(winbuf, sizeof(winbuf), "Winner: %d", cpu->winner);
    // nk_label(ctx, winbuf, NK_TEXT_CENTERED);
    snprintf(buf, sizeof(buf), "Active: %d", cpu->active);
    nk_label(ctx, buf, NK_TEXT_CENTERED);
    snprintf(buf, sizeof(buf), "CTD: %d", cpu->cycle_to_die);
    nk_label(ctx, buf, NK_TEXT_CENTERED);
    snprintf(buf, sizeof(buf), "Cycle: %d", cpu->clock);
    nk_label(ctx, buf, NK_TEXT_CENTERED);

    // if someone has won display winner
    static struct nk_rect win_rect = {100, 100, 500, 237};
    static char *win_str = NULL;
    static char *comment_str = NULL;
    // static char winner_buf;
    if (win_str || (cpu->winner != -1 && cpu->processes == NULL)) {
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Winner",
                         NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE, win_rect)) {
        if (!win_str) {
          char *s = ft_itoa(cpu->winner + 1);
          char *s2 = ft_strjoin(" ", cpu->players[cpu->winner].name);
          win_str = ft_strjoin(s, s2);
          free(s);
          free(s2);
          s = ft_strjoin("WINNER: P", win_str);
          free(win_str);
          win_str = s;
          comment_str = cpu->players[cpu->winner].comment;
          win_rect.w = 40 + MAX__(ft_strlen(cpu->players[cpu->winner].comment) *
                                      CHAR_WIDTH,
                                  ft_strlen(win_str) * CHAR_WIDTH);
        }
        nk_layout_row_dynamic(ctx, 75, 1);
        nk_label(ctx, win_str, NK_TEXT_CENTERED);
        nk_label(ctx, comment_str, NK_TEXT_CENTERED);
        nk_layout_row_static(ctx, 30, 30, 1);
        cpu->winner = -1;
        running = 0;
        if (nk_button_label(ctx, "ok")) {
          free(win_str);
          win_str = NULL;
          comment_str = NULL;
        }
        nk_popup_end(ctx);
      } else {
        free(win_str);
        win_str = NULL;
        comment_str = NULL;
      }
    }

    // display active players
    // nk_layout_row_static(ctx, 30, DEBUG_RECT_WIDTH/MAX_PLAYERS, MAX_PLAYERS);
    nk_layout_row_dynamic(ctx, 30, MAX_PLAYERS);
    for (int ii = 0; ii < MAX_PLAYERS; ii++) {
      if (cpu->players[ii].name)
        snprintf(plyrbuf, sizeof(plyrbuf), "P%d: \'%s\'", ii + 1,
                 cpu->players[ii].name);
      else
        snprintf(plyrbuf, sizeof(plyrbuf), "P0: NONE");
      nk_label(ctx, plyrbuf, NK_TEXT_LEFT);
    }

    // print the program
    for (int ii = 0; ii < MEM_SIZE; ii++) {
      if (ii % 64 == 0)
        nk_layout_row_static(ctx, 7, 13, 64);

      buf[0] = g_bytes_upper[cpu->program[ii]][0];
      buf[1] = g_bytes_upper[cpu->program[ii]][1];
      buf[2] = 0;
      struct s_process *hd = cpu->processes;
      while (hd != NULL) {
        if (hd->pc == ii) {
          switch (hd->player) {
          case -1:
            nk_label_colored(ctx, buf, NK_TEXT_LEFT, p1c);
            break;
          case -2:
            nk_label_colored(ctx, buf, NK_TEXT_LEFT, p2c);
            break;
          case -3:
            nk_label_colored(ctx, buf, NK_TEXT_LEFT, p3c);
            break;
          case -4:
            nk_label_colored(ctx, buf, NK_TEXT_LEFT, p4c);
            break;
          }
          break;
        }
        hd = hd->next;
      }
      if (hd == 0) {
        if (g_mem_colors[ii].player) {
          switch (g_mem_colors[ii].player) {
          case -1:
            if (g_mem_colors[ii].writes)
              nk_label_colored(ctx, buf, NK_TEXT_LEFT, p1w);
            else
              nk_label_colored(ctx, buf, NK_TEXT_LEFT, p1m);
            break;
          case -2:
            if (g_mem_colors[ii].writes)
              nk_label_colored(ctx, buf, NK_TEXT_LEFT, p2w);
            else
              nk_label_colored(ctx, buf, NK_TEXT_LEFT, p2m);
            break;
          case -3:
            if (g_mem_colors[ii].writes)
              nk_label_colored(ctx, buf, NK_TEXT_LEFT, p3w);
            else
              nk_label_colored(ctx, buf, NK_TEXT_LEFT, p3m);
            break;
          case -4:
            if (g_mem_colors[ii].writes)
              nk_label_colored(ctx, buf, NK_TEXT_LEFT, p4w);
            else
              nk_label_colored(ctx, buf, NK_TEXT_LEFT, p4m);
            break;
          }
        } else {
          nk_label_colored(ctx, buf, NK_TEXT_LEFT, defaultc);
        }
      }
    }
  }
  nk_end(ctx);
}

void new_player(struct s_cpu *cpu, header_t *h, int num) {
  cpu->players[num - 1].player_number = num;
  cpu->players[num - 1].active_processes = 1;
  cpu->players[num - 1].last_live = 0;
  cpu->players[num - 1].prog_size = h->prog_size;
  if (cpu->players[num - 1].name)
    free(cpu->players[num - 1].name);
  cpu->players[num - 1].name = strndup(h->prog_name, NAME_MAX);
  if (cpu->players[num - 1].comment)
    free(cpu->players[num - 1].comment);
  cpu->players[num - 1].comment = strndup(h->comment, COMMENT_LENGTH);
}

#define OUT(...) printf(__VA_ARGS__)

#define ROW_HEIGHT 15
#define BUTTON_HEIGHT 30
#define ROW_BUF 30
#define WIDTH_BUF 20
#define HEIGHT_BUF 30
#define OFF_Y 50
#define OFF_X 50

#define COR_FILENAME "/tmp/corewar.cor"
#define ASM_FILENAME "/tmp/corewar.s"

// load_file reads a file into readbuf. if the file starts with
// COREWAR_EXEC_MAGIC it is loaded via `cpu->load()' otherwise load_file reads
// the file into `edit_buf'. returns number of bytes read.
static int load_file(struct s_cpu *cpu, FILE *f, int location, int player) {
  header_t h;
  const size_t maxsize = CHAMP_MAX_SIZE + sizeof(header_t);
  char readbuf[maxsize + 1];
  long len = fread(readbuf, 1, maxsize, f);
  readbuf[maxsize] = 0;
  h = *(header_t *)readbuf;
  h.magic = ntohl(h.magic);
  h.prog_size = ntohl(h.prog_size);
  if (len > 0) {
    if (valid_header_p(h)) {
      cpu->load(cpu, readbuf + sizeof(header_t), len - sizeof(header_t),
                location);
      cpu->spawn_process(cpu, location, -player);
      new_player(cpu, &h, player);
      OUT("* Player %d, weighing %d bytes, \"%s\" (\"%s\") !\n", player,
             h.prog_size, h.prog_name, h.comment);
      if (g_color || g_gui) {
        for (int jj = location, ll = (len - sizeof(header_t));
             jj < ll + location; ++jj) {
          g_mem_colors[jj].player = -player;
        }
      }
    } else {
      rewind(f);
      bzero(edit_buf, sizeof(edit_buf));
      fread(edit_buf, 1, sizeof(edit_buf), f);
      for (size_t i = 0; i < sizeof(edit_buf); i++) {
        if (edit_buf[i] == '\t') {
          edit_buf[i] = ' ';
        }
      }
    }
  }
  return len;
}

// win_open is the window used for opening .cor and .s files.
static void win_open(struct nk_context *ctx, struct s_cpu *cpu) {
  static char *cantopen = NULL;

  if (nk_begin(
          ctx, "Open",
          nk_rect(OPEN_RECT_X, OPEN_RECT_Y, OPEN_RECT_WIDTH, OPEN_RECT_HEIGHT),
          NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE |
              NK_WINDOW_TITLE)) {
    static uint32_t offset = 0;
    static int select = 0;

    nk_layout_row_dynamic(ctx, 30, 1);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, open_buf,
                                   sizeof(open_buf) - 1, nk_filter_ascii);

    // set custom offset for opening champion
    nk_layout_row_dynamic(ctx, 30, 1);
    static char offset_buf[PATH_MAX] = {'0', '\0'};
    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, offset_buf,
                                   sizeof(offset_buf) - 1, nk_filter_ascii);

    // this box lets you select the region of memory you want to load the
    // program into.
    nk_layout_row_static(ctx, 25, 200, 1);
    static const char *players[] = {"One", "Two", "Three", "Four"};
    select = nk_combo(ctx, players, 4, select, 25, nk_vec2(200, 200));
    offset = (unsigned)((1024 * select) + ft_atoi(offset_buf)) % MEM_SIZE;

    nk_layout_row_dynamic(ctx, 30, 2);
    // this button opens a file, reads it into a buffer and copies it to the
    // edit buffer or the debug window depending if it's a compiled corewar bin.
    if (nk_button_label(ctx, "Open file") && open_buf[0]) {
      FILE *f = fopen(open_buf, "r");
      if (f == NULL) {
        puts("Can't open");
        cantopen = open_buf;
      } else {
        // Was 'select' instead of 'select + 1' changed because it would not
        // load players properly
        if (!load_file(cpu, f, offset, select + 1))
          cantopen = open_buf;
        // cantopen = !load_file(cpu, f, offset, select + 1);
        fclose(f);
      }
    }

    // error popup
    if (cantopen != NULL) {
      static struct nk_rect s = {-75, -75, 200, 150};
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "ERROR",
                         NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE, s)) {
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "can't open file:", NK_TEXT_LEFT);
        nk_label(ctx, cantopen, NK_TEXT_LEFT);
        nk_layout_row_static(ctx, 30, 30, 1);
        if (nk_button_label(ctx, "ok")) {
          cantopen = NULL;
        }
        nk_popup_end(ctx);
      } else {
        cantopen = NULL;
      }
    }

    // save button
    if (nk_button_label(ctx, "Save")) {
      int file = open(open_buf, O_RDWR | O_CREAT | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (file < 0) {
        printf("Error opening %s\n", open_buf);
        cantopen = open_buf;
      } else {
        size_t buflen = ft_strlen(edit_buf);
        size_t size = write(file, edit_buf, buflen);
        if (size != buflen) {
          printf("Error writing to file: %zu bytes written\n", size);
        }
        close(file);
      }
    }

    // display compilation error if one is thrown
    static char **strtab = NULL;
    static size_t max_strlen = 0;
    static unsigned num_lines = 0;
    static struct nk_rect err_rect;
    if (g_errstr) {
      if (!strtab) {
        max_strlen = BUTTON_HEIGHT;
        strtab = ft_strsplit(g_errstr, '\n');
        for (num_lines = 0; strtab[num_lines]; ++num_lines)
          max_strlen = MAX__(ft_strlen(strtab[num_lines]), max_strlen);
        // line length * width_of(line)
        // num lines * height_of(line) + height_of(ok_button)
        // printf("%zu\n", max_strlen);
        err_rect.w = max_strlen * CHAR_WIDTH + WIDTH_BUF;
        err_rect.h =
            (num_lines * (ROW_HEIGHT + ROW_BUF)) + BUTTON_HEIGHT + HEIGHT_BUF;
        err_rect.y = OFF_Y - err_rect.h;
        err_rect.x = OFF_X - err_rect.w;
      }

      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "ERROR",
                         NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE, err_rect)) {
        nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);

        for (int ii = 0; strtab[ii]; ++ii)
          nk_label(ctx, strtab[ii], NK_TEXT_LEFT);

        nk_layout_row_static(ctx, BUTTON_HEIGHT, BUTTON_HEIGHT, 1);
        if (nk_button_label(ctx, "ok")) {
          free(g_errstr);
          g_errstr = NULL;
          ft_free_str_tab(&strtab);
          max_strlen = 0;
          num_lines = 0;
        }
        nk_popup_end(ctx);
      } else {
        free(g_errstr);
        g_errstr = NULL;
        ft_free_str_tab(&strtab);
        max_strlen = 0;
        num_lines = 0;
      }
    } else if (strtab) {
      ft_free_str_tab(&strtab);
      max_strlen = 0;
      num_lines = 0;
    }

    // compile button
    if (nk_button_label(ctx, "Compile")) {
      int file = open(COR_FILENAME, O_RDWR | O_CREAT | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (file < 0) {
        puts("Error opening " COR_FILENAME);
        cantopen = COR_FILENAME;
      } else {
        size_t size = write(file, edit_buf, sizeof(edit_buf));
        if (size < NK_LEN(edit_buf)) {
          printf("Error writing to file: %zu bytes written\n", size);
        }

        size_t filesize;
        lseek(file, 0, SEEK_SET);
        char *s = assemble(file, &filesize);
        close(file);

        file = open(COR_FILENAME, O_RDWR | O_CREAT | O_TRUNC,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (file < 0) {
          puts("Error opening " COR_FILENAME);
          cantopen = COR_FILENAME;
        } else {
          size = write(file, s, filesize);

          if (size < filesize) {
            printf("Error: wrote %d bytes instead of %zu\n", err, filesize);
          }
          close(file);
          if (!g_errstr)
            strncpy(open_buf, COR_FILENAME, 17);
        }
      }
    }

    // un-compile button for disassembling file
    if (nk_button_label(ctx, "Un-compile")) {
      int file = open(open_buf, O_RDONLY);
      if (file < 0) {
        printf("Error opening %s\n", open_buf);
        cantopen = open_buf;
      } else {
        size_t filesize = 0;
        size_t size;
        // lseek(file, 0, SEEK_SET);
        char *s = disassemble(file, &filesize);
        close(file);
        char *filename = ASM_FILENAME;

        file = open(filename, O_RDWR | O_CREAT | O_TRUNC,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (file < 0) {
          printf("Error opening %s\n", filename);
          cantopen = filename;
        } else {
          size = write(file, s, filesize);

          if (size < filesize) {
            printf("Error: wrote %d bytes instead of %zu\n", err, filesize);
          }
          close(file);
          if (!g_errstr)
            strncpy(open_buf, filename, 17);
        }
      }
    }
  }

  nk_end(ctx); // end a nk_window
}

// win_edit contains an assembly editor, compiler, and loader.
static void win_edit(struct nk_context *ctx, struct s_cpu *cpu) {
  (void)cpu;

  if (nk_begin(
          ctx, "Edit",
          nk_rect(EDIT_RECT_X, EDIT_RECT_Y, EDIT_RECT_WIDTH, EDIT_RECT_HEIGHT),
          NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE |
              NK_WINDOW_TITLE)) {
    nk_layout_row_dynamic(ctx, EDIT_RECT_HEIGHT - 55, 1);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, edit_buf,
                                   sizeof(edit_buf) - 1, nk_filter_default);
  }
  nk_end(ctx); // end a nk_window
}

static void error_callback(int e, const char *d) {
  OUT("Error %d: %s\n", e, d);
}

static void dump_process(struct s_process *proc) {
  OUT("pid(%4d) player(%d) pc(%4d) last_live(%5d) carry(%d) opcode(%02x) "
         "ins_time(%4d)",
         proc->pid, proc->player, proc->pc, proc->last_live, proc->carry,
         proc->opcode, proc->instruction_time);
  for (int i = 0; i < 16; i++) {
    OUT(" r%d(%08x)", i + 1, proc->registers[i]);
  }
  OUT("\n");
}

static void vm_dump_byte(struct s_cpu *cpu, int idx, int space) {
  char buf[3];

  buf[0] = g_bytes_lower[cpu->program[idx]][0];
  buf[1] = g_bytes_lower[cpu->program[idx]][1];
  buf[2] = 0;
  if (space)
    OUT(" ");
  if (g_color) {
    struct s_process *cur = cpu->processes;
    while (cur != 0) {
      if (idx == cur->pc) {
        switch (cur->player) {
        case -1:
          OUT("\e[30;41m");
          break;
        case -2:
          OUT("\e[30;42m");
          break;
        case -3:
          OUT("\e[30;44m");
          break;
        case -4:
          OUT("\e[30;45m");
          break;
        }
        break;
      }
      cur = cur->next;
    }
    if (cur == 0) {
      if (g_mem_colors[idx].player) {
        switch (g_mem_colors[idx].player) {
        case -1:
          if (g_mem_colors[idx].writes == 0)
            OUT("\e[31m");
          else
            OUT("\e[91;4m");
          break;
        case -2:
          if (g_mem_colors[idx].writes == 0)
            OUT("\e[32m");
          else
            OUT("\e[92;4m");
          break;
        case -3:
          if (g_mem_colors[idx].writes == 0)
            OUT("\e[34m");
          else
            OUT("\e[92;4m");
          break;
        case -4:
          if (g_mem_colors[idx].writes == 0)
            OUT("\e[35m");
          else
            OUT("\e[95;4m");
          break;
        }
      }
    }
  }
  OUT("%s", buf);
  if (g_color)
    OUT("\e[0m");
}

#define DUMP_POW2 6
#define DUMP_WIDTH(X) ((X) << DUMP_POW2)

static void vm_dump_core(struct s_cpu *cpu) {
  register int ii, jj, kk;
  int max = MEM_SIZE >> DUMP_POW2;

  if ((g_verbose & OPT_DBGOUT) && (g_verbose & OPT_INTLDBG))
    OUT("-= [ CORE DUMP ] =-\n"
           "cpu->active(%d)\n"
           "cpu->clock(%d)\n",
           cpu->active, cpu->clock);
  for (ii = 0; ii < max; ii++) {
    for (kk = 0; kk < DUMP_WIDTH(1); ++kk) {         // 64
      if (cpu->program[DUMP_WIDTH(ii) + kk] != 0x00) // ii << 6
        break;
    }
    if (!(g_verbose <= -1) || kk != DUMP_WIDTH(1)) { // 64
      OUT("0x%04x : ", DUMP_WIDTH(ii));        // ii << 6
      for (jj = 0; jj < DUMP_WIDTH(1); jj++) {       // 64
        vm_dump_byte(cpu, DUMP_WIDTH(ii) + jj, jj);  // ii << 6
      }
      OUT("\n");
    }
  }
}

void vm_dump_processes(struct s_cpu *cpu) {
  struct s_process *cur = cpu->processes;
  if ((g_verbose & OPT_DBGOUT) && (g_verbose & OPT_INTLDBG))
    OUT("-= [ PROCESS DUMP ] =-\n");
  while (cur != 0) {
    dump_process(cur);
    cur = cur->next;
  }
}

void vm_dump_state(struct s_cpu *cpu) {
  if (g_dump_processes)
    vm_dump_processes(cpu);
  if (g_dump)
    vm_dump_core(cpu);
  if (g_verbose & OPT_INTLDBG) {
    OUT("DBG: g_instruction_calls[] {\n");
    for (int ii = 0; ii < NUM_OPS + 1; ++ii) {
      OUT("\t%6s[%2d] = %d,\n", g_op_tab[ii].name, ii,
             g_instruction_calls[ii]);
    }
    OUT("}\n");
  }
}

#define DOUT(FD, ...) dprintf(FD, __VA_ARGS__)

static void usage(const char *msg, const char *ext) {
  if (msg && ext)
    DOUT(STDERR_FILENO, "%s%s\n", msg, ext);
  else if (msg)
    DOUT(STDERR_FILENO, "%s\n", msg);
  DOUT(STDERR_FILENO, "Usage: corewar [OPTION]... FILE...\n"
                  "Try 'corewar -h' for more information.\n");
  exit((msg != 0 || ext != 0) && opterr != 0);
}

static void usage_help(void) {
  DOUT(STDERR_FILENO, "Usage: corewar [OPTION]... FILE...\n"
                  "Run each file in the Corewar virtual machine\n"
                  "Example: corewar -v 0 champ.cor zork.cor\n\n"
                  "Options:\n"
                  "  -a\tenable output from AFF instruction\n"
                  "  -b\trun GUI in background\n"
                  "  -c\tenable color in core dump\n"
                  "  -d\tdump core memory after NUM cycles\n"
                  "  -h\tdisplay this help output\n"
                  "  -l\tcall 'pause()' at the end of main\n"
                  "  -n\tenable graphical visualizer of vm\n"
                  "  -p\tdump processes when dumping core\n"
                  "  -v\tverbosity level\n"
                  "\t\t 0 - minimal output\n"
                  "\t\t 1 - live calls\n"
                  "\t\t 2 - current cycle\n"
                  "\t\t 4 - current instruction\n"
                  "\t\t 8 - process removal/death\n"
                  "\t\t16 - current program counter value\n"
                  "\t\t32 - instruction execution debug output\n"
                  "\t\t64 - internal vm debug output\n");
  exit(0);
}

static void corewar_gui(struct s_cpu *cpu) {
  // Platform
  static GLFWwindow *win;
  int width = 0, height = 0;
  struct nk_context *ctx;
  struct nk_colorf bg;

  // GLFW
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    DOUT(STDERR_FILENO, "Fatal error: [GFLW] failed to init!\n");
    exit(1);
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "corewar", NULL, NULL);
  glfwMakeContextCurrent(win);
  glfwGetWindowSize(win, &width, &height);

  // OpenGL
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glewExperimental = 1;
  if (glewInit() != GLEW_OK) {
    DOUT(STDERR_FILENO, "Failed to setup GLEW\n");
    exit(1);
  }

  ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
  // Load Fonts: if none of these are loaded a default font will be used
  // Load Cursor: if you uncomment cursor loading please hide the cursor
  {
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    nk_glfw3_font_stash_end();
  }

  bg.r = 0.55f, bg.g = 0.55f, bg.b = 0.55f, bg.a = 1.0f;

  set_color(ctx);
  // mainloop
  while (!glfwWindowShouldClose(win)) {
    // Input
    glfwPollEvents();
    nk_glfw3_new_frame();

    // check if each window should be shown and do that.

    win_open(ctx, cpu);
    win_edit(ctx, cpu);
    win_graph(ctx, cpu);
    win_debug(ctx, cpu);

    // Draw
    glfwGetWindowSize(win, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    // IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
    // with blending, scissor, face culling, depth test and viewport and
    // defaults everything back into a default state.
    // Make sure to either a.) save and restore or b.) reset your own state
    // after rendering the UI.
    nk_glfw3_render(NK_ANTI_ALIASING_OFF, MAX_VERTEX_BUFFER,
                    MAX_ELEMENT_BUFFER);
    glfwSwapBuffers(win);
  }
  nk_glfw3_shutdown();
  glfwTerminate();
}

// A valid numeric argument consists of only ascii digits
static int valid_number_arg(const char *str) {
  while (*str) {
    if (ISDIGIT(*str) == 0)
      return 0;
    ++str;
  }
  return 1;
}

int main(int argc, char *argv[]) {
  int ch = 0;
  int ii = 0;
  int dump_cycles = 0;
  FILE *f;
  static struct s_cpu cpu;

  g_gui = g_enable_aff = g_color = g_dump = g_leaks =
      g_dump_processes = g_verbose = 0;
  while ((ch = getopt(argc, argv, "acd:hlnpv:")) != -1) {
    switch (ch) {
    case 'a':
      g_enable_aff = 1;
      break;
    case 'c':
      g_color = 1;
      break;
    case 'd':
      g_dump = 1;
      if (optarg && valid_number_arg(optarg))
        dump_cycles = atoi(optarg);
      else
        usage("corewar: invalid \'-d\' argument: ", optarg);
      break;
    case 'h':
      usage_help();
      break;
    case 'l':
      g_leaks = 1;
      break;
    case 'n':
      g_gui = 1;
      break;
    case 'p':
      g_dump_processes = 1;
      break;
    case 'v':
      if (optarg && valid_number_arg(optarg))
        g_verbose = atoi(optarg);
      else
        usage("corewar: invalid \'-v\' argument: ", optarg);
      break;
    case '?':
    default:
      usage(0, 0);
      break;
    }
  }
  if (g_color || g_gui) {
    g_mem_colors = calloc(MEM_SIZE, sizeof(*g_mem_colors));
    assert(g_mem_colors != NULL);
  }
  argc -= optind;
  argv += optind;
  if (argc > MAX_PLAYERS || argc < 0) {
    DOUT(STDERR_FILENO, "Error: invalid number of arguments: %d\n", argc);
    return (1);
  }

  // make our cpu and program
  cpu = new_cpu();

  // load in .cor files
  while (ii < argc) {
    f = fopen(*argv, "r");
    if (f == NULL) {
      perror("Fatal error");
      return 1;
    }
    if (ii == 0)
      OUT("Introducing contestants...\n");
    int len = load_file(&cpu, f, (argc - (argc - ii)) * (MEM_SIZE / MAX_PLAYERS), ii + 1);
    if (len < 1 || (len - sizeof(header_t)) >= CHAMP_MAX_SIZE) {
      DOUT(STDERR_FILENO, "Fatal error: invalid champion file: %s\n", *argv);
      return 1;
    }
    if (fclose(f) != 0) {
      perror("Fatal error");
      return 1;
    }
    ++ii;
    ++argv;
  }

  if (cpu.processes == 0 && g_gui == false) {
    usage(0, 0);
  }

  // GUI stuff
  if (g_gui) {
    corewar_gui(&cpu);
  } else {
    while (cpu.active != 0) {
      if (g_dump && cpu.clock == dump_cycles) {
        vm_dump_state(&cpu);
        break;
      }
      cpu.step(&cpu);
    }
    // Display the winner
    if (cpu.winner != -1 && cpu.processes == NULL)
      OUT("Contestant %d, \"%s\", has won !\n", cpu.winner + 1,
             cpu.players[cpu.winner].name);
    else if (cpu.processes == NULL)
      OUT("Stalemate. (This shouldn't happen)\n");
  }
  cpu_cleanup(&cpu);
  if (g_color | g_gui)
    free(g_mem_colors);
  if (g_leaks)
    pause();
  return 0;
}
