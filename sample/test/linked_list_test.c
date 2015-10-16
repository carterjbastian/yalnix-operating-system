#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) { 
  struct list *list = malloc ( sizeof(struct list) ); 
  int data1 = 5;
  int data2 = 6;
  int data3 = 7;
  int *pt1 = &data1;
  int *pt2 = &data2;
  int *pt3 = &data3;
  
  add_data(list, pt1, 1);
  add_data(list, pt2, 2);
  add_data(list, pt3, 3);

  printf("should be 5, 6, 7:\n");
  print_list(list);

  struct node *n = find_by_id(list, 1);
  int *a = (int *)(n->data);
  printf("should be 5: %d\n", *a);

  struct node *n2 = find_by_data(list, pt2);
  int *b = (int *)(n2->data);
  printf("should be 6: %d\n", *b);

  remove_data(list, pt2);

  printf("should be 5, 7:\n");
  print_list(list);
  
  remove_data(list, pt3);

  printf("should be 5:\n");
  print_list(list);

  add_data(list, pt2, 2);
  add_data(list, pt3, 3);
  
  struct node *n3 = pop(list);
  printf("should be 1: %d\n", n3->id);
  struct node *n4 = pop(list);
  printf("should be 2: %d\n", n4->id);
  struct node *n5 = pop(list);
  printf("should be 3: %d\n", n5->id);
} 
