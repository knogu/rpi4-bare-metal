#ifndef MM_H
#define MM_H

#define PAGE_SHIFT 12
#define TABLE_SHIFT 9
#define SECTION_SHIFT (PAGE_SHIFT + TABLE_SHIFT)
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define SECTION_SIZE (1 << SECTION_SHIFT)

#define LOW_MEMORY (2* SECTION_SIZE)

#define ID_MAP_PAGES           3
#define HIGH_MAP_PAGES         6
#define ID_MAP_TABLE_SIZE      (ID_MAP_PAGES * PAGE_SIZE)
#define HIGH_MAP_TABLE_SIZE    (HIGH_MAP_PAGES * PAGE_SIZE)
#define ENTRIES_PER_TABLE      512
#define PGD_SHIFT              (PAGE_SHIFT + 3 * TABLE_SHIFT)
#define PUD_SHIFT              (PAGE_SHIFT + 2 * TABLE_SHIFT)
#define PMD_SHIFT              (PAGE_SHIFT + TABLE_SHIFT)
#define PUD_ENTRY_MAP_SIZE     (1 << PUD_SHIFT)
#define ID_MAP_SIZE            (8 * SECTION_SIZE)

#define HIGH_MAP_FIRST_START   (0x0 + LINEAR_MAP_BASE)
#define HIGH_MAP_FIRST_END     (0x3B400000 + LINEAR_MAP_BASE)
#define HIGH_MAP_SECOND_START  (0x40000000 + LINEAR_MAP_BASE)
#define HIGH_MAP_SECOND_END    (0x80000000 + LINEAR_MAP_BASE)
#define HIGH_MAP_THIRD_START   (0x80000000 + LINEAR_MAP_BASE)
#define HIGH_MAP_THIRD_END     (0xC0000000 + LINEAR_MAP_BASE)
#define HIGH_MAP_FOURTH_START  (0xC0000000 + LINEAR_MAP_BASE)
#define HIGH_MAP_FOURTH_END    (0xFC000000 + LINEAR_MAP_BASE)
#define HIGH_MAP_DEVICE_START  (0xFC000000 + LINEAR_MAP_BASE)
#define HIGH_MAP_DEVICE_END    (0x100000000 + LINEAR_MAP_BASE)

#define FIRST_START   (0x0)
#define FIRST_END     (0x3B400000)
#define SECOND_START  (0x40000000)
#define SECOND_END    (0x80000000)
#define THIRD_START   (0x80000000)
#define THIRD_END     (0xC0000000)
#define FOURTH_START  (0xC0000000)
#define FOURTH_END    (0xFC000000)
#define DEVICE_START  (0xFC000000)
#define DEVICE_END    (0x100000000)

#define MALLOC_START (0x40000000)
#define MALLOC_END   (0xFC000000)
#define MALLOC_SIZE  (MALLOC_END - MALLOC_START)
#define MALLOC_PAGES (MALLOC_SIZE / PAGE_SIZE)
#define THREAD_SIZE  PAGE_SIZE

#define LINEAR_MAP_BASE 0xffff000000000000

#ifndef __ASSEMBLER__

#include "types.h"

void memzero(u64 src, unsigned int n);
u64 get_kernel_page();
u64 get_free_page();
void free_page(u64 p);
void memcpy(u64* dst, u64* src, u64 bytes);

extern u64 _start;
#define KERNEL_START (&(_start))

extern u64 KERNEL_PA_BASE;

#endif /* __ASSEMBLER__ */

#endif /* MM_H */
