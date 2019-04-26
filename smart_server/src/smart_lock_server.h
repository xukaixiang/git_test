#ifndef __SMART_LOCK_H__
#define __SMART_LOCK_H__

#include<stdio.h>

#include<sys/types.h>
#include<sys/socket.h>

#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include<stdbool.h>

#include<pthread.h>

#include"../json/json_string_tool.h"
#include"../sqlite/sqlite3_tool.h"
#include"../kernellist/kernellist.h"
#include"../kernellist/list.h"

#define MAX_FD_SIZE 1024

#endif
