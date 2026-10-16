#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "vmm.h"
#include "isr.h"

#define MAX_PROCESSES       64
#define KERNEL_STACK_SIZE   8192   /* 8 KiB per kernel stack */
#define USER_STACK_SIZE     16384  /* 16 KiB per user stack */
#define USER_STACK_TOP      0x00007FFFFFFFF000UL

/* Process states */
typedef enum {
    PROCESS_STATE_FREE = 0,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_ZOMBIE,
} process_state_t;

/* Process control block */
typedef struct {
    uint32_t            pid;
    process_state_t     state;
    uint64_t            rsp;            /* Saved stack pointer */
    uint64_t            rbp;
    uint64_t            rip;
    page_table_t*       pml4;           /* Process address space (unused: shared kernel map) */
    uint64_t            kernel_stack;   /* Base of kernel stack allocation */
    uint64_t            user_stack;
    uint64_t            brk;
    int32_t             exit_code;
    uint32_t            parent_pid;
    char                name[64];
} process_t;

/* Process management API */
void        process_init(void);
int         process_create(const char* name, void (*entry)(void), bool is_user);
void        process_exit(int32_t code);
void        process_yield(void);
void        process_schedule(registers_t* regs);
uint32_t    process_get_pid(void);
process_t*  process_get_current(void);
void        process_block(uint32_t pid);
void        process_unblock(uint32_t pid);
int         process_wait(uint32_t pid);

/* Context switch — defined in boot/context_switch.asm */
extern void context_switch(uint64_t* old_rsp, uint64_t new_rsp);

#endif
