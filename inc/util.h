#ifndef UTIL_H
# define UTIL_H

// for use with lstdel.  Deletes elements of list
void free_(void *a, size_t b);

int get_param(uint8_t byte, int par);
uint8_t read_mem_byte(uint8_t *program, uint32_t idx);
uint16_t read_mem_word(uint8_t *program, uint32_t idx);
uint32_t read_mem_long(uint8_t *program, uint32_t idx);
uint64_t read_mem_quad(uint8_t *program, uint32_t idx);

#endif
