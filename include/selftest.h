#ifndef SELFTEST_H
#define SELFTEST_H

#include "types.h"

void selftest_report(const char* name, bool ok);
void selftest_cpu(void);      /* GDT/IDT/PIT/utoa — Task 3 */
void selftest_memory(void);   /* PMM/VMM/heap — Task 4 */
void selftest_sched(void);    /* context switch — Task 6 */

#endif
