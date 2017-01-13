#ifndef _MAIN_H
#define _MAIN_H

#define DEBUG 1
#define debug(msg) \
	if (DEBUG) printf("%s\n", msg)

extern void log_msg(const char *fmt, ...);
extern int forward(int fd, int msgid, GHashTable *users);

int webim_process(GHashTable *users, int msgid);
void sig_chld(int signo);

void free_key(gpointer data);
void free_value(gpointer data);

#endif
