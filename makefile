#############################################################################
# Makefile for: Syspro: Project 3: webcrawler				    #
#############################################################################


CC            = gcc
CFLAGS        = -g -Wall -W -pthread
DEL_FILE      = rm -f
DEL_DIR	      = rm -r

OBJECTS       = main.o cli.o cmdline_utils.o doc_utils.o jobexec.o list.o map.o posting_list.o retrie.o tools.o webcrawler.o workers.o

default: crawler

crawler: $(OBJECTS)
	$(CC) $(CFLAGS) -o crawler $(OBJECTS)

cli.o: cli.c cli.h \
	     map.h \
	     retrie.h \
	     jobexec.h \
	     workers.h \
	     doc_utils.h \
	     posting_list.h 
	$(CC) -c $(CFLAGS) -o cli.o cli.c

cmdline_utils.o: cmdline_utils.c tools.h \
				 cmdline_utils.h
	$(CC) -c $(CFLAGS) -o cmdline_utils.o cmdline_utils.c

doc_utils.o: doc_utils.c workers.h \
			 doc_utils.h 
	$(CC) -c $(CFLAGS) -o doc_utils.o doc_utils.c

jobexec.o: jobexec.c cli.h \
		     jobexec.h \
		     workers.h \
		     doc_utils.h 
	$(CC) -c $(CFLAGS) -o jobexec.o jobexec.c

list.o: list.c list.h \
	       tools.h \
	       webcrawler.h \
	       cmdline_utils.h 
	$(CC) -c $(CFLAGS) -o list.o list.c

main.o: main.c webcrawler.h \
	       cmdline_utils.h 
	$(CC) -c $(CFLAGS) -o main.o main.c

map.o: map.c cli.h \
		map.h \
		doc_utils.h 
	$(CC) -c $(CFLAGS) -o map.o map.c

posting_list.o: posting_list.c map.h \
			       posting_list.h
	$(CC) -c $(CFLAGS) -o posting_list.o posting_list.c

retrie.o: retrie.c retrie.h \
		   posting_list.h
	$(CC) -c $(CFLAGS) -o retrie.o retrie.c

tools.o: tools.c cli.h \
		 list.h \
		 tools.h \
		 jobexec.h \
		 webcrawler.h \
		 cmdline_utils.h 
	$(CC) -c $(CFLAGS) -o tools.o tools.c

webcrawler.o: webcrawler.c list.h \
			   tools.h \
			   webcrawler.h \
			   cmdline_utils.h
	$(CC) -c $(CFLAGS) -o webcrawler.o webcrawler.c

workers.o: workers.c map.h \
		     retrie.h \
		     jobexec.h \
		     workers.h \
		     doc_utils.h \
		     posting_list.h \
		     cmdline_utils.h
	$(CC) -c $(CFLAGS) -o workers.o workers.c

clean:
	-$(DEL_FILE)  crawler
	-$(DEL_FILE)  $(OBJECTS)
	-$(DEL_DIR) save_dir
