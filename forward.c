#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <glib.h>
#include <mysql/mysql.h>

#include "cjson/cJSON.h"
#include "msgbuf.h"
#include "forward.h"

// socket fd
int fd, msgid;
GHashTable *users = NULL;
char *hold_cb = NULL;
struct message message;

int forward(int connfd, int msg_id, GHashTable *users_hash)
{
	enum request_type rt;

	fd = connfd;
	msgid = msg_id;
	users = users_hash;

	get_http_data(fd);
	if (!message.data_size) {
		//log_msg("data error");
		return 1;
	}

	rt = (enum request_type)message.req_type;
	switch (rt) {
		case RT_ONLINE:
			online();
			break;

		case RT_STATUS:
			status();
			break;

		case RT_HOLD:
			hold(fd);
			exit(0);

		case RT_SEND:
			send_msg();
			break;

		case RT_GUSERS:
			get_gusers();

		defaut:
			break;
	}
	close(fd);

	if (message.data_size)
		free(message.data);

	return 0;
}

unsigned int online(void)
{
	int i;
	int uid = 0;
	char cb[64];
	char *retstr;
	char *item;
	char *json_str;
	char *key, *val;
	char *data_str;
	int data_len;
	//struct online_para op;		
	GArray *pa, *da, *friends, *rooms;

	cJSON *root = cJSON_CreateObject();	
	//friends, gn == group_name
	cJSON *fr, *gn;	
	// rooms
	cJSON *rm;	
	cJSON *fr_items = cJSON_CreateArray();
	cJSON *rm_items = cJSON_CreateArray();

	pa = explode("&", message.data + 7);	
	for (i = 0; i < pa->len; i++) {
		item = strdup(g_array_index(pa, char *, i));
		da = explode("=", item);	

		key = g_array_index(da, char *, 0);
		val = g_array_index(da, char *, 1);
		
		if (strcmp(key, "uid") == 0)
			uid = atoi(val);
		else if (strcmp(key, "callback") == 0)
			strncpy(cb, val, strlen(val) + 1);
			
		g_array_free(da, TRUE);
	}
	g_array_free(pa, TRUE);

	if (uid == 0)
		uid = 100059;

	//set_userinfo(uid);
	// my friends
	if (friends = get_friends(uid)) {
		int fuid;
		struct user_info *ui;

		for (i = 0; i < friends->len; i++) {
			//fuid = g_array_index(friends, int, i);
			ui = g_array_index(friends, struct user_info *, i);

			/*
			if ((ui = (struct user_info *)g_hash_table_lookup(users, &fuid)) != NULL) {
				//printf("%d\n", ui->uid);	
			}
			*/

			fr = cJSON_CreateObject();	
			cJSON_AddNumberToObject(fr, "uid", ui->uid);
			cJSON_AddStringToObject(fr, "nick", ui->tname);
			cJSON_AddNumberToObject(fr, "show", 1);
			cJSON_AddNumberToObject(fr, "group", ui->gid);
			cJSON_AddItemToArray(fr_items, fr);
		}
	} else {
		fr = cJSON_CreateObject();	
	}
	g_array_free(friends, TRUE);

	// group
	if (rooms = get_rooms(uid)) {
		struct room *room;

		for (i = 0; i < rooms->len; i++) {
			room = g_array_index(rooms, struct room *, i);	

			rm = cJSON_CreateObject();	
			cJSON_AddNumberToObject(rm, "roomid", room->id);
			cJSON_AddStringToObject(rm, "name", room->name);
			cJSON_AddStringToObject(rm, "pic", room->pic);
			cJSON_AddNumberToObject(rm, "nums", room->nums);
			cJSON_AddNumberToObject(rm, "onums", room->onums);
			cJSON_AddItemToArray(rm_items, rm);
		}

	} else {
		rm = cJSON_CreateObject();	
	}
	g_array_free(rooms, TRUE);

	cJSON_AddItemToObject(root, "retcode", cJSON_CreateNumber((double)0));

	gn = cJSON_CreateObject();	
	cJSON_AddItemToObject(gn, "-1", cJSON_CreateString("全部"));
	cJSON_AddItemToObject(gn, "0", cJSON_CreateString("不分组"));
	cJSON_AddItemToObject(gn, "1", cJSON_CreateString("同学"));
	cJSON_AddItemToObject(gn, "2", cJSON_CreateString("朋友"));
	cJSON_AddItemToObject(gn, "3", cJSON_CreateString("亲人"));
	cJSON_AddItemToObject(gn, "4", cJSON_CreateString("黑名单"));

	cJSON_AddItemToObject(root, "gname", gn);
	cJSON_AddItemToObject(root, "friends", fr_items);
	cJSON_AddItemToObject(root, "rooms", rm_items);

	json_str = cJSON_Print(root);
	data_len = strlen(json_str) + strlen(cb) + 3;
	data_str = (char *)malloc(data_len);
	snprintf(data_str, data_len, "%s(%s)", cb, json_str);
	response_str(data_str);

	free(data_str);
	cJSON_Delete(root);
	//cJSON_Delete(fr);
	//cJSON_Delete(rooms);
}

