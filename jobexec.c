#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cli.h"
#include "jobexec.h"
#include "workers.h"
#include "doc_utils.h"

int actual_num_of_workers;
int to_worker_fd[NUM_OF_WORKERS];
int to_handler_fd[NUM_OF_WORKERS];
struct worker_info workers[NUM_OF_WORKERS];

#define ARRAY_LEN(array) (sizeof((array))/sizeof((array)[0]))       // Get the number of elements within an array

void jobexec(void)
{    

    int  j, k, worker_fp;
    char *fifo_name1, *fifo_name2;

    actual_num_of_workers = NUM_OF_WORKERS;

    set_docfile_lines("docfile");                                   //Get number of docfile lines/number of paths
    struct docfile_line_info docfile_lines[get_docfile_lines()];    //Path table creation
    memset(docfile_lines, 0, ARRAY_LEN(docfile_lines)*sizeof(struct docfile_line_info));
    store_docfile_lines("docfile", docfile_lines);                  //Store paths in docfile_lines table
    if (get_docfile_lines() < NUM_OF_WORKERS)                       //If number of workers to be created are more than needed
    {                                                               //set the arg_w_val appropriately
        actual_num_of_workers = get_docfile_lines();
    }

                                                                    //Count the digits of the workers number f
    fifo_name1 = malloc(16 + 1 + count_digits(actual_num_of_workers)
                        *sizeof(char));                             //or correct memory allocation add one for \0 character
    fifo_name2 = malloc(17 + 1 + count_digits(actual_num_of_workers)
                        *sizeof(char));

    for (j = 0; j < actual_num_of_workers; j++)
    {
        sprintf(fifo_name1, "to_worker_fifo_%d", j);
        if (mkfifo(fifo_name1, 0666) < 0)                           //Create fifo for communication from handler to worker
        {
            perror("Error creating the named pipe");
            exit(EXIT_FAILURE);
        }
        sprintf(fifo_name2, "to_handler_fifo_%d", j);
        if (mkfifo(fifo_name2, 0666) < 0)                           //Create fifo for communication from worker to handler
        {
            perror("Error creating the named pipe");
            exit(EXIT_FAILURE);
        }
        workers[j].fifo_id = j;                                     //Store workers specific id for fifo usage
        workers[j].worker_pid = fork();                             //Store workers specific pid
        if (workers[j].worker_pid < 0)
            perror("fork");
        else if (workers[j].worker_pid == 0)
        {
            sprintf(fifo_name1, "to_worker_fifo_%d", j);
            sprintf(fifo_name2, "to_handler_fifo_%d", j);
            worker_fp = worker_paths(fifo_name1);
            worker_cli(worker_fp, fifo_name2);
            free_docfile_lines(docfile_lines);
            unlink(fifo_name1);
            unlink(fifo_name2);
            free(fifo_name1);
            free(fifo_name2);
            exit(1);
        }
    }
    /***********Stop here until all workers open FiFos for read***********/
    for (j = 0; j < actual_num_of_workers; j++)
    {
        waitpid(workers[j].worker_pid, NULL, WUNTRACED);            //open all handler to worker fifos for writing
        sprintf(fifo_name1, "to_worker_fifo_%d", j);
        sprintf(fifo_name2, "to_handler_fifo_%d", j);
        if ((to_worker_fd[j] = open(fifo_name1, O_WRONLY | O_NONBLOCK)) == -1)  //O_NONBLOCK for debugging reasons
        {
            perror("ERROR: ");
            exit(EXIT_FAILURE);
        }
        if ((to_handler_fd[j] = open(fifo_name2, O_RDONLY | O_NONBLOCK)) == -1) //O_NONBLOCK for debugging reasons
        {
            perror("ERROR: ");
            exit(EXIT_FAILURE);
        }
        k = j;
        while (k < get_docfile_lines())                             //Equally distribute paths to workers
        {
            write(to_worker_fd[j], &(docfile_lines[k].lchars_counter), sizeof(int));
            write(to_worker_fd[j], &(docfile_lines[k].docfile_line), docfile_lines[k].lchars_counter*sizeof(char));

            k = k + actual_num_of_workers;
        }
        kill(workers[j].worker_pid, SIGCONT);                       //Αfter handler wrote to specific workers FiFo,
    }                                                               //send that worker the continue signal to begin reading
    sleep(1);
    free_docfile_lines(docfile_lines);
    free(fifo_name1);
    free(fifo_name2);
    return;


//    free_docfile_lines(docfile_lines);


}
