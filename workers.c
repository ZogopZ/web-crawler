#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "map.h"
#include "jobexec.h"
#include "retrie.h"
#include "workers.h"
#include "doc_utils.h"
#include "posting_list.h"

struct docfile_line_info *line_list_head = NULL;

struct text_info *text_list_head = NULL;

struct answer_list_node *answer_list_head = NULL;

struct answer_list_node *new_answer_node()
{
    struct answer_list_node *answer_node_current = malloc(sizeof(struct answer_list_node));
    if (answer_node_current == NULL)
    {
        printf("Not enough memory in heap\n");
        exit(EXIT_FAILURE);
    }
    answer_node_current->answer_next = NULL;
    return answer_node_current;
}

struct docfile_line_info *new_list_node(int path_length)
{
    struct docfile_line_info *list_node_current = malloc(sizeof(struct docfile_line_info) + path_length*sizeof(char));
    if (list_node_current == NULL)
    {
        printf("Not enough memory in heap\n");
        exit(EXIT_FAILURE);
    }
    list_node_current->lchars_counter = path_length;
    list_node_current->line_list_next = NULL;
    return list_node_current;
}

struct text_info *new_text_node(int path_n_text_length)
{
    struct text_info *list_node_current = malloc(sizeof(struct text_info) + path_n_text_length*sizeof(char));
    if (list_node_current == NULL)
    {
        printf("Not enough memory in heap\n");
        exit(EXIT_FAILURE);
    }
    list_node_current->freq = 0;
    list_node_current->path_n_text_chars_counter = path_n_text_length;
    list_node_current->text_list_next = NULL;
    list_node_current->current_map_node_root = NULL;
    list_node_current->current_trie_node_root= NULL;
    return list_node_current;
}

int worker_paths(char *fifo_name1)
{
    int fp1, path_length;
    struct docfile_line_info *line_list_temp, *line_list_cur;

    if ((fp1 = open(fifo_name1, O_RDONLY | O_NONBLOCK)) == -1)  //NONBLOCK for debugging reasons
    {
        perror("Failed to open FiFo");
        exit(EXIT_FAILURE);
    }
    kill(getpid(), SIGSTOP);                                    //Wait for handler
    path_length = 0;

    while ((read(fp1, &path_length, sizeof(int))) > 0)
    {                                                           //first integer is the number of characters in each path
        line_list_temp = new_list_node(path_length);
        read(fp1, &(line_list_temp->docfile_line), line_list_temp->lchars_counter);
        if (line_list_head == NULL)                             //Create a list of docfiles lines
        {
            line_list_head = line_list_temp;
        }
        else
        {                                                       //Append to the end of the list
            line_list_cur  = line_list_head;
            while (line_list_cur->line_list_next != NULL)
                line_list_cur = line_list_cur->line_list_next;
            line_list_cur->line_list_next = line_list_temp;
        }
    }
    worker_dirs(line_list_head);                                //Worker opens directories
    worker_create_tries(text_list_head);                        //Worker creates tries
    free_line_list(line_list_head);
    return fp1;
}

