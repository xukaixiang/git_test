#include "kernellist.h"

/*客户机节点内核链表初始化*/
void clientlist_init(struct client_node ** client_head_pointer){
   
	*client_head_pointer = (struct client_node *)malloc(sizeof(struct client_node));
    if(*client_head_pointer == NULL){
		perror("malloc failed!");
	    exit(-1);
	}   
	     
	memset(*client_head_pointer,0,sizeof(struct client_node));
		        
	/*初始化一个链表*/
	INIT_LIST_HEAD(&(*client_head_pointer)->list);
			   
}

/*
 *
 */
void vclientlist_init(struct client_vnode ** client_head_pointer){
   
	*client_head_pointer = (struct client_vnode *)malloc(sizeof(struct client_vnode));
    if(*client_head_pointer == NULL){
		perror("malloc failed!");
	    exit(-1);
	}   
	     
	memset(*client_head_pointer,0,sizeof(struct client_vnode));
		        
	/*初始化一个链表*/
	INIT_LIST_HEAD(&(*client_head_pointer)->list);
			   
}


/*
 *
 */
void rclientlist_init(struct client_reqnode ** client_head_pointer){
   
	*client_head_pointer = (struct client_reqnode *)malloc(sizeof(struct client_reqnode));
    if(*client_head_pointer == NULL){
		perror("malloc failed!");
	    exit(-1);
	}   
	     
	memset(*client_head_pointer,0,sizeof(struct client_reqnode));
		        
	/*初始化一个链表*/
	INIT_LIST_HEAD(&(*client_head_pointer)->list);
			   
}

#if 0
//遍历链表 并检测该用户是否在线(既链表里面有无该用户数据)
void clientlist_check(client_node *h,char *usrname,int *state){
	struct client_node *client_node_pos;
	list_for_each_entry(client_node_pos,&h->list,list){
		if(strcmp(client_node_pos->usrname,usrname) == 0){//该用户存在与链表里面

			/**/
			*state = 1;//存在为1
			break;
		}
		*state = 0;//不存在为0
	}
		

}
#endif

//遍历链表 根据门牌号进行匹配 如果匹配成功 则返回对应的套接字，反之返回-1
void clientlist_getsocket_by_housenum(struct client_node *h,char *housenum,int *resocket){

	struct client_node *client_node_pos;
	
	list_for_each_entry(client_node_pos,&h->list,list){

		//printf("usrname:%s housenum:%s telenum:%s\n",client_node_pos->usrname,client_node_pos->housenum,client_node_pos->telenum);

		if(strncmp(client_node_pos->housenum,housenum,strlen(housenum)) == 0){

			printf("门牌号匹配成功.\n");
			
			//通过指针从链表返回套接字
			*resocket = client_node_pos->client_socket;
			
			return;
		}
	}
	
	/*如果没有找到,套接字返回-1*/
	*resocket = -1;
}

//遍历链表 根据套接字进行匹配 如果匹配成功 则返回对应的节点地址，反之返回NULL
struct client_node * clientlist_getnode_by_socket(struct client_node *h,int socket){

	struct client_node *client_node_pos;
	
	list_for_each_entry(client_node_pos,&h->list,list){
		
		//匹配成功
		if(client_node_pos->client_socket == socket){
			return client_node_pos;
		}

		return NULL;
	}
}

