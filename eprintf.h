/* Copyright (C) 1999 Lucent Technologies */
/* Excerpted from 'The Practice of Programming' */
/* by Brian W. Kernighan and Rob Pike */
#ifndef _EPRINTF_H
#define _EPRINTF_H 1

/* eprintf.h: error wrapper functions */
extern void report(const char *, ...);
extern void eprintf(const char *, ...);
extern void weprintf(const char *, ...);
extern void f_printf(FILE *dev, const char *fmt, ...);
extern char *estrdup(const char *);
extern void *emalloc(size_t);
extern void *erealloc(void *, size_t);
extern char *progname(void);
char *string_replace(char *, char, char);
extern void setprogname(const char *);
#endif
