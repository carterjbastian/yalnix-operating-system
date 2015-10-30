#include <stdlib.h> 
#include <stdio.h>
#include "linked_list.h"

  // add data at end of last node 
void add_to_list(List *list, void *data, int id) { 
  ListNode *new = malloc( sizeof(ListNode) );
  new->data = data;
  new->id = id;
  new->prev = NULL;
  new->next = NULL;

  ListNode *node = list->first;
  if (!node) { 
    list->first = new;
    return;
  } 

  while (node->next) { 
    node = node->next;
  } 
  
  node->next = new;
  new->prev = node;
} 

// remove data and reconnect the effected portions of 
// our linked list 
int remove_from_list(List *list, void * data) { 
  ListNode *node = list->first;

  while(node && node->data != data) { 
    node = node->next;
  }

  if (!node) return -1;

 
  if (node->prev) { // This is not the first item in the list
    node->prev->next = node->next;

    if (node->next) { // Not the last item in the list
        node->next->prev = node->prev;
    }

  } else { // This is the first item in the list

    if (node->next) {  // This is not the last/only item in the list
        node->next->prev = node->prev;
        list->first = node->next;
    } else { // This is the only item in the list
        list->first = NULL;
    }
  } 

  free(node);
  return 0;
} 

ListNode* pop(List *list) { 
  ListNode *node = list->first;
  if (!node) return NULL;
  
  list->first = node->next;
  if (node->next) { 
    node->next->prev = NULL;
  } 

  return node;
} 

int count_items(List *list) { 
  int count = 0;

  ListNode *node = list->first;

  while (node) { 
    count++;
    node = node->next;
  } 

  return count;
} 


ListNode* find_by_id(List *list, int id) { 
  ListNode *node = list->first;
  while(node) {
    if (node->id == id) { 
      return node;
    } 
    node = node->next;
  } 
  return NULL;
} 

ListNode* find_by_data(List *list, void * data) { 
  ListNode *node = list->first;
  while(node) { 
    if (node->data == data) { 
      return node;
    } 
    node = node->next;
  } 
  return NULL;
} 

// currently only works if data is int
// just put this in for testing purposes
void print_list(List *list) { 
  ListNode *node = list->first;
  while(node) { 
    int *data = node->data;
    printf("%d\n", *data);
    node = node->next;
  } 
} 
