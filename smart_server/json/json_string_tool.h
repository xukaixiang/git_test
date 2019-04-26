#ifndef __json_string_tool__
#define __json_string_tool__

#include<stdio.h>
#include</usr/local/include/json/json.h>
#include<string.h>


/*定义客户端信息结构体*/
struct client_info{
	char usrname[20];
	char housenum[20];
	char voideoid[20];
	char telenum[14];
	int videoid;
	
//	int client_socket;

	int state;
};

extern void json_string_pack(char *json_string,struct client_info *cli_info);

extern void json_string_unpack(char *json_string,struct client_info *cli_info);

#endif
