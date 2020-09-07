#ifndef _JOBEXEC_H_
#define _JOBEXEC_H_

#include <unistd.h>

#define log "log"
#define NUM_OF_WORKERS 3

extern int actual_num_of_workers;
extern int to_worker_fd[NUM_OF_WORKERS];
extern int to_handler_fd[NUM_OF_WORKERS];


struct worker_info
{
    pid_t worker_pid;
    int fifo_id;
};

extern struct worker_info workers[NUM_OF_WORKERS];

struct docfile_line_info
{
    struct docfile_line_info *line_list_next;
    int lchars_counter;
    char *docfile_line;
};

void jobexec(void);

#endif /* _JOBEXEC_H_ */

