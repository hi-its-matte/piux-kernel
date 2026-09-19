#include <stdint.h>
#include "memory.h"

extern char __kernel_end;

#define USER_MEMORY_LIMIT (64U * 1024U * 1024U)
#define PAGE_ENTRIES 1024
#define IDENTITY_PAGE_COUNT (1024U * 1024U)

static uint32_t page_directory[PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_tables[IDENTITY_PAGE_COUNT] __attribute__((aligned(PAGE_SIZE)));
static uint32_t heap_end;
static int paging_active;

static uint32_t align_up(uint32_t value) {
    return (value + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static void enable_paging(uint32_t directory) {
    asm volatile ("mov %0, %%cr3\n\t"
                  "mov %%cr0, %%eax\n\t"
                  "or $0x80000000, %%eax\n\t"
                  "mov %%eax, %%cr0"
                  : : "r"(directory) : "eax", "memory");
}

void memory_init(void) {
    uint32_t page_count = IDENTITY_PAGE_COUNT;
    for (uint32_t index = 0; index < PAGE_ENTRIES; index++) page_directory[index] = 0;
    for (uint32_t page = 0; page < page_count; page++) {
        uint32_t flags = 0x03;
        if (page * PAGE_SIZE >= USER_SPACE_BASE) flags |= 0x04;
        page_tables[page] = page * PAGE_SIZE | flags;
    }
    for (uint32_t table = 0; table < page_count / PAGE_ENTRIES; table++) {
        uint32_t flags = table == 0 ? 0x03 : 0x07;
        page_directory[table] = (uint32_t)&page_tables[table * PAGE_ENTRIES] | flags;
    }
    heap_end = align_up((uint32_t)(uintptr_t)&__kernel_end);
    enable_paging((uint32_t)(uintptr_t)page_directory);
    paging_active = 1;
}

void *kmalloc(uint32_t size) {
    uint32_t address;
    if (size == 0) return 0;
    address = align_up(heap_end);
    size = align_up(size);
    if (address < heap_end || address + size < address || address + size >= USER_SPACE_BASE) return 0;
    heap_end = address + size;
    return (void *)(uintptr_t)address;
}

void kfree(void *address) { (void)address; }

int memory_range_valid(const void *address, uint32_t size, int user_space) {
    uint32_t start = (uint32_t)(uintptr_t)address;
    uint32_t end;
    if (size == 0 || start + size < start) return 0;
    end = start + size;
    if (user_space && (start < USER_SPACE_BASE || end > USER_SPACE_TOP || end > USER_MEMORY_LIMIT)) return 0;
    return start >= 0x00100000U && end <= 0xfffff000U;
}

int paging_enabled(void) { return paging_active; }