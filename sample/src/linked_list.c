#include <stdlib.h> 
#include <stdio.h>
#include "linked_list.h"

  // add data at end of last node 
void add_data(struct list *list, void * data, int id) { 
  struct node *new = malloc( sizeof(struct node) );
  new->data = data;
  new->id = id;
    
  struct node *node = list->first;
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
int remove_data(struct list *list, void * data) { 
  struct node *node = list->first;
  while(node && node->data != data) { 
    node = node->next;
  }

  if (!node) return -1;
  
  node->prev->next = node->next;
  if (node->next) { 
    node->next->prev = node->prev;
  } 

  free(node);

  return 0;
} 

struct node* pop(struct list *list) { 
  struct node *node = list->first;
  if (!node) return NULL;
  
  list->first = node->next;
  if (node->next) { 
    node->next->prev = NULL;
  } 

  return node;
} 

int count(struct list *list) { 
  int count = 0;
  struct node *node = list->first;
  while (node) { 
    count++;
    node = node->next;
  } 

  return count;
} 


struct node* find_by_id(struct list *list, int id) { 
  struct node *node = list->first;
  while(node) { 
    if (node->id == id) { 
      return node;
    } 
    node = node->next;
  } 
  return NULL;
} 

struct node* find_by_data(struct list *list, void * data) { 
  struct node *node = list->first;
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
void print_list(struct list *list) { 
  struct node *node = list->first;
  while(node) { 
    int *data = node->data;
    printf("%d\n", *data);
    node = node->next;
  } 
} 
