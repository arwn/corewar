/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   op.h                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zaz <marvin@42.fr>                         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2013/10/04 11:33:27 by zaz               #+#    #+#             */
/*   Updated: 2019/08/16 13:32:27 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
** Toutes les tailles sont en octets.
** On part du principe qu'un int fait 32 bits. Est-ce vrai chez vous ?
*/

#ifndef OP_H
#define OP_H
#include <stdint.h>

#define IND_SIZE 2
#define REG_SIZE 1
#define DIR_SIZE 4
#define SPECIAL_DIR_SIZE 2

#define REG_CODE 1
#define DIR_CODE 2
#define IND_CODE 3

#define MAX_ARGS_NUMBER 4
#define ENC_SIZE 1
#define MAX_PLAYERS 4
#define MEM_SIZE (4 * 1024)
#define IDX_MOD (MEM_SIZE / 8)
#define CHAMP_MAX_SIZE (MEM_SIZE / 6)

#define NUM_OPS (16)

#define COMMENT_CHAR ';'
#define COMMENT_CHAR_ALT '#'
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

/*
**
*/

typedef char t_arg_type;

#define T_REG 1 // Registry
#define T_DIR 2 // Direct
#define T_IND 4 // Indirect
#define T_LAB 8 // Label

/*
**
*/

#define PROG_NAME_LENGTH (128)
#define COMMENT_LENGTH (2048)
#define COREWAR_EXEC_MAGIC 0x00ea83f3

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
  int direct_size; // 1 - DIR_SIZE = 4 bits
                   // 0 - DIR_SIZE *= 2
} t_op;

extern t_op g_op_tab[NUM_OPS + 1];

#endif
