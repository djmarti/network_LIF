#ifndef _PARSER_H
#define _PARSER_H 1
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "eprintf.h"
#include "network.h"
#include "simulation.h"

#define MAX_CHAR_SYMBOLS 25
#define MAX_LINE 200


struct List {
    /* This is a linked list of ints. It contains the ids of with all the
       populations that are created because
       they are the target of some other population, but that have not yet been
       defined (Stefano's code would read the conf file twice. I prefer not to
       do that). This list will be used at the end of parse_config_file to
       check that no Population is left undefined. */ 
    int id_pop; 
    struct List *next; 
};

/* parser.c */
_Bool is_numeric(const char *s);
char *rstrip(char *s);
char *lskip(char *s);
char *unquote(char *s);
char *find_char_or_comment(char *s, char c);
char *strncpy0(char *dest, const char *src, size_t size);
char *get_contents_in_brackets(char *s, int n);
void usage(int status, char *s);
int parse_config_file(const char *filename, struct State *S);
int process_options(void *s, int, char *[]);
int read_network_parameters(int argc, char *argv[], struct State *S);
#endif
