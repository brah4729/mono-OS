#ifndef SELFTEST_H
#define SELFTEST_H

#include "types.h"

void selftest_report(const char* name, bool ok);
void selftest_cpu(void);      /* GDT/IDT/PIT/utoa — Task 3 */

#endif