void status(void)
{
	int i;
	int uid = 0;
	char cb[64];
	char buf[32];
	char *show, *item, *key, *val;
	GArray *pa, *da;

	pa = explode("&", message.data + 7);	
	for (i = 0; i < pa->len; i++) {
		item = strdup(g_array_index(pa, char *, i));
		da = explode("=", item);	
		key = g_array_index(da, char *, 0);
		val = g_array_index(da, char *, 1);

		if (strcmp(key, "uid") == 0)
			uid = atoi(val);
		else if (strcmp(key, "show") == 0)
			show = strdup(val);
		else if (strcmp(key, "callback") == 0)
			strncpy(cb, val, strlen(val) + 1);
			
		g_array_free(da, TRUE);
	}
	g_array_free(pa, TRUE);

	memset(buf, 0, 32);
	snprintf(buf, 32, "%s(\"ok\")", cb);
	response_str(buf);

	free(show);
}

/* connection hold */
void hold(int fd)
{
	int i;
	int uid = 0;
	char *cb = NULL;
	char *item, *key, *val;
	struct pmsgbuf mb;
	int data_len;
	char *json_str;
	char *data_str;
	GArray *pa, *da;

	signal(SIGALRM, hold_timeout);

	cJSON *root = cJSON_CreateObject();	
	cJSON *msg_items = cJSON_CreateObject();	
	cJSON *status_items = cJSON_CreateObject();	
	cJSON *msg = cJSON_CreateArray();
	cJSON *status = cJSON_CreateArray();

	pa = explode("&", message.data + 8);	
	for (i = 0; i < pa->len; i++) {
		item = strdup(g_array_index(pa, char *, i));
		da = explode("=", item);	
		key = g_array_index(da, char *, 0);
		val = g_array_index(da, char *, 1);

		if (strcmp(key, "uid") == 0) {
			uid = atoi(val);
		} else if (strcmp(key, "callback") == 0) {
			cb = malloc(64);
			strncpy(cb, val, strlen(val) + 1);
		}
			
		g_array_free(da, TRUE);
	}
	g_array_free(pa, TRUE);

	hold_cb = cb;
	alarm(ALRM_SEC);

	if (uid == 0)
		uid = 100059;
	mb.mtype = uid;
	// process will be blocked here until the data recive
	msg_rcv(msgid, &mb);
	// clear alarm
	alarm(0);

	msg_items = cJSON_CreateObject();	
	cJSON_AddNumberToObject(msg_items, "from", mb.mn.from);
	cJSON_AddStringToObject(msg_items, "nick", mb.mn.nick);
	cJSON_AddNumberToObject(msg_items, "to", mb.mn.to);
	cJSON_AddNumberToObject(msg_items, "type", mb.mn.type);
	cJSON_AddStringToObject(msg_items, "boye", mb.mn.mtext);
	cJSON_AddItemToArray(msg, msg_items);

	status_items = cJSON_CreateObject();	
	cJSON_AddNumberToObject(status_items, "from", 100014);
	cJSON_AddStringToObject(status_items, "show", "offline");
	cJSON_AddItemToArray(status, status_items);

	cJSON_AddItemToObject(root, "retcode", cJSON_CreateNumber((double)0));
	cJSON_AddItemToObject(root, "message", msg);
	cJSON_AddItemToObject(root, "status", status);

	json_str = cJSON_Print(root);
	data_len = strlen(json_str) + strlen(cb) + 3;
	data_str = (char *)malloc(data_len);
	snprintf(data_str, data_len, "%s(%s)", cb, json_str);

	response_str(data_str);

	cJSON_Delete(root);
	if (cb)
		free(cb);
	free(data_str);
}

