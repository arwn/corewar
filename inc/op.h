#ifndef OP_H
#define OP_H

#define IND_SIZE 2
#define REG_SIZE 4
#define DIR_SIZE REG_SIZE

#define REG_ARG_SIZE 1
#define IND_ARG_SIZE 2
#define DIR_ARG_SIZE 4

#define SPECIAL_DIR_SIZE IND_ARG_SIZE

#define REG_CODE 1
#define DIR_CODE 2
#define IND_CODE 3

#define MAX_ARGS_NUMBER 4
#define MAX_PLAYERS 4
#define MEM_SIZE (4 * 1024)
#define IDX_MOD (MEM_SIZE / 8)
#define CHAMP_MAX_SIZE (MEM_SIZE / 6)

#define NUM_BASE_OPS (16)
#define NUM_EXT_OPS (2)
#define NUM_OPS (NUM_BASE_OPS + NUM_EXT_OPS)

#define COMMENT_CHAR '#'
#define COMMENT_CHAR_ALT ';'
#define LABEL_CHAR ':'
#define DIRECT_CHAR '%'
#define SEPARATOR_CHAR ','

#define LABEL_CHARS "abcdefghijklmnopqrstuvwxyz_0123456789"

#define NAME_CMD_STRING ".name"
#define COMMENT_CMD_STRING ".comment"

#define REG_NUMBER 16

#define CYCLE_TO_DIE 1536
#define CYCLE_DELTA 50
#define NBR_LIVE 21
#define MAX_CHECKS 10

typedef unsigned char t_arg_type;
#define ENC_SIZE (sizeof(t_arg_type))

#define T_REG 1 // Registry
#define T_DIR 2 // Direct
#define T_IND 4 // Indirect
#define T_LAB 8 // Label

#define PROG_NAME_LENGTH (128)
#define COMMENT_LENGTH (2048)
#define COREWAR_EXEC_MAGIC 0x00ea83f3
#define COREWAR_EXTENDED_EXEC_MAGIC (COREWAR_EXEC_MAGIC + 0x411c102d)

// TODO: calculate based on args in main
#define OFFSET_1P_P1 0
#define OFFSET_2P_P1 OFFSET_1P_P1
#define OFFSET_2P_P2 (MEM_SIZE / 2)
#define OFFSET_3P_P1 OFFSET_1P_P1
#define OFFSET_3P_P2 (MEM_SIZE / 3)
#define OFFSET_3P_P3 ((MEM_SIZE / 3) * 2)
#define OFFSET_4P_P1 OFFSET_1P_P1
#define OFFSET_4P_P2 (MEM_SIZE / 4)
#define OFFSET_4P_P3 ((MEM_SIZE / 4) * 2)
#define OFFSET_4P_P4 ((MEM_SIZE / 4) * 3)

typedef struct header_s {
  unsigned int magic;
  char prog_name[PROG_NAME_LENGTH + 1];
  unsigned int prog_size;
  char comment[COMMENT_LENGTH + 1];
} header_t;

typedef struct s_op {
  char *name;
  int numargs;
  int argtypes[MAX_ARGS_NUMBER];
  int opcode;
  int cycles_to_exec;
  char *desc;
  int param_encode;
  int direct_size; // 0 - DIR_SIZE = DIR_SIZE
                   // 1 - DIR_SIZE = IND_SIZE
} t_op;

extern t_op g_op_tab[NUM_OPS + 1];

#endif
