#ifndef _WORKERS_H_
#define _WORKERS_H_

#include "map.h"
#include "retrie.h"
#include "jobexec.h"

extern struct text_info *text_list_head;
extern struct answer_list_node *answer_list_head;

struct answer_list_node
{
    int line_id;
    int path_chars_counter;
    char *path_name;
    struct map_node *answer_map_node;
    struct answer_list_node *answer_next;
};

struct text_info
{
    struct text_info *text_list_next;
    struct map_node *current_map_node_root;
    struct trie_node *current_trie_node_root;
    char *path_name;
    int freq;
    int path_chars_counter;
    int path_n_text_chars_counter;
    char path_n_text_name[0];
};

struct wc_answer_list_node
{
    unsigned long int n_bytes;
    unsigned long int n_words;
    unsigned int n_lines;
};

struct answer_list_node *new_answer_node(void);

struct docfile_line_info *new_list_node(int path_length);

struct text_info *new_text_node(int path_n_text_length);

int worker_paths(char *fifo_name1);

void worker_dirs(struct docfile_line_info *line_list_head);

void worker_create_tries(struct text_info *text_list_head);

void search_word_n_update_log(struct text_info *text_list_node, struct word *worker_word, time_t query_arrival, FILE *log_fp);

void worker_update_answer_list(struct trie_node *trie_node_temp, struct text_info *text_list_temp);

void worker_get_wc_statistics(struct wc_answer_list_node *wc_answer_list_temp, struct text_info *incoming_struct);

void worker_update_wc_statistics(struct wc_answer_list_node *wc_answer_list_temp, struct map_node *incoming_map_node);

struct text_info *search_max(struct text_info *text_list_head, struct word *worker_word);

struct text_info *search_min(struct text_info *text_list_head, struct word *worker_word);

void free_line_list(struct docfile_line_info *incoming_struct);

#endif /* _WORKERS_H_ */
