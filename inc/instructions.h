#ifndef INSTRUCTIONS_H
# define INSTRUCTIONS_H

// # include "cpu.h"

int				instruction_live(struct s_cpu *cpu, struct s_process *proc);
int				instruction_ld(struct s_cpu *cpu, struct s_process *proc);
int				instruction_st(struct s_cpu *cpu, struct s_process *proc);
int				instruction_add(struct s_cpu *cpu, struct s_process *proc);
int				instruction_sub(struct s_cpu *cpu, struct s_process *proc);
int				instruction_and(struct s_cpu *cpu, struct s_process *proc);
int				instruction_or(struct s_cpu *cpu, struct s_process *proc);
int				instruction_xor(struct s_cpu *cpu, struct s_process *proc);
int				instruction_zjmp(struct s_cpu *cpu, struct s_process *proc);
int				instruction_ldi(struct s_cpu *cpu, struct s_process *proc);
int				instruction_sti(struct s_cpu *cpu, struct s_process *proc);
int				instruction_fork(struct s_cpu *cpu, struct s_process *proc);
int				instruction_lld(struct s_cpu *cpu, struct s_process *proc);
int				instruction_lldi(struct s_cpu *cpu, struct s_process *proc);
int				instruction_lfork(struct s_cpu *cpu, struct s_process *proc);
int				instruction_aff(struct s_cpu *cpu, struct s_process *proc);

typedef int		(*t_inst)(struct s_cpu*, struct s_process *);

extern t_inst	inst_tab[NUM_OPS + 1];

#endif /* INSTRUCTIONS_H */
