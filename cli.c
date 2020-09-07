#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "cli.h"
#include "map.h"
#include "jobexec.h"
#include "retrie.h"
#include "workers.h"
#include "doc_utils.h"
#include "posting_list.h"

#define MAX_ARG_STR     256
#define deadline_str    8
#define not_found_sig   -1
#define search_sig      -2
#define maxcount_sig    -3
#define mincount_sig    -4
#define wc_sig          -5
#define exit_sig        -6
#define LOCKED          1
#define UNLOCKED        0

int timeout = UNLOCKED;

//node for the head of search_words list
struct word *search_words_head = NULL;
struct word *found_words_head = NULL;
struct word *search_wwords_head = NULL;

struct word *get_word_list(void)
{
    return search_words_head;
}

struct word *new_word(int cli_word_length)
{
    //Memory allocated is sizeof(struct) + sizeof(word from cli) + sizeof("\0")
    struct word *cli_word_current = (struct word*) malloc(sizeof(struct word) + cli_word_length*sizeof(char));
    if (cli_word_current == NULL)
    {
        printf("Not enough memory in heap\n");
        exit(EXIT_FAILURE);
    }
    memset(cli_word_current, 0, (sizeof(struct word) + cli_word_length*sizeof(char)));
    cli_word_current->word_next = NULL;
    cli_word_current->word_length = cli_word_length;
    return cli_word_current;
}

void got_alarm()
{
    timeout = LOCKED;
    //puts("^^^TIMEOUT^^^");
    ualarm(0, 0);           //Disable ualarm
}

void command_line_user(char *buffer, int to_worker_fd[], int to_handler_fd[], struct worker_info workers[])
{
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    char *str = malloc(MAX_ARG_STR*sizeof(char));
    memset(str, 0, MAX_ARG_STR);
    char *temp_str = malloc(MAX_ARG_STR*sizeof(char));
    memset(temp_str, 0, MAX_ARG_STR);
    char *d_str = malloc(deadline_str*sizeof(char));
    memset(d_str, 0, deadline_str);
    char *word, *tokenSpace1;

    int cli_word_length, j, fifo_sig;
    double deadline;
//    unsigned long int bytes_all, fifo_bytes, words_all, fifo_words;
//    unsigned int lines_all, fifo_lines;

    struct word *cli_word, *search_words_temp;
//    struct text_info *answer_list_head, *answer_list_temp, *answer_list_cur;
    siginfo_t temp;

    useconds_t sec;

    signal(SIGALRM, got_alarm);

    cli_word_length = 0;
    fifo_sig = 0;

    /***************HANDLER CLI***************/

    memcpy(str, buffer, 100);
    strcpy(temp_str, str);
    tokenSpace1 = strtok(temp_str, " \t\n");    //Get first argument from stdin

    if (strcmp(str, "\n") == 0);                //When users just types enter, cli continues



    /***************SEARCH QUERY***************/

    else if ((strcmp(tokenSpace1, "search") == 0) || (strcmp(tokenSpace1, "SEARCH") == 0))
    {
        fifo_sig = search_sig;
        deadline = 0;
        word = strtok (str, " \t");
        while (word != NULL)  //Read all words from stdin after /search
        {
            word = strtok (NULL, " \t\n");
            if (word == NULL)
            {
                break;
            }
            if (strcmp(word, "-d") == 0)
            {
                d_str = strtok(NULL, " \t\n");
                deadline = atof(d_str);
                sec = deadline*1000000;
                break;
            }
            cli_word_length = strlen(word) + 1;     //+1 for \0
            cli_word = new_word(cli_word_length);
            strcpy(cli_word->actual_word, word);    //Fill word struct
            strcat(cli_word->actual_word, "\0");
            add_word_to_list(cli_word);             //Create a list with given words in cli
        }
        /******Writing to FIFO of each worker the complete search query******/
        for (j = 0;j < actual_num_of_workers; j++)
        {
            write(to_worker_fd[j], &fifo_sig, sizeof(int));
            search_words_temp = search_words_head;
            while (search_words_temp != NULL)
            {
                write(to_worker_fd[j], &(search_words_temp->word_length), sizeof(int));
                write(to_worker_fd[j], &(search_words_temp->actual_word), search_words_temp->word_length);
                search_words_temp = search_words_temp->word_next;
            }
            kill(workers[j].worker_pid, SIGCONT);   //After handler wrote to workers FiFo,
        }                                           //send that worker the continue signal to begin search operation
        if (deadline != 0)
            sec = ualarm(sec, 0);
        free_word_list(search_words_head);          //Free list created by handler for query.
        search_words_head  = NULL;

        /***********Stop here until all workers complete their search queries***********/
        for(j = 0; j < actual_num_of_workers; j++)
        {
            waitpid(workers[j].worker_pid, NULL, WUNTRACED);
            kill(workers[j].worker_pid, SIGCONT);
        }

        /******Reading from FIFO of each worker the results of the search query******/
        for (j = 0;j < actual_num_of_workers; j++)
        {
            while (read(to_handler_fd[j], &fifo_sig, sizeof(int)) > 0)
            {
                if (fifo_sig == not_found_sig)
                {
                    printf("Not found in %d Child\n", j);
                    fflush(stdout);
                    break;
                }
                if (timeout == UNLOCKED)                //If timeout didn't happen read from Fifos and
                {                                       //print to STDOUT
                    printf("Line %d", fifo_sig);
                    read(to_handler_fd[j], &fifo_sig, sizeof(int));
                    char answer_path[fifo_sig];
                    memset(answer_path, 0, fifo_sig);
                    read(to_handler_fd[j], &answer_path, fifo_sig);
                    printf(":In directory:%s\n", answer_path);
                    read(to_handler_fd[j], &fifo_sig, sizeof(int));
                    char answer_text[fifo_sig];
                    memset(answer_text, 0, fifo_sig);
                    read(to_handler_fd[j], &answer_text, fifo_sig);
                    printf("%s\n\n", answer_text);
                }
                else                                    //If timeout did happen, read from Fifos
                {                                       //to flush them but don't print to STDOUT
                    read(to_handler_fd[j], &fifo_sig, sizeof(int));
                    char answer_path[fifo_sig];
                    memset(answer_path, 0, fifo_sig);
                    read(to_handler_fd[j], &answer_path, fifo_sig);
                    read(to_handler_fd[j], &fifo_sig, sizeof(int));
                    char answer_text[fifo_sig];
                    memset(answer_text, 0, fifo_sig);
                    read(to_handler_fd[j], &answer_text, fifo_sig);
                }
            }
        }
        if(timeout == LOCKED)
        {
            printf("TIMEOUT\n");
            ualarm(0, 0);
            timeout = UNLOCKED;                         //reset timeout to default
        }
    }
    /***************EXIT QUERY***************/

    else if (strcmp(tokenSpace1, "/exit") == 0 || strcmp(tokenSpace1, "exit") == 0)
    {
        fifo_sig = exit_sig;
        for (j = 0;j < actual_num_of_workers; j++)
        {
            write(to_worker_fd[j], &fifo_sig, sizeof(int));         //Send exit signal to specific worker
            kill(workers[j].worker_pid, SIGCONT);                   //Send SIGCONT to specific worker to exit
        }

        for (j = 0;j < actual_num_of_workers; j++)
        {
            waitid(P_PID, workers[j].worker_pid, &temp, WEXITED);    //Wait for specific worker to exit
            close(to_worker_fd[j]);                                 //Close FiFo
            close(to_handler_fd[j]);                                //Close FiFo
        }
        puts("jobExecutor is now Exiting");
    }
    else
    {
        printf("Wrong Arguments\n");                                //Wrong Arguments
    }
    printf("##########################################################################\n");
    free(str);
    free(temp_str);
    free(d_str);

}


