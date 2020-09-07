#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "tools.h"
#include "cmdline_utils.h"


int serving_port;
int serving_port_length;
int command_port;
int num_of_threads;
int arg_host;
int arg_save_dir;
int arg_SURL;
char* host;
char* save_dir;
char* starting_URL;
FILE *fp_docfile;

int parse_cli_args(int argc, char** argv)
{

    if (argc < 12)
    {
        printf("Error, too few arguments ...\n");
        return 1;
    }
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            serving_port = atoi(argv[i+1]);
            serving_port_length = strlen(argv[i+1]);
        }
        else if (strcmp(argv[i], "-c") == 0)
            command_port = atoi(argv[i+1]);
        else if (strcmp(argv[i], "-t") == 0)
            num_of_threads = atoi(argv[i+1]);
        else if (strcmp(argv[i], "-h") == 0)
        {
            arg_host = i + 1;
            host = mallocNcheck(strlen(argv[arg_host]) + 1);
            strcpy(host, argv[arg_host]);
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            arg_save_dir = i + 1;
            save_dir = mallocNcheck(strlen(argv[arg_save_dir]) + 1);
            strcpy(save_dir, argv[arg_save_dir]);
            mkdir(save_dir, 0700);
            if ((fp_docfile = fopen("docfile", "w+")) == NULL)
            {
                printf("can not open file \n");
                exit(EXIT_FAILURE);
            }
        }
        else if (i == 11)
        {
            arg_SURL = i;
            starting_URL = mallocNcheck(strlen(argv[arg_SURL]) + 1);
            strcpy(starting_URL, argv[arg_SURL]);
        }
    }

return 0;
}

int get_serving_port(void)
{
    return serving_port;
}
int get_serving_port_length(void)
{
    return serving_port_length;
}
int get_command_port(void)
{
    return command_port;
}
int get_num_of_threads(void)
{
    return num_of_threads;
}
int get_arg_host(void)
{
    return arg_host;
}
int get_arg_save_dir(void)
{
    return arg_save_dir;
}
int get_arg_SURL(void)
{
    return arg_SURL;
}
char* get_host(void)
{
    return host;
}
char* get_save_dir(void)
{
    return save_dir;
}
char* get_starting_URL(void)
{
    return starting_URL;
}
