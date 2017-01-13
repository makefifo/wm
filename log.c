#include <stdio.h> 
#include <string.h> 
#include <stdarg.h> 
#include <time.h>

void log_msg(const char *fmt, ...)
{
	int n;
	va_list ap;
	FILE *fp;
	char buf[32];

	fp = fopen("webim.log", "a");
	if (!fp)
		return;

	get_time(buf);
	va_start(ap, fmt);

	vfprintf(fp, fmt, ap);
	fprintf(fp, " %s\n", buf);
	//fprintf(fp, "\n");

	va_end(ap);
	fclose(fp);
}

int get_time(char *buf)
{
	time_t tp;
	struct tm *p;

	time(&tp);
	//p = gmtime(&tp);
	p = localtime(&tp);
	sprintf(buf, "%d-%d-%d %d:%d:%d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

	return strlen(buf);	
}

