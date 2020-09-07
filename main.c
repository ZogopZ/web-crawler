#include <stdio.h>
#include <stdlib.h>

#include "webcrawler.h"
#include "cmdline_utils.h"


int main(int argc, char **argv)
{


    if (parse_cli_args(argc, argv) == 1)
        exit(EXIT_FAILURE);
    webcrawler();   
    return 0;
}