void send_msg(void)
{
	int i;
	char cb[64];
	char buf[128];
	char *item, *key, *val;
	GArray *pa, *da;
	struct pmsgbuf mbuf;

	//mbuf = malloc(sizeof(struct pmsgbuf));

	pa = explode("&", message.data + 5);	
	for (i = 0; i < pa->len; i++) {
		item = strdup(g_array_index(pa, char *, i));
		da = explode("=", item);	
		key = g_array_index(da, char *, 0);
		val = g_array_index(da, char *, 1);

		if (strcmp(key, "type") == 0) 
			mbuf.mn.type = atoi(val);
		else if (strcmp(key, "from") == 0)
			mbuf.mn.from = atoi(val);
		else if (strcmp(key, "to") == 0)
			mbuf.mn.to = atoi(val);
		else if (strcmp(key, "nick") == 0)
			strncpy(mbuf.mn.nick, val, strlen(val));
		else if (strcmp(key, "body") == 0)
			strncpy(mbuf.mn.mtext, val, strlen(val) + 1);
		else if (strcmp(key, "callback") == 0)
			strncpy(cb, val, strlen(val) + 1);
	}
	g_array_free(pa, TRUE);

	printf("body: %s\n", g_uri_unescape_string(mbuf.mn.mtext, NULL));

	mbuf.mtype = mbuf.mn.to;
	msg_send(msgid, &mbuf);

	memset(buf, 0, 128);
	snprintf(buf, 128, "%s(\"ok\")", cb);
	response_str(buf);
}

void get_gusers(void)
{
	int i;
	int gid = 0; 
	int uid = 0;
	int data_len;
	char cb[256];
	char *json_str;
	char *data_str;
	char *item, *key, *val;
	GArray *pa, *da, *gus;
	struct group_user *pgu;

	cJSON *root = cJSON_CreateArray();
	cJSON *gu_item;

	pa = explode("&", message.data + 7);	
	for (i = 0; i < pa->len; i++) {
		item = strdup(g_array_index(pa, char *, i));
		da = explode("=", item);	
		key = g_array_index(da, char *, 0);
		val = g_array_index(da, char *, 1);

		if (strcmp(key, "uid") == 0)
			uid = atoi(val);
		else if (strcmp(key, "gid") == 0)
			gid = atoi(val);
		else if (strcmp(key, "callback") == 0)
			strncpy(cb, val, strlen(val) + 1);

		g_array_free(da, TRUE);
	}
	g_array_free(pa, TRUE);

	if (uid == 0 || gid == 0)
		goto ret;

	gus = get_group_users(uid, gid);
	if (!gus)
		goto ret;
	
	for (i = 0; i < gus->len; i++) {
		pgu = g_array_index(gus, struct group_user *, i);

		gu_item = cJSON_CreateArray();
		cJSON_AddItemToArray(gu_item, cJSON_CreateNumber((double)pgu->uid));
		cJSON_AddItemToArray(gu_item, cJSON_CreateString(strdup(pgu->name)));
		cJSON_AddItemToArray(gu_item, cJSON_CreateNumber((double)pgu->status));

		cJSON_AddItemToArray(root, gu_item);
	}

ret:
	json_str = cJSON_Print(root);
	data_len = strlen(json_str) + strlen(cb) + 3;
	data_str = (char *)malloc(data_len);
	snprintf(data_str, data_len, "%s(%s)", cb, json_str);

	response_str(data_str);

	cJSON_Delete(root);
	free(data_str);
	return;
}

