#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define USER_SPACE_TOP 0xC0000000U
#define USER_SPACE_BASE 0x00400000U

void memory_init(void);
void *kmalloc(uint32_t size);
void kfree(void *address);
int memory_range_valid(const void *address, uint32_t size, int user_space);
int paging_enabled(void);

#endif