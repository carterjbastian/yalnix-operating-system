// linked_list.h


#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_
// Node data structure for linked lists
typedef struct ListNode ListNode;
struct ListNode { 
  int id;
  void *data; 
  ListNode *next;
  ListNode *prev;
};

// List data structure
typedef struct List { 
  ListNode *first; // void *?
} List;

// Functions related to the linked lists
List *init_list();
void add_to_list(List *list, void *data, int id);
int remove_from_list(List *list, void *data);
int count_items(List *list);
ListNode* find_by_id(List *list, int id);
ListNode* find_by_data(List *list, void *data);
ListNode* pop(List *list);
void print_list(List *list);

#endif // _LINKED_LIST_H_
