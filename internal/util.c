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

// Extract the PAR'th parameter type from the encoded BYTE
int get_param(uint8_t byte, int par) {
  return (byte >> (8 - (par * 2))) & (0b11);
}

// Read a single byte from buffer PROGRAM at IDX, swapping endianness
uint8_t read_mem_byte(uint8_t *program, uint32_t idx) {
  return (program[idx % MEM_SIZE]);
}

// Read two bytes from buffer PROGRAM at IDX, swapping endianness
uint16_t read_mem_word(uint8_t *program, uint32_t idx) {
  return ((uint16_t)program[idx % MEM_SIZE] << 8) |
         ((uint16_t)program[(idx + 1) % MEM_SIZE]);
}

// Read four bytes from buffer PROGRAM at IDX, swapping endianness
uint32_t read_mem_long(uint8_t *program, uint32_t idx) {
  return ((uint32_t)program[idx % MEM_SIZE] << 24) |
         ((uint32_t)program[(idx + 1) % MEM_SIZE] << 16) |
         ((uint32_t)program[(idx + 2) % MEM_SIZE] << 8) |
         ((uint32_t)program[(idx + 3) % MEM_SIZE]);
}

// Read eight bytes from buffer PROGRAM at IDX, swapping endianness
uint64_t read_mem_quad(uint8_t *program, uint32_t idx) {
  return ((uint64_t)program[idx % MEM_SIZE] << 56 |
          (uint64_t)program[(idx + 1) % MEM_SIZE] << 48 |
          (uint64_t)program[(idx + 2) % MEM_SIZE] << 40 |
          (uint64_t)program[(idx + 3) % MEM_SIZE] << 32 |
          (uint64_t)program[(idx + 4) % MEM_SIZE] << 24 |
          (uint64_t)program[(idx + 5) % MEM_SIZE] << 16 |
          (uint64_t)program[(idx + 6) % MEM_SIZE] << 8 |
          (uint64_t)program[(idx + 7) % MEM_SIZE]);
}
