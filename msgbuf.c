#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "msgbuf.h"

int msg_crt(void)
{
	int msgid;

	if ((msgid = msgget(MSG_KEY, IPC_CREAT | 0600)) == -1) {
		log_msg("msgget error");
		return 0;
	}

	return msgid;
}

int msg_send(int msgid, struct pmsgbuf *mbuf)
{
	if (msgsnd(msgid, mbuf, sizeof(struct pmsgbuf), IPC_NOWAIT) == -1) {
		log_msg("msgsnd error");
		return 0;
	}

	return 1;
}

int msg_rcv(int msgid, struct pmsgbuf *mb)
{
	ssize_t len;

	// blocking here
	if ((len = msgrcv(msgid, mb, sizeof(struct pmsgbuf), mb->mtype, 0)) < 0) {
		return 0;
	}

	return len;
}

int msg_ctl(int msgid, struct msqid_ds *minfo)
{
	return msgctl(msgid, IPC_STAT, minfo);
}

int msg_del(int msgid)
{
	return msgctl(msgid, IPC_RMID, NULL); 
}
