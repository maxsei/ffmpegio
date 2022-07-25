#include "panic.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int panicf(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	const int ret = fprintf(stderr, fmt, argp);
	if (ret != 0)
		exit(ret);
	va_end(argp);
	exit(1);
}

int panic(const char *msg)
{
	return panicf(msg);
}
