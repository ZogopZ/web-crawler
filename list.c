#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include "list.h"
#include "tools.h"
#include "webcrawler.h"
#include "cmdline_utils.h"

list_node_t *list_head = NULL;
int checker = 0;

list_node_t* new_list_crawlnode(int link_length)
{
    list_node_t *neos;

    neos = mallocNcheck(sizeof(list_node_t) + link_length);
    memset(neos, 0, sizeof(list_node_t) + link_length);
    neos->link_length = link_length;
    neos->next = NULL;
    neos->status = NOT_CHECKED;
    return neos;
}

int enlist(list_node_t* neos)
{
    list_node_t *list_temp;
    list_temp = list_head;



    if (list_head == NULL)
    {
        list_head = neos;
    }
    else
    {
        while (list_temp->next != NULL)
        {
            if (strcmp(neos->link, list_temp->link) == 0 ||
                    strcmp(neos->link, list_temp->next->link) == 0)
                return ENQUEUE_NO_SUCCESS;
            list_temp = list_temp->next;
        }
       list_temp->next = neos;
    }
    return ENQUEUE_SUCCESS;
}

list_node_t* search_list(void)
{
    list_node_t *list_temp;
    list_temp = list_head;

    if (list_head != NULL)
    {
        if (list_head->status == NOT_CHECKED)
        {
            list_head->status = CHECKED;
            checker++;
            printf("-THREAD-:%ld will now serve link:%s.\tLink count:%d\n", pthread_self(), list_temp->link, checker);
            return list_head;
        }
        else
        {
            while (list_temp->next != NULL)
            {
                if (list_temp->next->status == NOT_CHECKED)
                {
                    list_temp->next->status = CHECKED;
                    checker++;
                    printf("\n\n-THREAD-:%ld will now serve link:%s.\tLink count:%d\n", pthread_self(), list_temp->next->link, checker);
                    return list_temp->next;
                }
                list_temp = list_temp->next;
            }
        }
    }
    return NULL;

}

void free_list(void)
{
    list_node_t *temp;

    if (list_head != NULL)
    {
        while (1)
        {
            temp = list_head;
            if (temp->next != NULL)
            {
                list_head = temp->next;
                free(temp);
            }
            else
            {
                free(temp);
                break;
            }
        }
    }
    return;
}



