/*************************************************************************
	> File Name: comm.cpp
	> Author: bxq
	> Mail: 544177215@qq.com
	> Created Time: Sunday, December 20, 2015 AM07:37:50 CST
 ************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include "comm.h"


void main_print_char_line(const char ch, int len, int cr)
{
	int i;

	for (i = 0; i < len; i++)
		printf("%c", ch);
	if (cr)
		printf("\n");
}

void main_print_name_line(const char* pname, int len, int cr)
{
	int name_len = strlen(pname);
	int space_len = (len - name_len) / 2;

	main_print_char_line(' ', space_len, 0);
	printf("%s", pname);
	main_print_char_line(' ', space_len, 0);
	if (cr)
		printf("\n");
}

void main_print_app_info(const char* pname, uint32_t version, uint32_t port)
{
	int line_len = LINE_LEN;
	int ver_hi = version >> 16;
	int ver_lo = version & 0xFFFF;

	printf("\n");
	main_print_char_line('=', line_len, 1);
	main_print_name_line(pname, line_len, 1);
	main_print_char_line('=', line_len, 1);
	printf(" Version  : %d.%02d\n", ver_hi, ver_lo);
	printf(" Port     : %d\n", port);
	printf(" Copyright (c) 2019\n");
	main_print_char_line('=', line_len, 1);
}

void main_print_app_end(const char* pname)
{
	int line_len = LINE_LEN;
	main_print_char_line('*', line_len, 1);
	main_print_name_line(pname, line_len, 1);
	main_print_name_line("bye bye", line_len, 1);
	main_print_char_line('*', line_len, 1);
}
