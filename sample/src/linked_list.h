// linked_list.h

// Node data structure for linked lists
struct node { 
  int id;
  void *data; 
  struct node *next; 
  struct node *prev;
};

// List data structure
struct list { 
  void *first;
};

// Functions related to the linked lists
void add_data(struct list *list, void * data, int id);
int remove_data(struct list *list, void * data);
int count(struct list *list);
struct node* find_by_id(struct list *list, int id);
struct node* find_by_data(struct list *list, void * data);
struct node* pop(struct list *list);
void print_list(struct list *list);
