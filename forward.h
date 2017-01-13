#ifndef _FORWARD_H
#define _FORWARD_H

#define HTTP_ERR 0
#define HTTP_GET 1
#define HTTP_POST 2

#define MAX_BODY 1024
#define ALRM_SEC 20

#include "cjson/cJSON.h"
#include "main.h"

enum request_type {
	RT_ONLINE = 0,
	RT_OFFLINE,
	RT_STATUS,
	RT_HOLD,
	RT_SEND,
	RT_GUSERS
};

struct message {
	int status_code;
	unsigned http_method;
	short req_type;
	size_t body_size;
	char body[MAX_BODY];
	char *data;
	size_t data_size;
};

struct online_para {
	int uid;	
};

struct user_info {
	int uid;
	char *tname;
	int gid;
};

struct room {
	int id;
	char *name;
	char *pic;
	int nums; /* group nums  */
	int onums; /* group online nums  */
};	

struct group_user {
	int uid;
	int status;
	char *name;
};

typedef struct mysql_server {
	char *host; 	
	char *username;
	char *password;
	char *dbname;
} MY_SER; 

unsigned int get_http_data(int fd);
unsigned int online(void);
void status(void);
void hold(int fd);
void send_msg(void);
void get_gusers(void);
int set_userinfo(int uid);
void hold_timeout(int signo);

extern size_t parser(struct message *message);
extern short get_req_type(char *data);

int forward(int fd, int msgid, GHashTable *users);
GArray *explode(char *delim, char *str);
GArray *get_friends(int uid);
GArray *get_rooms(int uid);
GArray *get_group_users(int uid, int gid);

ssize_t response_json(cJSON *cj);
ssize_t response_str(char *str);
int get_response_header(char *header, size_t dlen);


#endif
