#include <stdint.h>
#include "process.h"
#include "auth.h"

static process_t process_table[PROCESS_MAX];
static process_t *current_process;
static int next_pid = 1;

static void copy_name(char *destination, const char *source) {
    int index = 0;
    while (source[index] && index < PROCESS_NAME_SIZE - 1) {
        destination[index] = source[index];
        index++;
    }
    destination[index] = '\0';
}

void process_init(void) {
    for (int index = 0; index < PROCESS_MAX; index++) process_table[index].state = PROCESS_UNUSED;
    current_process = &process_table[0];
    current_process->pid = next_pid++;
    current_process->parent_pid = 0;
    current_process->uid = auth_current_uid();
    current_process->state = PROCESS_RUNNING;
    copy_name(current_process->name, "kernel");
}

int process_create(const char *name, process_entry_t entry, void *argument) {
    for (int index = 0; index < PROCESS_MAX; index++) {
        if (process_table[index].state == PROCESS_UNUSED) {
            process_table[index].pid = next_pid++;
            process_table[index].parent_pid = current_process ? current_process->pid : 0;
            process_table[index].uid = process_getuid();
            process_table[index].state = PROCESS_READY;
            process_table[index].entry = entry;
            process_table[index].argument = argument;
            copy_name(process_table[index].name, name);
            return process_table[index].pid;
        }
    }
    return -1;
}

int process_getpid(void) { return current_process ? current_process->pid : 0; }

int process_getuid(void) {
    if (!current_process) return (int)auth_current_uid();
    if (current_process->pid == 1) current_process->uid = auth_current_uid();
    return (int)current_process->uid;
}

void process_exit(int status) {
    (void)status;
    if (current_process) current_process->state = PROCESS_ZOMBIE;
}

void process_yield(void) {
    if (current_process && current_process->state == PROCESS_RUNNING) current_process->state = PROCESS_READY;
    process_schedule_once();
}

int process_schedule_once(void) {
    for (int index = 0; index < PROCESS_MAX; index++) {
        process_t *candidate = &process_table[index];
        if (candidate->state != PROCESS_READY || !candidate->entry) continue;
        current_process = candidate;
        current_process->state = PROCESS_RUNNING;
        current_process->entry(current_process->argument);
        if (current_process->state == PROCESS_RUNNING) current_process->state = PROCESS_ZOMBIE;
        current_process = &process_table[0];
        current_process->state = PROCESS_RUNNING;
        return candidate->pid;
    }
    return 0;
}

const process_t *process_current(void) { return current_process; }

static process_context_t exec_context;
static int exec_exit_status;

int process_exec(uint32_t entry_point, uint32_t user_stack_top) {
    exec_exit_status = 0;
    if (process_context_save(&exec_context) == 0) {
        process_enter_user_mode(entry_point, user_stack_top);
    }
    return exec_exit_status;
}

void process_exit_user(int status) {
    exec_exit_status = status;
    process_context_restore(&exec_context, 1);
}