/***************WORKER CLI***************/

void worker_cli(int worker_fp, char *fifo_name2)
{
    char *log_file;
    int fifo_sig, handler_fp;
    struct word *worker_word;
    struct answer_list_node *answer_list_temp;

    FILE *log_fp;
    time_t query_arrival;

    fifo_sig = 0;
    if ((handler_fp = open(fifo_name2, O_WRONLY | O_NONBLOCK)) == -1)            //NONBLOCK for debugging reasons
    {
        perror("ERROR: ");
        exit(EXIT_FAILURE);
    }

    log_file = malloc((11 + count_digits(getpid()) + 1)
                      *sizeof(char));
    sprintf(log_file, "%s/Worker_%d",log, getpid());
    if ((log_fp = fopen(log_file, "w+")) == NULL)
    {
        printf("can not open file \n");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        kill(getpid(), SIGSTOP);
        read(worker_fp, &fifo_sig, sizeof(int));    //First read from worker is one of the defined exit_sig, search_sig, wc_sig
        time(&query_arrival);                       //Get the time of query arrival


/***************SEARCH QUERY***************/

        if (fifo_sig == search_sig)
        {
            while (read(worker_fp, &fifo_sig, sizeof(int)) > 0)
            {                                                   //Read words from handler one by one
                worker_word = new_word(fifo_sig);               //and search them in tries
                read(worker_fp, &(worker_word->actual_word), worker_word->word_length);
                search_word_n_update_log(text_list_head, worker_word, query_arrival, log_fp);
                free(worker_word);
            }
            answer_list_temp = answer_list_head;
            /******Writing to handlers FIFO the results of the search query******/
            while (answer_list_temp != NULL)
            {
                write(handler_fp, &(answer_list_temp->line_id), sizeof(int));
                write(handler_fp, &(answer_list_temp->path_chars_counter), sizeof(int));
                write(handler_fp, (answer_list_temp->path_name), answer_list_temp->path_chars_counter);
                write(handler_fp, &(answer_list_temp->answer_map_node->text_length), sizeof(int));
                write(handler_fp, &(answer_list_temp->answer_map_node->text), answer_list_temp->answer_map_node->text_length);
                answer_list_temp = answer_list_temp->answer_next;
            }
            if (answer_list_head == NULL)                               //Nothing Found
            {
                fifo_sig = not_found_sig;
                write(handler_fp, &fifo_sig, sizeof(int));
            }
            if (answer_list_head != NULL)
            {
                free_answer_list(answer_list_head);                     //Free list of query results.
                answer_list_head = NULL;
            }
        }
/**************EXIT QUERY**************/

        else if (fifo_sig == exit_sig)
        {
            fprintf(log_fp, "%.24s : exit\n", ctime(&query_arrival));        /***Update log file***/
            free_text_list(text_list_head);
            free(log_file);
            fclose(log_fp);
            close(worker_fp);               //Close FiFo
            close(handler_fp);              //Close FiFo
            break;
            //exit(EXIT_SUCCESS);
        }
    }
}

