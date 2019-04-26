#ifndef __SQLITE3_TOOL__
#define __SQLITE3_TOOL__

#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>

#include"../kernellist/kernellist.h"
#include"../json/json_string_tool.h"

#define SQL_SIZE    128

static char sql[SQL_SIZE] = {'\0'};
char *errmsg;

extern void insert(struct client_info *cli_info,char *t_name);
extern void search_table(char *t_name,char *key_val,char *msg,int *state,char *out);
extern void search_tachar (char *t_name,char *key_val,char *msg,int *state);

#endif