unsigned int 
get_http_data(int fd)
{
	int n, blen, dlen = 0;
	char buf[1024];

	if ((n = read(fd, message.body, 1023)) > 0) {
		buf[n] = '\0';

		if (blen = strlen(message.body)) {
			message.body_size = blen;
			parser(&message);
		} else {
			return 0;
		}
	}

	return n;
}

void hold_timeout(int signo)
{
	char buf[256];
	cJSON *root, *msg, *status;

	root = cJSON_CreateObject();	
	msg = cJSON_CreateArray();
	status = cJSON_CreateArray();

	cJSON_AddItemToObject(root, "retcode", cJSON_CreateNumber((double)0));
	cJSON_AddItemToObject(root, "message", msg);
	cJSON_AddItemToObject(root, "status", status);

	memset(buf, 0, 256);
	snprintf(buf, sizeof(buf), "%s(%s)", hold_cb, cJSON_Print(root));
	response_str(buf);

	cJSON_Delete(root);
	if (hold_cb)
		free(hold_cb);
	exit(signo);
}

GArray *explode(char *delim, char *str)
{
	char *p;
	GArray *array = g_array_new(FALSE, TRUE, sizeof(char *));

	for (p = strtok(str, delim); p; p = strtok(NULL, delim)) {
		g_array_append_val(array, p);
	}

	return array;
}

int set_userinfo(int uid)
{
	MYSQL mysql;
	MYSQL_RES *mysql_res;
	MYSQL_ROW mysql_row;
	struct user_info *ui;
	char sql[256];
	
	if (!mysql_init(&mysql)) {
		log_msg("error mysql");
		exit(1);
	}
	/* if (!mysql_real_connect(&mysql, mys.host, mys.username, mys.password, mys.dbname, 0, NULL, 0)) { */
	if (!mysql_real_connect(&mysql, "192.168.1.205", "snsdb", "test@2010", "ppl", 0, NULL, 0)) {
		log_msg("%s", mysql_error(&mysql));
		exit(1);
	}

	//printf("heere:%d\n", g_hash_table_lookup(users, &uid) == NULL);	
	printf("lookup:%d\n", g_hash_table_lookup(users, &uid) != NULL);
	if (g_hash_table_lookup(users, &uid) != NULL)
		return 0;

	memset(sql, 0, 256);
	sprintf(sql, "SELECT a.ub_truename, b.fg_id FROM ppl_user_base AS a INNER JOIN ppl_friend_rel AS b ON a.ub_id = b.ub_id WHERE a.ub_id = %d LIMIT 1", uid);
	mysql_query(&mysql, sql);
	if (mysql_res = mysql_store_result(&mysql)) {
		ui = malloc(sizeof(struct user_info));

		if (mysql_row = mysql_fetch_row(mysql_res)) {
			ui->uid = uid;
			ui->tname = strdup(mysql_row[0]);
			ui->gid = atoi(mysql_row[1]);

			g_hash_table_insert(users, &uid, ui);	
			printf("lookup:%d\n", g_hash_table_lookup(users, &uid) != NULL);
			//printf("heere:%d\n", g_hash_table_size(users));	
			//printf("heere:%d\n", g_hash_table_lookup(users, &uid) == NULL);	
		}
	}
	mysql_close(&mysql);

	return 1;
}

