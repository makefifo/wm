#!/bin/bash
gcc main.c cjson/cJSON.c forward.c parser.c log.c msgbuf.c -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include  -L/usr/local/lib -lglib-2.0   -lm -L /usr/local/mysql/lib/mysql -l mysqlclient
