#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "list.h"
#include "tools.h"
#include "webcrawler.h"
#include "cmdline_utils.h"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_exit = PTHREAD_COND_INITIALIZER;

long int total_bytes_served;
int pages_served;

int fd_comm;
int new_comm = 0;
int max_fd;

fd_set set;
int global_counter = 0;
int exit_counter = 0;

void* crawler_thread()
{
    int thread_fd;
    list_node_t *link_node;

    while (1)
    {
        pthread_mutex_lock(&mutex);
        if (global_counter == 0)                                //If there is no url in the list,
        {                                                       //unlock the mutex and continue
            pthread_mutex_unlock(&mutex);                       //until crawler enlists it.
            continue;
        }
        else if (global_counter >= get_num_of_threads())        //All threads found the list empty
            break;                                              //and will exit.
        global_counter++;
        thread_fd = handle_new_connection();                    //Thread connects to server.
        link_node = search_list();                              //Search for links in list.
        if (link_node == NULL)
        {                                                       //Threads will not decrease the global
            close(thread_fd);                                   //counter here.
            pthread_mutex_unlock(&mutex);                       //If all threads found the list empty,
            continue;                                           //they will exit.
        }
        createNsend_GET(link_node, thread_fd);
        pages_served++;
        edit_response(link_node, thread_fd);
        global_counter--;
        pthread_mutex_unlock(&mutex);
    }
    exit_counter++;
    pthread_mutex_unlock(&mutex);
    printf("-THREAD-: I am thread %ld and "
           "I will now exit. BYE...\n", pthread_self());
    pthread_cond_signal(&cond_exit);                            //Signal the crawler that thread is exiting.

    return NULL;
}

void webcrawler(void)
{
    time_t start_time;
    int i, error, read_sockets;
    list_node_t *first_url;
    pthread_t thread_id[get_num_of_threads()];

    time(&start_time);
    char *command1 = "du -b --max-depth=0 save_dir | cut -f1";
    fd_comm = initialize_server(get_command_port());
    max_fd = fd_comm;                                           //Get the max value for select usage.
    setnonblocking(fd_comm);                                    //Set fd_comm O_NONBLOCK.
    for (i = 0; i < get_num_of_threads(); i++)                  //Thread Creation.
    {
        error = pthread_create(&(thread_id[i]), 0, &crawler_thread, NULL);
        if (error != 0)
            printf("\ncan't create thread :[%s]", strerror(error));
    }

    first_url = new_list_crawlnode(strlen(get_starting_URL()) + 1); //+1 for terminating character.
    sprintf(first_url->link, "%s", get_starting_URL());
    pthread_mutex_lock(&mutex);
    enlist(first_url);                                          //Insert starting_URL to list.
    global_counter++;                                           //Threads will continue execution.
    while (exit_counter < get_num_of_threads())
        pthread_cond_wait(&cond_exit, &mutex);                  //Release mutex and wait for condition.
    pthread_mutex_unlock(&mutex);
    printf("\n\n");
    for (i = 0; i < get_num_of_threads(); i++)
    {
        printf("~CRAWLER~: Thread %ld will now join...\n", thread_id[i]);
        pthread_join(thread_id[i], NULL);
    }
    printf("~CRAWLER~: Freeing list...\n");
    free_list();
    printf("~CRAWLER~: Destroying mutexes and condition variables...\n");
    pthread_cond_destroy(&cond_exit);
    pthread_mutex_destroy(&mutex);
    printf("~CRAWLER~: Size of save_dir:");
    fflush(stdout);
    system(command1);                                           //Print save_dir size in bytes.
    fflush(stdout);
    fclose(fp_docfile);
    free(get_host());
    free(get_save_dir());
    free(get_starting_URL());
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
    while (1)
    {
        build_select_list();
        read_sockets = select (max_fd + 1, &set, NULL, NULL, NULL);
        if (read_sockets < 0)
        {
            perror ("select");
            exit (EXIT_FAILURE);
        }
        else
        {
            if (FD_ISSET(fd_comm, &set))               //Connection request on command socket.
                new_comm = handle_new_comm_connection(fd_comm);
            else if (FD_ISSET(new_comm, &set))
            {
                if ((read_command(new_comm, start_time)) == SHUTDOWN)
                {
                    shutdown_crawler();
                    return;
                }
            }
        }
    }
}

