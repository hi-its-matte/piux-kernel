#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define PROCESS_MAX 32
#define PROCESS_NAME_SIZE 32

typedef void (*process_entry_t)(void *argument);

typedef enum {
    PROCESS_UNUSED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_SLEEPING,
    PROCESS_ZOMBIE
} process_state_t;

typedef struct {
    int pid;
    int parent_pid;
    uint32_t uid;
    process_state_t state;
    process_entry_t entry;
    void *argument;
    char name[PROCESS_NAME_SIZE];
} process_t;

void process_init(void);
int process_create(const char *name, process_entry_t entry, void *argument);
int process_getpid(void);
int process_getuid(void);
void process_exit(int status);
void process_yield(void);
int process_schedule_once(void);
const process_t *process_current(void);

#endif