void free_all(void)
{
    printf("Freeing allocated memory\n");
}

void add_word_to_list(struct word* cli_word)
{
    struct word *search_words_cur = search_words_head;

    if (search_words_head == NULL)
    {
        search_words_head = cli_word;
        return;
    }
    else
    {
        while(search_words_cur->word_next != NULL)
            search_words_cur = search_words_cur->word_next;
        search_words_cur->word_next = cli_word;
        search_words_head->words_found++;
    }
}

struct word *free_word_list(struct word *incoming_struct)
{
    struct word* search_words_tmp;

    while (1)
    {
        search_words_tmp = incoming_struct;
        if (incoming_struct->word_next != NULL)
        {
            incoming_struct = incoming_struct->word_next;
            free(search_words_tmp);
        }
        else
        {
            free(search_words_tmp);
            incoming_struct = NULL;
            return incoming_struct;
        }
    }
}

void free_answer_list(struct answer_list_node *incoming_struct)
{
    struct answer_list_node *answer_list_temp;

    while (1)
    {
        answer_list_temp = incoming_struct;
        if (incoming_struct->answer_next != NULL)
        {
            incoming_struct = incoming_struct->answer_next;
            free(answer_list_temp);
        }
        else
        {
            free(answer_list_temp);
            incoming_struct = NULL;
            return;
        }
    }
}

void free_text_list(struct text_info *incoming_struct)
{
    struct text_info *text_list_temp;

    if (incoming_struct != NULL)
    {
        while (1)
        {
            text_list_temp = incoming_struct;
            if (incoming_struct->text_list_next != NULL)
            {
                if (incoming_struct->current_trie_node_root != NULL)
                    free_trie(incoming_struct->current_trie_node_root);
                if (incoming_struct->current_map_node_root != NULL)
                    free_map_list(incoming_struct->current_map_node_root);
                incoming_struct = incoming_struct->text_list_next;
                free(text_list_temp);
            }
            else
            {
                if (text_list_temp->current_trie_node_root != NULL)
                    free_trie(text_list_temp->current_trie_node_root);
                if (text_list_temp->current_map_node_root != NULL)
                    free_map_list(text_list_temp->current_map_node_root);
                free(text_list_temp);
                incoming_struct = NULL;
                return;
            }
        }
    }
}


void free_docfile_lines(struct docfile_line_info *array_lines)
{
    for (int i = 0; i < get_docfile_lines(); i++)
    {
        free(array_lines[i].docfile_line);
    }
}

void handler_search_max(struct text_info *answer_list_head)
{
    struct text_info *answer_list_temp, *answer_list_cur;
    answer_list_temp = answer_list_cur = answer_list_head;

    while (answer_list_temp != NULL)                                //Search the list created by answers from workers
    {
        if (answer_list_temp->freq > answer_list_cur->freq)         //If found frequency is greater than a previous one, keep it
            answer_list_cur = answer_list_temp;
        else if (answer_list_temp->freq == answer_list_cur->freq)
        {                                                           //If there is a tie between answers.
            if ((strcmp(answer_list_temp->path_n_text_name, answer_list_cur->path_n_text_name)) < 0)
                answer_list_cur = answer_list_temp;                 //Winner is tha alphabetically smaller path
        }
        answer_list_temp = answer_list_temp->text_list_next;
    }
    printf("Winner Path: %s with frequency (%d)\n", answer_list_cur->path_n_text_name, answer_list_cur->freq);
}

void handler_search_min(struct text_info *answer_list_head)
{
    struct text_info *answer_list_temp, *answer_list_cur;
    answer_list_temp = answer_list_cur = answer_list_head;

    while (answer_list_temp != NULL)                                //Search the list created by answers from workers
    {
        if (answer_list_temp->freq < answer_list_cur->freq)         //If found frequency is less than a previous one, keep it
            answer_list_cur = answer_list_temp;
        else if (answer_list_temp->freq == answer_list_cur->freq)
        {                                                           //If there is a tie between answers.
            if ((strcmp(answer_list_temp->path_n_text_name, answer_list_cur->path_n_text_name)) < 0)
                answer_list_cur = answer_list_temp;                 //Winner is tha alphabetically smaller path
        }
        answer_list_temp = answer_list_temp->text_list_next;
    }
    printf("Winner Path: %s with frequency (%d)\n", answer_list_cur->path_n_text_name, answer_list_cur->freq);
}
