#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "cli.h"
#include "list.h"
#include "tools.h"
#include "jobexec.h"
#include "webcrawler.h"
#include "cmdline_utils.h"


void createNsend_GET(list_node_t* link_node, int thread_fd)
{
    char* GET_request;
    int request_size;

    request_size = link_node->link_length + strlen(get_host()) + 139;
    GET_request = malloc(request_size);
    sprintf(GET_request, "GET %s HTTP/1.1\r\n"
                          "User-Agent: webcrawler (Syspro3)\r\n"
                          "Host: %s\r\n"
                          "Accept-Language: en-us\r\n"
                          "Accept-Encoding: gzip, deflate\r\n"
                          "Connection: Keep-Alive\r\n\r\n", link_node->link, get_host());
    write(thread_fd, GET_request, request_size);
    free(GET_request);
}

void edit_response(list_node_t* link_node, int thread_fd)
{
    int path_size, i, j, new_file;
    long int file_size, bytes_read, byte_checker;
    char message[158];
    char working_dir[1024];
    char *path, *dir, *buffer;
    struct stat *dir_info;


    memset(message, 0, 158);
    path_size = strlen(get_save_dir()) + link_node->link_length;
    path = mallocNcheck(path_size);
    dir_info = mallocNcheck(sizeof(struct stat));



    for (i = 0; i < link_node->link_length; i++)
    {
        if (link_node->link[i] == '/' && i == 0)
            continue;
        else if (link_node->link[i] == '/' && i != 0)
        {
            j = i;                                              //Get the size of directory.
            break;                                              //example: /site0/ size=7
        }
    }
    dir = mallocNcheck(strlen(get_save_dir()) + j+1 + 1);       //+1 for terminating character.
    memcpy(dir, get_save_dir(), strlen(get_save_dir()) + 1);    //strlen does not count terminating character.
    memcpy(&dir[strlen(dir)], link_node->link, j + 1);          //strlen does not count terminating character.
    dir[strlen(get_save_dir()) + j + 1] = 0;                    //Set the last character of dir to terminating.

    if (stat(dir, dir_info) < 0)
    {
        memset(working_dir, 0 , 1024);
        mkdir(dir, 0700);
        getcwd(working_dir, 1024);
        working_dir[strlen(working_dir)] = '/';
        working_dir[strlen(working_dir)] = 0;
        memcpy(&working_dir[strlen(working_dir)], dir, strlen(dir) - 1);
        working_dir[strlen(working_dir)] = '\n';                //Write all different paths to docfile for jobExecutor
        fwrite(working_dir, 1, strlen(working_dir), fp_docfile);
    }
    snprintf(path, path_size, "%s%s", get_save_dir(), link_node->link);
    if((new_file = open(path, O_RDWR|O_CREAT, 0666)) < 0)
    {
        printf("can not open file \n");
        exit(EXIT_FAILURE);
    }
    read(thread_fd, message, 158);
    file_size = find_size(message);                             //Find size of file in message.
    buffer = malloc(file_size);
    memset(buffer, 0, file_size);
    byte_checker = 0;
    while (((bytes_read = read(thread_fd, buffer, file_size)) > 0)
           && (byte_checker <= file_size))                      //Read whole file size.
    {
        total_bytes_served = total_bytes_served + bytes_read;
        byte_checker = byte_checker + bytes_read;
        write(new_file, buffer, bytes_read);
        if (byte_checker == file_size)                          //Check if whole size was read.
            break;
        else if (byte_checker < file_size)                      //Read was incomplete... Read again.
        {
            printf("\t-THREAD-:%ld Warning incomplete file write. Missing %ld bytes. "
                   "Attempting to read again\n", pthread_self(), file_size - byte_checker);
            file_size = file_size - byte_checker;
            byte_checker = 0;
        }
    }
    close(new_file);
    printf("\tCreated file:%s and will now search for links\n", link_node->link);
    find_links(buffer, file_size);


    free(dir);
    free(path);
    free(buffer);
    free(dir_info);
}

void find_links(char* buffer, long int file_size)
{
    long int i, j, chars;
    int flag;
    char* found_link;

    flag = 0;

    for (i = 0; i < file_size; i++)
    {
        if (buffer[i] == '\'' && buffer[i-1] == '='
                              && buffer[i-7] == 'a'
                              && buffer[i-8] == '<')
        {
            chars = 0;
            j = i;
            flag = 1;
            continue;
        }
        else if (buffer[i] == '\'' && buffer[i-1] == 'l'
                                   && buffer[i-2] == 'm'
                                   && buffer[i-3] == 't'
                                   && buffer[i-4] == 'h')
        {
            found_link = mallocNcheck(chars + 1);   //+1 one for terminating character
            memset(found_link, 0, chars + 1);
            memcpy(found_link, &buffer[j + 1], chars);
            found_link[chars] = 0 ;                 //Set the last character of found_link to terminating
            checkNenlist(found_link, chars + 1);
            flag = 0;
            chars = 0;
            free(found_link);
            continue;
        }
        if (flag == 1)
        {
            chars++;
        }
    }
}

