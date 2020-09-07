#ifndef _TOOLS_H_
#define _TOOLS_H_

#include "list.h"

#define MAXSIZE 1000

void createNsend_GET(list_node_t*, int);

void edit_response(list_node_t*, int);

void find_links(char*, long int);

void checkNenlist(char*, int);

long int find_size(char*);

int initialize_server(int);

void shutdown_crawler(void);

int handle_new_connection(void);

int handle_new_comm_connection(int);

int read_command(int, time_t);

void build_select_list(void);

int max(int, int);

void* mallocNcheck(size_t);

void setnonblocking(int);

#endif /* _TOOLS_H_ */
