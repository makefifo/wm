#ifndef _MSGBUF_H
#define _MSGBUF_H

#include <sys/ipc.h>
#include <sys/msg.h>

#define MSG_KEY 0xff
#define BUF_MAX 1024

struct msgnode {
	int from;
	int to;
	int type;
	char mtext[BUF_MAX];
	char nick[16];
};

struct pmsgbuf {
	long mtype;
	struct msgnode mn;
};

int msg_crt(void);
int msg_send(int msgid, struct pmsgbuf *mb);
int msg_rcv(int msgid, struct pmsgbuf *mb);
int msg_ctl(int msgid, struct msqid_ds *minfo);
int msg_del(int msgid);

extern void log_msg(const char *fmt, ...);

#endif