void checkNenlist(char* found_link, int link_length)
{
    int status;
    char* file_checker;
    list_node_t *new_url;
    struct stat *found_link_info;

    status = -1;
    file_checker = mallocNcheck(strlen(get_save_dir()) + link_length);  //link_length includes terminating character
    found_link_info = mallocNcheck(sizeof(struct stat));

    sprintf(file_checker, "%s%s", get_save_dir(),found_link);
    if (stat(file_checker, found_link_info) < 0)
    {
        printf("\tCould not stat link:%s\t...Trying to enlist", found_link);
        new_url = new_list_crawlnode(link_length);
        strncpy(new_url->link, found_link, link_length);
        status = enlist(new_url);
        if (status == ENQUEUE_NO_SUCCESS)
        {
            printf("  !Warning: Link already exists in list");
            printf("...Freeing allocated node\n");
            free(new_url);
        }
        else if (status == ENQUEUE_SUCCESS)
            printf("...Successfully enlisted\n");
    }

    free(found_link_info);
    free(file_checker);

    return;
}

long int find_size(char* message)
{
    int size, i;
    long int file_size;
    char* file_size_string;

    size = 0;
    i = 104;
    file_size = 0;

    while(message[i] >= '0' && message[i] <= '9')
    {
        i++;
        size++;
    }
    file_size_string = mallocNcheck(size + 1);      //+1 for null terminating character
    i = 104;
    memcpy(file_size_string, &message[i], size);
    file_size_string[size] = 0;                     //Set the last character as terminating
    file_size = (long int) atoi(file_size_string);
    free(file_size_string);
    return file_size;
}

int initialize_server(int port)
{
    struct sockaddr_in server_addr;
    int fd_temp;

    int on = 1;

    fd_temp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_temp < 0)
    {
        perror("socket");
        exit(1);
    }
    setsockopt(fd_temp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));  //Set reusability of fd_temp
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(fd_temp,(struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(fd_temp);
        exit(1);
    }
    if (listen(fd_temp, 10) == -1)
    {
        perror("listen");
        close(fd_temp);
        exit(1);
    }
    return fd_temp;
}

void shutdown_crawler(void)
{
    printf("~CRAWLER~: Crawler will now shutdown...\n");
    close(fd_comm);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
    return;
}

int handle_new_connection(void)
{
    int thread_fd, n;
    struct sockaddr_in serv_addr;

    thread_fd = n = 0;

    if ((thread_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        exit(EXIT_FAILURE);
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(get_serving_port());
    if (inet_pton(AF_INET, get_host(), &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        exit(EXIT_FAILURE);
    }
    if (connect(thread_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       exit(EXIT_FAILURE);
    }
    return thread_fd;
}

int handle_new_comm_connection(int socket)
{
    int new_connection;

    new_connection = accept(socket, NULL, NULL);
    if (new_connection < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    max_fd = max(max_fd, new_connection);           //Get the max value for select usage.
    setnonblocking(new_connection);                 //Set new_connection O_NONBLOCK.
    return new_connection;
}

int read_command(int new_comm, time_t start_time)
{
    int hours, minutes, seconds;
    char message[100];
    char search_message[7];
    char exit_message[6] = "/exit";
    time_t current_time, diff;

    static int flag = 0;
    memset(message, 0 , 100);

    read(new_comm, &message, 100);
    if ((strcmp(message, "SHUTDOWN\n") == 0) || (strcmp(message, "shutdown\n") == 0))
    {
        command_line_user(exit_message, to_worker_fd, to_handler_fd, workers);      //Send exit message to all workers and handler.
        write(new_comm, "Message from Server: Server will now shut down...\n", 51);
        close(new_comm);
        return SHUTDOWN;
    }
    else if ((strcmp(message, "STATS\n") == 0) || (strcmp(message, "stats\n") == 0))
    {
        printf("############################################################\n");
        printf("#          Connection accepted:   FD: %2d                   #\n",
            new_comm);
        printf("############################################################\n");
        time(&current_time);
        diff = current_time - start_time;
        hours = diff/3600; minutes = (diff-hours*3600)/60; seconds = (diff-hours*3600-minutes*60);
        printf("Server up for %.2d:%.2d.%.2d, downloaded pages:%d, total bytes:%ld\n", hours, minutes, seconds, pages_served, total_bytes_served);
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
    }
    else
    {
        memcpy(search_message, message, 7);
        search_message[7] = 0;
        if ((strcmp(search_message, "SEARCH ") == 0) || (strcmp(search_message, "search ") == 0))
        {
            if (flag == 0)
            {
                jobexec();
                flag = 1;
            }
            command_line_user(message, to_worker_fd, to_handler_fd, workers);
        }
    }

    return 0;
}

void build_select_list(void)
{

    FD_ZERO(&set);
    FD_SET(fd_comm, &set);
    if (new_comm != 0)
        FD_SET(new_comm, &set);
    return;
}

int max(int fd_1, int fd_2)
{
    int max_fd = 0;

    if (fd_1 > fd_2)
        max_fd = fd_1;
    else if (fd_1 < fd_2)
        max_fd = fd_2;
    else
    {
        max_fd = fd_1;
    }
    return max_fd;
}

void* mallocNcheck(size_t incoming_size)
{
    void *ptr;

    ptr = malloc(incoming_size);
    if (ptr == NULL && incoming_size > 0)
    {
        printf("Not enough memory in heap\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void setnonblocking(int sock)
{
    int opts;

    opts = fcntl(sock,F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(sock,F_SETFL,opts) < 0) {
        perror("fcntl(F_SETFL)");
        exit(EXIT_FAILURE);
    }
    return;
}
