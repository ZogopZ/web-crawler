#ifndef _CLI_H_
#define _CLI_H_

#include "map.h"
#include "jobexec.h"
#include "workers.h"

struct count_answer
{
    int freq;
    int path_length;
    char path[0];
};

struct word *get_word_list(void);

void command_line_user(char *buffer, int to_worker_fd[], int to_handler_fd[], struct worker_info workers[]);

void worker_cli(int worker_fp, char *fifo_name2);

void free_all(void);

void add_word_to_list(struct word* cli_word);

struct word *free_word_list(struct word *incoming_struct);

void free_answer_list(struct answer_list_node *incoming_struct);

void free_text_list(struct text_info *incoming_struct);

void free_docfile_lines(struct docfile_line_info *array_lines);

void handler_search_max(struct text_info *answer_list_head);

void handler_search_min(struct text_info *answer_list_head);

#endif /* _CLI_H_ */
