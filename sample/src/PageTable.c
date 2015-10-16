#include "hardware.h"
#include "PageTable.h"

int make_page_table(pte *page_table, int max_pages) {
    // Make a dynamically allocated array of max_pages pte pointers
    // and assign it to page_table.
    // Return a return code indicating success or failure
}

int add_to_page_table(pte *page_table, pte next_page, int current_count) {
    // Add next_page to the the page_table array at page_table[current_count+1]
    // return the updated current count (or -1 on error)
} 
