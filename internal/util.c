#include "asm.h"
#include <stdlib.h>

void free_(void *a, size_t b) {
  (void)b;
  if (b == sizeof(t_tok)) {
    t_tok *t = (t_tok *)(a);
    if ((t->type == label_def || t->type == bot_name ||
         t->type == bot_comment) &&
        t->str)
      free(t->str);
  } else if (b == sizeof(t_cmd)) {
  }
  free(a);
}

// Extract the PAR'th parameter type from the encoded BYTE TODO: delete
int get_param(t_arg_type byte, int par) {
  return (byte >> ((3 - par) * 2)) & (0b11);
}

// Prevent out of bounds accesses for cpu->program
int mod_idx(int idx) {
  int ret;

  ret = idx % MEM_SIZE;
  if (ret < 0)
    ret += MEM_SIZE;
  return ret;
}

// Read a single byte from buffer PROGRAM at IDX, swapping endianness
uint8_t read_mem_1(uint8_t *program, uint32_t idx) {
  return (program[idx % MEM_SIZE]);
}

// Read two bytes from buffer PROGRAM at IDX, swapping endianness
uint16_t read_mem_2(uint8_t *program, uint32_t idx) {
  return ((uint16_t)program[idx % MEM_SIZE] << 8) |
         ((uint16_t)program[(idx + 1) % MEM_SIZE]);
}

// Read four bytes from buffer PROGRAM at IDX, swapping endianness
uint32_t read_mem_4(uint8_t *program, uint32_t idx) {
  return ((uint32_t)program[idx % MEM_SIZE] << 24) |
         ((uint32_t)program[(idx + 1) % MEM_SIZE] << 16) |
         ((uint32_t)program[(idx + 2) % MEM_SIZE] << 8) |
         ((uint32_t)program[(idx + 3) % MEM_SIZE]);
}
