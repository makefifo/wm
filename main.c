#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <glib.h>

#include "main.h"
#include "msgbuf.h"

int main(void)
{
	int msgid;
	GHashTable *users = NULL;

	users = g_hash_table_new_full(g_int_hash, g_int_equal, free_key, free_value);

	msgid = msg_crt();
	if (msgid < 0) {
		log_msg("msgbuf error");
		exit(1);
	}
	webim_process(users, msgid);
	g_hash_table_destroy(users);
	msg_del(msgid);

	return 0;
}

int webim_process(GHashTable *users, int msgid)
{
	int listenfd, connfd;
	struct sockaddr_in servaddr;
	char buff[2048];
	char fdbuf[10];
	int i, n, pid;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_msg("socket error");
		return 1;
	}

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8001);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		log_msg("%s", strerror(errno));
		return 1;
	}

	if (listen(listenfd, 10) < 0) {
		log_msg("listen error");
		return 1;
	}

	signal(SIGCHLD, sig_chld);

	for (;;) {
		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
		if (connfd < 0) {
			// interrupt
			if (errno == EINTR) {
				continue;
			} else {
				log_msg("accept error");
				exit(1);
			}
		}
		
		if ((pid = fork()) < 0) {
			log_msg("fork error");
			exit(1);
		} else if (pid == 0) {
			close(listenfd);

			//sprintf(fdbuf, "%d", connfd);
			//execl("./forward", fdbuf, users, (char *)0);
			forward(connfd, msgid, users);
			exit(1);
		}

		close(connfd);
	}

	close(listenfd);
	return 0;
}

void sig_chld(int signo)
{
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0);
	return;
}

void free_key(gpointer data)
{
	free(data);
}

void free_value(gpointer data)
{
	free(data);
}
