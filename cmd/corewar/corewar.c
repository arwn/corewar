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

#define WINDOW_WIDTH 1200  // 1200
#define WINDOW_HEIGHT 1350 // 800

#define MAX_VERTEX_BUFFER 512 * 1536  // 512 * 1024 increase to fix buggy ui
#define MAX_ELEMENT_BUFFER 128 * 1536 // 128 * 1024

// open_buf is the buffer for the pathname inside the open window.
char open_buf[PATH_MAX] = {"./tests/aff1.s"};

// edit_buf is the buffer in which the editor holds it's text
char edit_buf[4096] = {"beans haha lol."};

// the following macros are the bit masks for determining which windows to
// display.

// instruction_calls is the number of instruction calls in the current program.
int instruction_calls[18] = {0};

// running keeps track of if the program should be stepped through
// automatically.
bool running = 0;

char winbuf[64] = "none";

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
  if (nk_begin(ctx, "graph", nk_rect(300, 30, 800, 250),
               NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE |
                   NK_WINDOW_TITLE)) {
    static char *labels[] = {"live", "ld",   "st",    "add", "sub", "and",
                             "or",   "xor",  "zjmp",  "ldi", "sti", "fork",
                             "lld",  "lldi", "lfork", "aff"};
    nk_layout_row_dynamic(ctx, 150, 1);
    nk_chart_begin(ctx, NK_CHART_COLUMN, NK_LEN(instruction_calls), 0, 128);
    for (unsigned long i = 0; i < NK_LEN(instruction_calls); i++) {
      nk_chart_push(ctx, instruction_calls[i] % 128);
    }
    nk_chart_end(ctx);
    nk_layout_row_dynamic(ctx, 20, 17);
    for (unsigned long i = 0; i < NK_LEN(labels); i++) {
      nk_label(ctx, labels[i], NK_TEXT_CENTERED);
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

extern char g_mem_tab[4096];
// win_debug displays the program and buttons to step through.
static void win_debug(struct nk_context *ctx, struct s_cpu *cpu) {
  if (nk_begin(ctx, "debug", nk_rect(40, 400, 1150, 800),
               NK_WINDOW_BORDER | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE |
                   NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {
    char buf[44];
    struct nk_color p1c; // red color for current player 1 instruction
    struct nk_color p2c; // green color for current player 2 instruction
    struct nk_color p3c; // blue color for current player 3 instruction
    struct nk_color p4c; // magenta color for current player 4 instruction
    p1c.r = 255, p1c.g = 100, p1c.b = 100, p1c.a = 255;
    p2c.r = 100, p2c.g = 255, p2c.b = 100, p2c.a = 255;
    p3c.r = 100, p3c.g = 100, p3c.b = 255, p3c.a = 255;
    p4c.r = 255, p4c.g = 100, p4c.b = 255, p4c.a = 255;

    // top buttons
    nk_layout_row_static(ctx, 30, 80, 10);
    if (cpu->processes == 0 && cpu->winner != 0 && cpu->clock == 0 &&
        (cpu->num_checks != 0 || cpu->nbr_lives != 0)) {
      printf("winner is %d\n", cpu->winner);
    }
    if (nk_button_label(ctx, "step")) {
      if (cpu->processes != 0)
        cpu->step(cpu);
    }
    if (nk_button_label(ctx, "walk")) {
      if (cpu->processes != 0) {
        int cont = 0;
        while (cont == 0) {
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
    static int slider = 2;
    nk_slider_int(ctx, 2, &slider, 9, 1);

    double trash;
    double time = modf(glfwGetTime(), &trash);
    time *= 100;
    if (cpu->processes != 0 &&
        running) { // && (int)time % (10 - slider) == 0) {
      cpu->step(cpu);
    }
    snprintf(winbuf, sizeof(winbuf), "Winner: %d", cpu->winner);

    // top stats
    // nk_layout_row_static(ctx, 20, 120, 2);
    // nk_label(ctx, "Winner:", NK_TEXT_LEFT);
    nk_label(ctx, winbuf, NK_TEXT_CENTERED);
    // nk_layout_row_static(ctx, 20, 120, 2);
    snprintf(buf, sizeof(buf), "Active: %d", cpu->active);
    // nk_label(ctx, "Processes:", NK_TEXT_LEFT);
    nk_label(ctx, buf, NK_TEXT_CENTERED);
    snprintf(buf, sizeof(buf), "CTD: %d", cpu->cycle_to_die);
    // nk_label(ctx, "Cycle to die:", NK_TEXT_LEFT);
    nk_label(ctx, buf, NK_TEXT_CENTERED);
    snprintf(buf, sizeof(buf), "Cycle: %zu", cpu->clock);
    // nk_label(ctx, "cpu cycles:", NK_TEXT_LEFT);
    nk_label(ctx, buf, NK_TEXT_CENTERED);

    // print the registers IDEA: make separate windows for stats for each player
    if (cpu->processes != NULL) {
      for (int i = 0; i < REG_NUMBER; i++) {
        if (i % 16 == 0)
          nk_layout_row_dynamic(ctx, 15, 8);
        snprintf(buf, sizeof(buf), "r%02d[%08x]", i + 1,
                 cpu->processes->registers[i]);
        nk_label(ctx, buf, NK_TEXT_LEFT);
      }
    }

    // print the program
    for (register int i = 0; i < MEM_SIZE; i++) {
      if (i % 64 == 0)
        nk_layout_row_static(ctx, 7, 13, 64);

      buf[0] = g_bytes_upper[cpu->program[i]][0];
      buf[1] = g_bytes_upper[cpu->program[i]][1];
      buf[2] = 0;
      struct s_process *hd = cpu->first;
      while (hd != NULL) {
        if (hd->pc == i)
          break;
        hd = hd->next;
      }
      if (hd != NULL && hd->pc == i) {
        switch (*hd->registers) {
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
        default:
          nk_label(ctx, buf, NK_TEXT_LEFT);
          break;
        }
      } else
        nk_label(ctx, buf, NK_TEXT_LEFT);
    }
  }
  nk_end(ctx);
}

header_t h;
// load_file reads a file into readbuf. if the file starts with
// COREWAR_EXEC_MAGIC it is loaded via `cpu->load()' otherwise load_file reads
// the file into `edit_buf'. returns number of bytes read.
static int load_file(struct s_cpu *cpu, FILE *f, int location, int player) {
  const size_t maxsize = CHAMP_MAX_SIZE + sizeof(header_t);
  char readbuf[maxsize + 1];
  int magic = -1;
  long len = fread(readbuf, 1, maxsize, f);
  readbuf[maxsize] = 0;
  h = *(header_t *)readbuf;
  h.magic = ntohl(h.magic);
  h.prog_size = ntohl(h.prog_size);
  rewind(f);
  fread(&magic, sizeof(int), 1, f);
  magic = ntohl(magic);
  if (len > 0) {
    if (valid_header_p(h)) {
      cpu->load(cpu, readbuf + sizeof(header_t), len - sizeof(header_t),
                location);
      cpu->spawn_process(cpu, location, -player); // player id reg must be < 0
      printf("* Player %d, weighing %d bytes, \"%s\" (\"%s\") !\n", player,
             h.prog_size, h.prog_name, h.comment);
      for (int jj = location, ll = (len - sizeof(header_t)); jj < ll + location;
           ++jj) {
        g_mem_tab[jj] = player;
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
  static int cantopen = nk_false;

  if (nk_begin(ctx, "open", nk_rect(0, 30, 300, 180),
               NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE |
                   NK_WINDOW_TITLE)) {
    static uint32_t offset = 0;
    static int select = 0;

    nk_layout_row_dynamic(ctx, 30, 1);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, open_buf,
                                   sizeof(open_buf) - 1, nk_filter_ascii);

    // this button opens a file, reads it into a buffer and copies it to the
    // edit buffer or the debug window depending if it's a compiled corewar bin.
    if (nk_button_label(ctx, "open file") && open_buf[0]) {
      FILE *f = fopen(open_buf, "r");
      if (f == NULL) {
        puts("Can't open");
        cantopen = nk_true;
      } else {
        cantopen = !load_file(cpu, f, offset, select);
        fclose(f);
      }
    }

    // this box lets you select the region of memory you want to load the
    // program into.
    nk_layout_row_static(ctx, 25, 200, 1);
    static const char *players[] = {"One", "Two", "Three", "Four"};
    select = nk_combo(ctx, players, 4, select, 25, nk_vec2(200, 200));
    switch (select) {
    case 0:
      offset = 0;
      break;
    case 1:
      offset = 1024;
      break;
    case 2:
      offset = 1024 * 2;
      break;
    case 3:
      offset = 1024 * 3;
      break;
    }

    // error popup
    // bugs: "open" win freezes until you click on another window or mouse over
    // the background.
    if (cantopen) {
      static struct nk_rect s = {-10, -10, 200, 150};
      if (nk_popup_begin(ctx, NK_POPUP_STATIC, "ERROR",
                         NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE, s)) {
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "can't open file:", NK_TEXT_LEFT);
        nk_label(ctx, open_buf, NK_TEXT_LEFT);
        nk_layout_row_static(ctx, 30, 30, 1);
        if (nk_button_label(ctx, "ok")) {
          cantopen = nk_false;
        }
        nk_popup_end(ctx);
      } else {
        cantopen = nk_false;
      }
    }
  }
  nk_end(ctx); // end a nk_window
}

// win_edit contains an assembly editor, compiler, and loader.
static void win_edit(struct nk_context *ctx, struct s_cpu *cpu) {
  (void)cpu;
  if (nk_begin(ctx, "edit", nk_rect(0, 210, 300, 400),
               NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE |
                   NK_WINDOW_TITLE)) {
    nk_layout_row_dynamic(ctx, 300, 1);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, edit_buf,
                                   sizeof(edit_buf) - 1, nk_filter_default);
    nk_layout_row_static(ctx, 30, 80, 1);
    if (nk_button_label(ctx, "Compile")) {
      // remove("/tmp/corewar.cor");
      // int file = open("/tmp/corewar.cor", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR
      // | S_IWUSR | S_IXUSR);
      int file = open("/tmp/corewar.cor", O_RDWR | O_CREAT | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (file < 0) {
        puts("Error opening /tmp/corewar.cor");
        nk_end(ctx);
        return;
      }
      size_t size = write(file, edit_buf, sizeof(edit_buf));
      if (size < NK_LEN(edit_buf)) {
        printf("Error writing to file: %zu bytes written\n", size);
      }

      size_t filesize;
      lseek(file, 0, SEEK_SET);
      char *s = assemble(file, &filesize);
      // lseek(file, 0, SEEK_SET);
      close(file);

      file = open("/tmp/corewar.cor", O_RDWR | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (file < 0) {
        puts("Error opening /tmp/corewar.cor");
        nk_end(ctx);
        return;
      }
      // write(1, s, filesize);
      size = write(file, s, filesize);

      if (size < filesize) {
        printf("Error: wrote %d bytes instead of %zu\n", err, filesize);
      }
      close(file);
      strncpy(open_buf, "/tmp/corewar.cor", 17);
    }
  }
  nk_end(ctx); // end a nk_window
}

static void error_callback(int e, const char *d) {
  printf("Error %d: %s\n", e, d);
}

static void dump_process(struct s_process *proc, int num) {
  printf("-- [ PROCESS   %02d ] --\n", num);
  printf("proc(%d)->pc(%d)\n", proc->pid, proc->pc);
  printf("proc(%d)->last_live(%d)\n", proc->pid, proc->last_live);
  printf("proc(%d)->carry(%d)\n", proc->pid, proc->carry);
  for (int i = 0; i < 16; i++) {
    printf("proc(%d)->r%02d(%08x)\n", proc->pid, i, proc->registers[i]);
  }
}

char g_mem_tab[4096] = {0};
static void vm_dump_byte(struct s_cpu *cpu, int idx, int space) {
  char buf[3];

  buf[0] = g_bytes_lower[cpu->program[idx]][0];
  buf[1] = g_bytes_lower[cpu->program[idx]][1];
  buf[2] = 0;
  if (space)
    printf(" ");
  if (f_color) {
    struct s_process *cur = cpu->first;
    while (cur != 0) {
      if (idx == cur->pc) {
        switch (cur->registers[0]) {
        case -1:
          printf("\e[30;41m");
          break;
        case -2:
          printf("\e[30;42m");
          break;
        case -3:
          printf("\e[30;44m");
          break;
        case -4:
          printf("\e[30;45m");
          break;
        }
        break;
      }
      cur = cur->next;
    }
    if (g_mem_tab[idx] && cur == 0) {
      switch (g_mem_tab[idx]) {
      case 1:
        printf("\e[31m");
        break;
      case 2:
        printf("\e[32m");
        break;
      case 3:
        printf("\e[34m");
        break;
      case 4:
        printf("\e[35m");
        break;
      }
    }
  }
  printf("%s", buf);
  if (f_color)
    printf("\e[0m");
}

static void vm_dump_core(struct s_cpu *cpu) {
  register int ii, jj, kk;
  int max = MEM_SIZE >> 6;

  printf("-= [ CORE DUMP ] =-\n"
         "cpu->active(%d)\n"
         "cpu->clock(%zu)\n",
         cpu->active, cpu->clock);
  for (ii = 0; ii < max; ii++) {
    for (kk = 0; kk < 64; ++kk) {
      if (cpu->program[(ii << 6) + kk] != 0x00)
        break;
    }
    if (!(f_verbose <= -1) || kk != 64) {
      printf("0x%04x : ", ii << 6);
      for (jj = 0; jj < 64; jj++) {
        vm_dump_byte(cpu, (ii << 6) + jj, jj);
      }
      printf("\n");
    }
  }
}

void vm_dump_processes(struct s_cpu *cpu) {
  struct s_process *cur = cpu->first;
  int n = 0;

  printf("-= [ PROCESS DUMP ] =-\n");
  while (cur != 0) {
    dump_process(cur, n);
    cur = cur->next;
    ++n;
  }
}

void vm_dump_state(struct s_cpu *cpu) {
  if (f_dump_processes)
    vm_dump_processes(cpu);
  if (f_dump)
    vm_dump_core(cpu);
}

char *bin;
static void usage(const char *msg, const char *ext) {
  if (msg && ext)
    fprintf(stderr, "%s%s\n", msg, ext);
  else if (msg)
    fprintf(stderr, "%s\n", msg);
  fprintf(stderr, "Usage: corewar [OPTION]... FILE...\n"
                  "Try 'corewar -h' for more information.\n");
  exit(1);
}

static void usage_help(void) {
  fprintf(stderr, "Usage: corewar [OPTION]... FILE...\n"
                  "Run each file in the Corewar virtual machine\n"
                  "Example: corewar -v 0 champ.cor zork.cor\n\n"
                  "Options:\n"
                  "  -c\t\tenable color in core dump\n"
                  "  -d\t\tdump core memory after NUM cycles\n"
                  "  -h\t\tdisplay this help output\n"
                  "  -l\t\tcall 'pause()' at the end of main\n"
                  "  -p\t\tdump processes when dumping core\n"
                  "  -r\t\tdisable graphical visualizer of vm\n"
                  "  -v\t\tverbosity level\n"
                  "\t\t\t 0 - minimal output\n"
                  "\t\t\t 1 - live calls\n"
                  "\t\t\t 2 - current cycle\n"
                  "\t\t\t 4 - current instruction\n"
                  "\t\t\t 8 - process removal/death\n"
                  "\t\t\t16 - current program counter value\n"
                  "\t\t\t32 - instruction execution debug output\n"
                  "\t\t\t64 - internal vm debug output\n");
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
    fprintf(stdout, "[GFLW] failed to init!\n");
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
    fprintf(stderr, "Failed to setup GLEW\n");
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

int main(int argc, char *argv[]) {
  int ch = 0;
  int ii = 0;
  size_t dump_cycles = 0;
  FILE *f;
  static struct s_cpu cpu;

  f_color = f_dump = f_leaks = f_dump_processes = f_verbose = 0;
  f_gui = 1;
  bin = *argv;
  while ((ch = getopt(argc, argv, "cd:hlprv:")) != -1) {
    switch (ch) {
    case 'c':
      f_color = 1;
      break;
    case 'd':
      f_dump = 1;
      if (optarg && ISDIGIT(*optarg))
        dump_cycles = atoi(optarg);
      else
        usage("corewar: invalid \'-d\' argument: ", optarg);
      break;
    case 'h':
      usage_help();
      break;
    case 'l':
      f_leaks = 1;
      break;
    case 'p':
      f_dump_processes = 1;
      break;
    case 'r':
      f_gui = 0;
      break;
    case 'v':
      if (optarg && ISDIGIT(*optarg))
        f_verbose = atoi(optarg);
      else
        usage("corewar: invalid \'-v\' argument: ", optarg);
      break;
    case '?':
    default:
      usage(0, 0);
      break;
    }
  }
  argc -= optind;
  argv += optind;

  // make our cpu and program
  cpu = new_cpu();

  // table for determining the offset based on number of players
  int offsets[4][4] = {
      {OFFSET_1P_P1, 0, 0, 0},
      {OFFSET_2P_P1, OFFSET_2P_P2, 0, 0},
      {OFFSET_3P_P1, OFFSET_3P_P2, OFFSET_3P_P3, 0},
      {OFFSET_4P_P1, OFFSET_4P_P2, OFFSET_4P_P3, OFFSET_4P_P4},
  };
  if (argc > 4 || argc < 0)
    return 56;

  // load in .cor files
  while (ii < argc) {
    f = fopen(*argv, "r");
    if (f == NULL) {
      perror("Fatal error");
      return 1;
    }
    if (ii == 0)
      printf("Introducing contestants...\n");
    load_file(&cpu, f, offsets[argc - 1][ii], ii + 1);
    if (fclose(f) != 0)
      return 1;
    ++ii;
    ++argv;
  }

  if (cpu.processes == 0 && f_gui == false) {
    usage(0, 0);
  }

  // GUI stuff
  if (f_gui) {
    corewar_gui(&cpu);
  } else {
    while (cpu.active && cpu.processes) {
      if (f_verbose & OPT_CYCLES) {
        printf("It is now cycle %zu\n", cpu.clock + 1);
      }
      if (f_dump && cpu.clock == dump_cycles) {
        vm_dump_state(&cpu);
        break;
      }
      cpu.step(&cpu);
    }
    if (cpu.winner != 0 && cpu.processes == NULL)
      printf("Winner is player %d, \"%s\"\n", cpu.winner, h.prog_name);
    else if (cpu.processes == NULL)
      printf("Stalemate.\n");
    while (cpu.active != 0 && cpu.processes != NULL) {
      cpu.kill_process(&cpu);
    }
  }
  if (f_leaks)
    pause();
  return 0;
}
