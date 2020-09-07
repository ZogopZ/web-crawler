#ifndef _CMDLINE_UTILS_H_
#define _CMDLINE_UTILS_H_

extern FILE *fp_docfile;

int get_serving_port(void);

int get_serving_port_length(void);

int get_command_port(void);

int get_num_of_threads(void);

int get_arg_host(void);

int get_arg_save_dir(void);

int get_arg_SURL(void);

char* get_host(void);

char* get_save_dir(void);

char* get_starting_URL(void);

int parse_cli_args(int, char** );

#endif /* _CMDLINE_UTILS_H_ */
