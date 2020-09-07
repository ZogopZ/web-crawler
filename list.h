#ifndef _LIST_H_
#define _LIST_H_

#define CHECKED 100
#define NOT_CHECKED 101
#define ENQUEUE_SUCCESS 1
#define ENQUEUE_NO_SUCCESS 0

typedef struct list
{
    struct list *next;
    int status;
    int link_length;
    char link[0];
}list_node_t;

list_node_t* new_list_crawlnode(int link_length);

int enlist(list_node_t*);

list_node_t* search_list(void);

void free_list(void);

#endif /* _LIST_H_ */
