#ifndef ELF_H
#define ELF_H

#include <stdint.h>

/* Loads a static ELF32 executable from the filesystem into its own
   linked virtual addresses and reports its entry point. */
int elf_load(const char *path, uint32_t *entry_point);

#endif
