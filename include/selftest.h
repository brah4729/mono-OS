#ifndef SELFTEST_H
#define SELFTEST_H

#include "types.h"

void selftest_report(const char* name, bool ok);
void selftest_cpu(void);      /* GDT/IDT/PIT/utoa — Task 3 */
void selftest_memory(void);   /* PMM/VMM/heap — Task 4 */
void selftest_sched(void);    /* context switch — Task 6 */
void selftest_syscall(void);  /* int 0x80 ABI — Task 7 */
void selftest_elf(void);      /* ELF64 validation — Task 7 */
void selftest_dorifs(void);   /* DoriFS dirents across block ends (needs mounted /) */

#endif
