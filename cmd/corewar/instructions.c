#include "cpu.h"
#include "op.h"
#include "util.h"

// clang-format off
// must be included AFTER cpu.h
#include "instructions.h"
// clang-format on

t_inst g_inst_tab[NUM_OPS + 1] = {
    (t_inst)0,
    [e_live] = instruction_live,
    [e_ld] = instruction_ld,
    [e_st] = instruction_st,
    [e_add] = instruction_add,
    [e_sub] = instruction_sub,
    [e_and] = instruction_and,
    [e_or] = instruction_or,
    [e_xor] = instruction_xor,
    [e_zjmp] = instruction_zjmp,
    [e_ldi] = instruction_ldi,
    [e_sti] = instruction_sti,
    [e_fork] = instruction_fork,
    [e_lld] = instruction_lld,
    [e_lldi] = instruction_lldi,
    [e_lfork] = instruction_lfork,
    [e_aff] = instruction_aff,
    [e_nop] = instruction_nop,
    [e_kill] = instruction_kill,
};
