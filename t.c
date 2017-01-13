#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tmp(char *str)
{
	printf("%d\n", strlen(str));
}

int main(void)
{

	printf("text\n");
	goto ret;

ret:
	printf("ret here\n");
}
