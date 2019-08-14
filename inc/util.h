#ifndef UTIL_H
# define UTIL_H

// for use with lstdel.  Deletes elements of list
void free_(void *a, size_t b);

int get_param(uint8_t byte, int par);
uint8_t read_mem_1(uint8_t *program, uint32_t idx);
uint16_t read_mem_2(uint8_t *program, uint32_t idx);
uint32_t read_mem_4(uint8_t *program, uint32_t idx);
uint64_t read_mem_8(uint8_t *program, uint32_t idx);

#endif