void worker_dirs(struct docfile_line_info *line_list_head)
{

    struct docfile_line_info *line_list_temp = line_list_head;
    struct text_info *text_list_temp, *text_list_cur;
    struct dirent *dp;
    DIR *dir;
    int path_n_text_length;
    while (line_list_temp != NULL)
    {
        if ((dir = opendir(line_list_temp->docfile_line)) == NULL)
        {
            printf("ERROR: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        while ((dp = readdir(dir)) != NULL)             //Read every file from specific directory
        {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."));
                                                        //Ignore files . and ..
            else
            {                                           //For every text file create a node in text list               
                path_n_text_length = line_list_temp->lchars_counter + strlen(dp->d_name) + 1;
                text_list_temp = new_text_node(path_n_text_length);
                snprintf(text_list_temp->path_n_text_name, path_n_text_length, "%s/%s", line_list_temp->docfile_line, dp->d_name);
                if(text_list_head == NULL)
                {
                    text_list_head = text_list_temp;
                    text_list_head->path_name = line_list_temp->docfile_line;
                    text_list_head->path_chars_counter = line_list_temp->lchars_counter;
                }
                else
                {                                       //Append node to text list
                    text_list_cur  = text_list_head;
                    while (text_list_cur->text_list_next != NULL)
                        text_list_cur = text_list_cur->text_list_next;
                    text_list_cur->text_list_next = text_list_temp;
                    text_list_cur->text_list_next->path_name = line_list_temp->docfile_line;
                    text_list_cur->text_list_next->path_chars_counter = line_list_temp->lchars_counter;
                }
            }
        }
        closedir(dir);
        line_list_temp = line_list_temp->line_list_next;
    }

}

void worker_create_tries(struct text_info *text_list_head)
{
    struct text_info *text_list_temp = text_list_head;
    while (text_list_temp != NULL)
    {
        text_list_temp->current_map_node_root = load_map_list(text_list_temp->path_n_text_name);
        text_list_temp->current_trie_node_root = load_retrie();
        trie_node_head = NULL;
        map_node_head = NULL;
        text_list_temp = text_list_temp->text_list_next;
    }

}

void search_word_n_update_log(struct text_info *text_list_node, struct word *worker_word, time_t query_arrival, FILE *log_fp)
{
    struct trie_node *trie_node_temp;
    struct text_info *text_list_temp = text_list_node;

    fprintf(log_fp, "%.24s : search : %s", ctime(&query_arrival), worker_word->actual_word);        //Print in log file
    while (text_list_temp != NULL)                                              //Search all tries that belong to worker
    {
        trie_node_temp = search_word_to_trie(worker_word, text_list_temp->current_trie_node_root);
        if (trie_node_temp != NULL)                                             //If word was found inside trie
        {
            fprintf(log_fp, " : %s", text_list_temp->path_n_text_name);         //Print in log file
            worker_update_answer_list(trie_node_temp, text_list_temp);          //Create and update answer list
        }
        text_list_temp = text_list_temp->text_list_next;
    }
    fprintf(log_fp, "\n");
}

void worker_update_answer_list(struct trie_node *trie_node_temp, struct text_info *text_list_temp)
{
    struct answer_list_node *answer_list_temp, *answer_list_cur;
    struct post_list_node *post_list_temp;

    post_list_temp = trie_node_temp->post_list_head;

    while (post_list_temp != NULL)
    {
        if (answer_list_head == NULL)
        {                                                                   //Initialize head of answer list
            answer_list_head = new_answer_node();
            answer_list_head->line_id = post_list_temp->line_id;
            answer_list_head->path_chars_counter = text_list_temp->path_chars_counter;
            answer_list_head->path_name = text_list_temp->path_name;
            answer_list_head->answer_map_node = search_map_node_to_list_with_id
                    (answer_list_head->line_id,
                     text_list_temp->current_map_node_root);
        }
        else
        {
            answer_list_temp = answer_list_head;
            while (answer_list_temp != NULL)                                //Repeat for all the answer list
            {
                if (answer_list_temp->line_id == post_list_temp->line_id)   //If the word described from post_list_head belongs in the same line
                    break;                                                  //with a previous word,  DO NOT update answer_list
                else
                    answer_list_temp = answer_list_temp->answer_next;       //Search all answer_list
            }
            if (answer_list_temp == NULL)                                   //If we searched all the answer_list and didn't find a duplicate line
            {                                                               //create node and initialize it
                answer_list_temp = new_answer_node();
                answer_list_temp->line_id = post_list_temp->line_id;
                answer_list_temp->path_chars_counter = text_list_temp->path_chars_counter;
                answer_list_temp->path_name = text_list_temp->path_name;
                answer_list_temp->answer_map_node = search_map_node_to_list_with_id
                        (answer_list_temp->line_id,
                         text_list_temp->current_map_node_root);

                answer_list_cur  = answer_list_head;                        //Append the newly found answer to the end of the answer list
                while (answer_list_cur->answer_next != NULL)
                    answer_list_cur = answer_list_cur->answer_next;
                answer_list_cur->answer_next = answer_list_temp;
            }
        }
        post_list_temp = post_list_temp->post_next;
    }
}

void worker_get_wc_statistics(struct wc_answer_list_node *wc_answer_list_temp, struct text_info *incoming_struct)
{
    struct text_info *text_list_temp = incoming_struct;

    while (text_list_temp != NULL)
    {
        worker_update_wc_statistics(wc_answer_list_temp, text_list_temp->current_map_node_root);
        text_list_temp = text_list_temp->text_list_next;
    }
}

void worker_update_wc_statistics(struct wc_answer_list_node *wc_answer_list_temp, struct map_node *incoming_map_node)
{
    struct map_node *map_node_temp = incoming_map_node;

    while (map_node_temp != NULL)
    {
        wc_answer_list_temp->n_bytes +=map_node_temp->text_length;
        if (map_node_temp->text[0] != '\0')
        {
            wc_answer_list_temp->n_words +=map_node_temp->number_of_words;
        }
        wc_answer_list_temp->n_lines ++;
        map_node_temp = map_node_temp->next;
    }

}

struct text_info *search_max(struct text_info *text_list_head, struct word *worker_word)
{
    struct text_info *text_list_temp, *maxcount_temp;
    struct trie_node *trie_node_temp;
    struct post_list_node *post_list_temp;
    int max_freq, temp_freq;

    text_list_temp = text_list_head;
    max_freq = 0;
    maxcount_temp = NULL;

    while (text_list_temp != NULL)
    {
        temp_freq = 0;
        trie_node_temp = search_word_to_trie(worker_word, text_list_temp->current_trie_node_root);
        if (trie_node_temp != NULL)                             //If word is found inside trie
        {
            post_list_temp = trie_node_temp->post_list_head;
            while (post_list_temp != NULL)
            {
                temp_freq += post_list_temp->freq;              //Count the words frequency inside specific file
                post_list_temp = post_list_temp->post_next;
            }
            if (temp_freq == max_freq)                          //If there is a tie in frequencies between different files
            {
                if ((strcmp(text_list_temp->path_n_text_name, maxcount_temp->path_n_text_name)) < 0)
                {                                               //Keep the one with the alphabetically smaller path
                    maxcount_temp = text_list_temp;
                }
            }
            else if (temp_freq > max_freq)                      //Compare frequencies with previous found word
            {
                max_freq = temp_freq;
                maxcount_temp = text_list_temp;
            }
        }
        text_list_temp = text_list_temp->text_list_next;
    }
    if (maxcount_temp != NULL)
        maxcount_temp->freq = max_freq;
    return maxcount_temp;
}

struct text_info *search_min(struct text_info *text_list_head, struct word *worker_word)
{
    struct text_info *text_list_temp, *mincount_temp;
    struct trie_node *trie_node_temp;
    struct post_list_node *post_list_temp;
    int min_freq, temp_freq;

    text_list_temp = text_list_head;
    min_freq = 1;
    mincount_temp = NULL;

    while (text_list_temp != NULL)
    {
        temp_freq = 0;
        trie_node_temp = search_word_to_trie(worker_word, text_list_temp->current_trie_node_root);
        if (trie_node_temp != NULL)                             //If word is found inside trie
        {
            post_list_temp = trie_node_temp->post_list_head;
            while (post_list_temp != NULL)
            {
                temp_freq += post_list_temp->freq;              //Count the words frequency inside specific file
                post_list_temp = post_list_temp->post_next;
            }
            if (min_freq == 1 || temp_freq < min_freq)          //First time here
            {                                                   //Or compare frequencies with previous found word
                min_freq = temp_freq;
                mincount_temp = text_list_temp;
            }
            else if (temp_freq == min_freq)                     //If there is a tie in frequencies between different files
            {
                if ((strcmp(text_list_temp->path_n_text_name, mincount_temp->path_n_text_name)) < 0)
                {                                               //Keep the one with the alphabetically smaller path
                    mincount_temp = text_list_temp;
                }
            }
        }
        text_list_temp = text_list_temp->text_list_next;
    }
    if (mincount_temp != NULL)
        mincount_temp->freq = min_freq;
    return mincount_temp;
}

void free_line_list(struct docfile_line_info *incoming_struct)
{
    struct docfile_line_info *line_list_temp;

    while (1)
    {
        line_list_temp = incoming_struct;
        if (incoming_struct->line_list_next != NULL)
        {
            incoming_struct = incoming_struct->line_list_next;
            free(line_list_temp);
        }
        else
        {
            free(line_list_temp);
            incoming_struct = NULL;
            return;
        }
    }
}
