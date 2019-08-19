/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   op.c                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zaz <marvin@42.fr>                         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2013/10/04 11:43:01 by zaz               #+#    #+#             */
/*   Updated: 2019/08/19 14:39:17 by acarlson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "op.h"
// clang-format off
t_op g_op_tab[NUM_OPS + 1] = {
    {0,       0, {0},                                            0,    1, 0,                 0, 0},
    {"live",  1, {T_DIR},                                        1,   10, "alive",           0, 0},
    {"ld",    2, {T_DIR|T_IND, T_REG},                           2,    5, "load",            1, 0},
    {"st",    2, {T_REG, T_IND|T_REG},                           3,    5, "store",           1, 0},
    {"add",   3, {T_REG, T_REG, T_REG},                          4,   10, "addition",        1, 0},
    {"sub",   3, {T_REG, T_REG, T_REG},                          5,   10, "subtraction",     1, 0},
    {"and",   3, {T_REG|T_DIR|T_IND, T_REG|T_IND|T_DIR, T_REG},  6,    6, "bitwise and",     1, 0},
    {"or",    3, {T_REG|T_IND|T_DIR, T_REG|T_IND|T_DIR, T_REG},  7,    6, "bitwise or",      1, 0},
    {"xor",   3, {T_REG|T_IND|T_DIR, T_REG|T_IND|T_DIR, T_REG},  8,    6, "bitwise xor",     1, 0},
    {"zjmp",  1, {T_DIR},                                        9,   20, "jump if zero",    0, 1},
    {"ldi",   3, {T_REG|T_DIR|T_IND, T_DIR|T_REG, T_REG},       10,   25, "load index",      1, 1},
    {"sti",   3, {T_REG, T_REG|T_DIR|T_IND, T_DIR|T_REG},       11,   25, "store index",     1, 1},
    {"fork",  1, {T_DIR},                                       12,  800, "fork",            0, 1},
    {"lld",   2, {T_DIR|T_IND, T_REG},                          13,   10, "long load",       1, 0},
    {"lldi",  3, {T_REG|T_DIR|T_IND, T_DIR|T_REG, T_REG},       14,   50, "long load index", 1, 1},
    {"lfork", 1, {T_DIR},                                       15, 1000, "long fork",       0, 1},
    {"aff",   1, {T_REG},                                       16,    2, "print",           1, 0},
	// new instructions
    {"nop",   0, {},                                            17,    1, "it's a nop",             0, 0},
    {"die",   1, {T_DIR},                                       18,   10, "kill process",           0, 0},
};
// clang-format on
