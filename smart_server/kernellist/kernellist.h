#ifndef __KERNELLIST_H__
#define __KERNELLIST_H__

#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include"list.h"

struct client_node{
	int client_socket;
	char usrname[20];
	char housenum[20];
	char telenum[20];
	struct list_head list;
};


//
struct client_vnode{

	int vedioID;
	int client_socket;
	char housenum[20];
	struct list_head list;
};


//
struct client_reqnode{

	int client_socket;
	char housenum[20];
	int source;				//请求的来源
	int tag_socket;         //目标的套接字
	struct list_head list;
};

#define SOURCE_FRONT  1
#define SOURCE_HUS    2
#define SOURCE_ALL    3

/*声明客户端内核链表初始化函数*/
extern void clientlist_init(struct client_node ** client_head_pointer);
extern void vclientlist_init(struct client_vnode ** client_head_pointer);
extern void rclientlist_init(struct client_reqnode ** client_head_pointer);
/*根据用户名检测用户是否登陆*/
extern void clientlist_check(struct client_node *h,char *usrname,int *state);
extern struct client_node * clientlist_getnode_by_socket(struct client_node *h,int socket);
extern void clientlist_getsocket_by_housenum(struct client_node *h,char *housenum,int *resocket);
#endif
