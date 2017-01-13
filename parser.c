#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "forward.h"

size_t parser(struct message *message)
{
	int i;
	unsigned method;
	int data_len = 0;
	int space_count = 0;
	char *p, *mp;
	char buf[256];

	for (p = message->body, i = 0; *p != '\0'; p++, i++) {
		if (i == 0) {
			if (*p == 'G')
				method = HTTP_GET;
			else if (*p == 'P')
				method = HTTP_POST;
			else
				method = HTTP_ERR;

			continue;
		}

		if (*p == ' ') {
			space_count++;
			continue;
		}

		if (method == HTTP_GET) {
			if (space_count == 1) {
				if (*p == '/') {
					mp = p + 1;
					continue;
				}

				data_len++;
			}
		} else if (method == HTTP_POST) {
			if (*p == '\n') {
				mp = p + 1;
				data_len = 0;	
				continue;
			}

			data_len++;
		}
	}

	if (data_len) {
		strncpy(buf, mp, data_len + 1);
		buf[data_len + 1] = '\0';
	}

	message->http_method = method;
	message->data = (char *)malloc(data_len + 1);
	if (message->data == NULL)
		return 0;

	strncpy(message->data, buf, data_len);
	message->data_size = data_len + 1;
	message->req_type = get_req_type(message->data);

	return data_len + 1;
}

short get_req_type(char *data)
{
	int i;
	unsigned type;
	char *p;
	char sign;
	char buf[16];
 
	sign = strstr(data, "?") ? '?' : ' ';

	for (p = data, i = 0; *p != '\0'; p++, i++) {
		if (*p == sign)
			break;
	}
	if (i == 1)
		return -1;

	strncpy(buf, data, i);
	buf[i] = '\0';

	if (strcmp(buf, "message") == 0)
		return RT_HOLD;
	else if (strcmp(buf, "online") == 0)
		return RT_ONLINE;
	else if (strcmp(buf, "status") == 0)
		return RT_STATUS;
	else if (strcmp(buf, "send") == 0)
		return RT_SEND;
	else if (strcmp(buf, "gusers") == 0)
		return RT_GUSERS;
}


