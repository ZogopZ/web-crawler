#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "workers.h"
#include "doc_utils.h"

int docfile_lc;
int number_of_lines;


int set_docfile_lines(char *argv)
{
    int c;
    FILE *fp;

    if((fp = fopen(argv, "r")) == NULL)
    {
        printf("can not open file \n");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        c = fgetc(fp);
        if (c == '\n')
        {
            docfile_lc++;
        }
        else if (c == EOF)
            break;
    }
    fclose(fp);
    return(docfile_lc);
}

int get_docfile_lines(void)
{
        return(docfile_lc);
}

void store_docfile_lines(char *argv, struct docfile_line_info *array_lines)
{
    int c, i, chars;

    FILE *fp;

    if((fp = fopen(argv, "r")) == NULL)
    {
        printf("can not open file \n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < docfile_lc; i++)
    {
        chars = 0;
        while (1)
        {
            c = fgetc(fp);
            if (c == '\n')
                break;
            else
                chars++;                    //Count chars of each docfile line
        }
        fseek(fp, -(chars+1), SEEK_CUR);    //Go back chars times plus one for \n
        array_lines[i].lchars_counter = chars+1;
        array_lines[i].docfile_line = malloc((chars+1)*sizeof(char)); //+1 for \0
        memset(array_lines[i].docfile_line, 0,chars+1);
        //strcat(array_lines[i].docfile_line, "\0");
        if (array_lines[i].docfile_line == NULL)
        {
            printf("Not enough memory in heap\n");
            exit(EXIT_FAILURE);
        }
        if((fgets(array_lines[i].docfile_line, chars+1, fp)) == NULL)
        {
            printf("Could not read\n");
            exit(EXIT_FAILURE);
        }
        fseek(fp, 1, SEEK_CUR);             //Go 1 character forward to skip \n
    }
    fclose(fp);
    return;

}

int count_digits(int number)
{
    int count = 0;

    while(number != 0)
    {
        number = number/10;
        count++;
    }
    return(count);
}

//**********************************//
/*      FUNCTIONS FROM PROJECT 1    */
//**********************************//

void set_numb_of_lines(FILE *fp)
{
    int c, lc;

    lc = 0;
    while (1)
    {
            c = fgetc(fp);
            if (c == '\n')
            {
                    lc++;
            }
            else if (c == EOF)
            {
                    break;
            }
    }
    //Set fp to beginning of file
    fseek(fp, 0, SEEK_SET);
    number_of_lines = lc;
}

int get_numb_of_lines(void)
{
        return (number_of_lines);
}

int get_ID(FILE *fp)
{
        int numb,c;

        //Get the document ID
        fscanf(fp, "%d", &numb);
        //Loop until new line
        while (1)
        {
                c = fgetc( fp );
                if (c == '\n')
                {
                        break;
                }
        }
        return numb;
}

int check_line_numbs(FILE *fp, int numb_of_lines)
{
        for (int i = 0; i < numb_of_lines; i++)
        {
               if (get_ID(fp) != i)
               {
                       return -1;
               }
        }
        return 0;
}

int validate_doc(char *argv)
{
        FILE *fp;

        fp = fopen(argv, "r");
        if (!fp)
        {
                printf("can not open file \n");
                return 0;
        }
        set_numb_of_lines(fp);
        //Docfile's documents ID should have specific serial number
        if (check_line_numbs(fp, get_numb_of_lines()) == -1)
        {
                printf("invalid file \n");
                return 0;
        }
        fclose(fp);
        return 1;
}

void skip_space_chars(FILE *fp)
{
        int c;
        while (1)
        {
                c = fgetc(fp);
                if ( c != ' ' )
                {
                        break;
                }
        }
        //Set fp back by 1 so that we don't lose a non space character
        fseek(fp, -1, SEEK_CUR);
}

int get_numb_of_chars_per_line(FILE *fp)
{
        int c, clc;
        clc = 0;

//        //Skip document ID and do not count it
//        fscanf(fp, "%d", &numb);
//        //Skip space characters until the first non space character
//        skip_space_chars(fp);
        while (1)
        {
                c = fgetc(fp);
                if (c != '\n')
                {
                        clc++;
                }
                else
                {
                        break;
                }
        }
        //Set fp back by clc+1
        fseek(fp, -(clc+1), SEEK_CUR);
        return clc;
}
