#ifndef _WEBCRAWLER_H_
#define _WEBCRAWLER_H_

#define SHUTDOWN -111
#define BUSY -99
#define COMMAND -333
#define SERVICE -444
#define EMPTY -222
#define OK 200
#define FORBIDDEN 403
#define MAXMSG  1024

extern long int total_bytes_served;
extern int pages_served;

extern int fd_comm;
extern int new_comm;
extern int max_fd;

extern fd_set set;

void webcrawler(void);

#endif /* _WEBCRAWLER_H_ */