GArray *get_friends(int uid)
{
	int fuid;
	char sql[100];
	GArray *friends = NULL;
	struct user_info *ui;
	
	MYSQL mysql;
	MYSQL_RES *mysql_res;
	MYSQL_ROW mysql_row;
	
	if (!mysql_init(&mysql)) {
		log_msg("error mysql");
		exit(1);
	}
	/* if (!mysql_real_connect(&mysql, mys.host, mys.username, mys.password, mys.dbname, 0, NULL, 0)) { */
	if (!mysql_real_connect(&mysql, "192.168.1.205", "snsdb", "test@2010", "ppl", 0, NULL, 0)) {
		log_msg("%s", mysql_error(&mysql));
		exit(1);
	}

	memset(sql, 0, 100);
	sprintf(sql, "SELECT fr_uid, fr_truename, fg_id FROM ppl_friend_rel WHERE ub_id = %d AND fr_status = 1", uid);
	//printf("%s\n", sql);
	mysql_query(&mysql, sql);
	if (mysql_res = mysql_store_result(&mysql)) {
		friends = g_array_new(FALSE, FALSE, sizeof(struct user_info *));

		while (mysql_row = mysql_fetch_row(mysql_res)) {
			//fuid = atoi(mysql_row[0]);
			ui = malloc(sizeof(struct user_info));
			ui->uid = atoi(mysql_row[0]);
			ui->tname = strdup(mysql_row[1]);
			ui->gid = atoi(mysql_row[2]);
			g_array_append_val(friends, ui);
		}
	}
	mysql_close(&mysql);

	return friends;
}

GArray *get_rooms(int uid)
{
	char sql[256];
	struct room *room;
	GArray *rooms = NULL;
	
	MYSQL mysql;
	MYSQL_RES *mysql_res;
	MYSQL_ROW mysql_row;
	
	if (!mysql_init(&mysql)) {
		log_msg("error mysql");
		exit(1);
	}
	/* if (!mysql_real_connect(&mysql, mys.host, mys.username, mys.password, mys.dbname, 0, NULL, 0)) { */
	if (!mysql_real_connect(&mysql, "192.168.1.205", "snsdb", "test@2010", "ppl", 0, NULL, 0)) {
		log_msg("%s", mysql_error(&mysql));
		exit(1);
	}

	memset(sql, 0, 256);
	sprintf(sql, "SELECT a.group_id, b.group_name, b.group_pic, b.group_nums FROM ppl_group_user AS a INNER JOIN ppl_group AS b ON a.group_id = b.group_id WHERE gu_uid = %d  and gu_grade >= 0", uid);

	mysql_query(&mysql, sql);
	if (mysql_res = mysql_store_result(&mysql)) {
		rooms = g_array_new(FALSE, FALSE, sizeof(struct room *));

		while (mysql_row = mysql_fetch_row(mysql_res)) {
			room = malloc(sizeof(struct room));
			room->id = atoi(mysql_row[0]);
			room->name = strdup(mysql_row[1]);
			room->pic = strdup(mysql_row[2]);
			room->nums = atoi(mysql_row[3]);
			// todo
			room->onums = 1;

			g_array_append_val(rooms, room);
		}
	}
	mysql_close(&mysql);

	return rooms;
}

