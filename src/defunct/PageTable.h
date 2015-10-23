#ifndef _PAGE_TABLE_H_
#define _PAGE_TABLE_H_
/*
 * PageTable.h
 */
#include <hardware.h>

// A function that creates an empty page table (an array of pte entries)
int generate_page_table(struct pte *page_table, int max_pages);

int add_to_page_table(struct pte *page_table, struct pte next_page, int current_count);

#endif // _PAGE_TABLE_H_
