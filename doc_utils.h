#ifndef _DOC_UTILS_H_
#define _DOC_UTILS_H_

#include "jobexec.h"

int set_docfile_lines(char *argv);

int get_docfile_lines(void);

void store_docfile_lines(char *argv, struct docfile_line_info *array_lines);

int count_digits(int number);

void set_numb_of_lines(FILE *fp);

int get_numb_of_lines(void);

int get_ID(FILE *fp);

int check_line_numb(FILE *fp, int number_of_lines);

int validate_doc(char *argv);

//get fp and number of line and returns number of chars of current line and fp in start of current line
int get_numb_of_chars_per_line(FILE *fp);

#endif /* _DOC_UTILS_H_ */