GArray *get_group_users(int uid, int gid)
{
	char sql[256];
	struct group_user *guser;
	GArray *gus = NULL;
	
	MYSQL mysql;
	MYSQL_RES *mysql_res;
	MYSQL_ROW mysql_row;
	
	if (!mysql_init(&mysql)) {
		log_msg("error mysql");
		exit(1);
	}
	/* if (!mysql_real_connect(&mysql, mys.host, mys.username, mys.password, mys.dbname, 0, NULL, 0)) { */
	if (!mysql_real_connect(&mysql, "192.168.1.205", "snsdb", "test@2010", "ppl", 0, NULL, 0)) {
		log_msg("%s", mysql_error(&mysql));
		exit(1);
	}

	memset(sql, 0, 256);
	//sprintf(sql, "SELECT b.gu_uid, b.gu_truename FROM ppl_group AS a INNER JOIN ppl_group_user AS b ON a.group_id = b.group_id WHERE a.ub_id = %d and b.gu_grade >= 0", uid);
	sprintf(sql, "SELECT b.gu_uid, b.gu_truename FROM ppl_group AS a INNER JOIN ppl_group_user AS b ON a.group_id = b.group_id WHERE b.group_id = %d and b.gu_grade >= 0", gid);

	mysql_query(&mysql, sql);
	if (mysql_res = mysql_store_result(&mysql)) {
		gus = g_array_new(FALSE, FALSE, sizeof(struct group_user *));

		while (mysql_row = mysql_fetch_row(mysql_res)) {
			guser = malloc(sizeof(struct group_user));
			guser->uid = atoi(mysql_row[0]);
			guser->name = strdup(mysql_row[1]);
			guser->status = 1;

			g_array_append_val(gus, guser);
		}
	}
	mysql_close(&mysql);

	return gus;
}

ssize_t response_str(char *str)
{
	time_t tp;
	size_t len, hlen, rlen;
	char *rsp;
	char buf[256];

	time(&tp);

	hlen = get_response_header(buf, strlen(str));
	rlen = hlen + strlen(str) + 1;
	rsp = (char *)malloc(rlen);
	if (rsp == NULL) {
		log_msg("malloc error");
		exit(1);
	}

	snprintf(rsp, rlen, "%s%s", buf, str);
	//printf("%s\n", str);
	do {
		len = write(fd, rsp, rlen);
	} while (len == -1 && errno == EINTR);

	free(rsp);

	return len;
}

int get_response_header(char *header, size_t dlen)
{
	int len;
	char tbuf[32];

	get_gmt_time(tbuf);
	len = snprintf(header, 256, 
		"HTTP/1.1 200 OK\r\n"
		"Date: %s\r\n"
		"Server: webim/1.0\r\n"
		"Content-Type: application/javascript\r\n"
		"Content-Length: %d\r\n"
		"\r\n", 
		tbuf, dlen);

	return len;
}

int get_gmt_time(char *t)
{
	time_t tp;		
	struct tm *p;
	const char *weeks[]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	time(&tp);
	p = gmtime(&tp);

	return snprintf(t, 32, "%s, %d %s %04d %02d:%02d:%02d GMT", 
		weeks[p->tm_wday], p->tm_mday, months[p->tm_mon], p->tm_year + 1900, p->tm_hour, p->tm_min, p->tm_sec);
}

void parse_conf(char *filename)
{
	/*
	FILE *fp;
	char *p;
	char buf[100];
	struct conf_line cl;

	if (NULL == (fp = fopen(filename, "r"))) {
		log_msg("can't open config file");
		return;
	}

	while (fgets(buf, 100, fp) != NULL) {
		if (buf[0] == '\n')
			continue;

		cl = split_conf_line(buf);
		if (!strcmp(cl.k, "mysql_host"))
			mys.host = strdup(cl.v);
		else if (!strcmp(cl.k, "mysql_user"))
			mys.username = strdup(cl.v);
		else if (!strcmp(cl.k, "mysql_pass"))
			mys.password = strdup(cl.v);
		else if (!strcmp(cl.k, "mysql_db"))
			mys.dbname = strdup(cl.v);
		else if (!strcmp(cl.k, "redis_host"))
			rss.host = strdup(cl.v);
		else if (!strcmp(cl.k, "redis_port"))
			rss.port = atoi(cl.v);
		else if (!strcmp(cl.k, "memc_host"))
			mes.host = strdup(cl.v);
		else if (!strcmp(cl.k, "memc_port"))
			mes.port = atoi(cl.v);
		else if (!strcmp(cl.k, "data_path"))
			data_path = strdup(cl.v);
	}

	fclose(fp);
	*/
}

