#include "smart_lock_server.h"
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

//数据库的表名
#define TABEL_NAME    "cliinfo"
//注册回应
#define REG_ACK_OK    "0"
#define REG_ACK_ER    "1"
//登录回应
#define LOGIN_ACK_OK  "0"
#define LOGIN_ACK_ER  "1"

#define HOUSENUM      "xxxx"

//大门端的门牌号
#define FORTNAME      "front"

/*stata 的取值*/
#define TAG_REG             0      //表明消息与注册相关
#define TAG_LOG             1	   //表明消息与登录相关
#define TAG_FORT_ID         2      //表明消息与大门的videoID相关
#define TAG_OPEN_CMD        3
#define TAG_FORT_OPEN_REQ   4
#define TAG_HUS_OPEN_REQ    5
#define TAG_HOUS_ID         6	   //表明消息与家门的videoID相关

/*客户端请求的服务类型*/
#define SERTYPE_APP_REG     0     //移动端注册
#define SERTYPE_APP_LOG     1	  //移动端登录
#define SERTYPE_HUS_LOG     4	  //家门端登录
#define SERTYPE_FORT_LOG    5     //大门端登录

/*****************************************************************************************
 *
 */
//用于暂存大门端的videoID
int FrontVedioID = 0;

//用于保存json字符串
char json_string[128] = {0};	

//用于保存 在线住户端信息链表的头结点地址
struct client_node *client_node_pointer_h;
//用于记录有多少个用户在线
int usr_cnt = 0;

//用于保存vedeoid以及与其相关信息链表的头结点地址
struct client_vnode *client_videoid_node_h;
//用于记录链表中有多少个videoID节点
int vde_cnt = 0;

//用于保存发出开门请求的家门端信息
struct client_reqnode *client_requst_node_h;
//用于记录有多少个开门请求
int req_cnt = 0;

//返回值
int ret = 0;



/*****************************************************************************************
 * local function.
 */
static void *cli_data_handle(void *);
static void Servo_MobileTerminal(int ,char *,char *);
static void Servo_FrontDoor(int ,char *);
static void Servo_HouseDoor(int ,char *);
static int Private_DataTreating(int ,struct client_info *);
static int addInlinUsr(char *,char *,char *,int );
static int House_Video_add(char *,int ,int );
static int House_requster_add(char *,int ,int);
static int delInlinUsr(char *);
static int House_Video_del(char *);
static int House_requster_del(char *,int);
static struct client_node * InlinUsr_Check(char *,int *);
static struct client_reqnode * House_requster_check(char *,int *,int *);
static struct client_vnode * House_Video_Check(char *,int *);
static int updataFrontVideoID(int );
static int usrOutlineClear(char * usrname,char *housname,int socket);
static int doorOutlineClear(char *housname,int source);
static int sqlCheck(void);
static void alrmFunc(int sign_no);

/*
 *
 */
void main(int argc,char *argv[]){

	//服务器套接字
	int listenfd;

	//服务器套接字参数
	struct sockaddr_in servaddr;

	printf("[%d]>start running main...\n",__LINE__);

	//检查数据库
	sqlCheck();

	/*创建流式套接字*/
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	if(listenfd < 0){
		perror("server create socket failed!");
		exit(-1);
	}

	printf("[%d]>socket creat success.\n",__LINE__);

	int bReuseaddr = 1;
	
	/*设置服务器套接字ip和端口可重载*/
	ret = setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&bReuseaddr,sizeof(bReuseaddr));
	if(ret < 0){
		perror("setsockopt failed!");
		exit(-1);
	}

	//清空结构体变量
	memset(&servaddr,0,sizeof(servaddr));

	/*设置sockaddr_in结构体参数*/
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9999);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/*绑定sockvaddr*/
	ret = bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	if(ret < 0){
		perror("server bind failed！");
		exit(-1);
	}

	printf("[%d]>bind success.\n",__LINE__);

	/*调用listen()函数，设置监听模式*/
	ret= listen(listenfd,10);
	if(ret == -1){
		perror("server listen failed!");
		exit(-1);
	}

	printf("[%d]>客户端内核初始化...\n",__LINE__);
	
	/*客户信息内核链表初始化*/
	clientlist_init(&client_node_pointer_h);

	/*vedioid 链表初始化*/
	vclientlist_init(&client_videoid_node_h);

	/*requster 链表初始化*/
	rclientlist_init(&client_requst_node_h);
	
	struct sockaddr_in client_tmp_attr;
	socklen_t cli_len = sizeof(client_tmp_attr);
	pthread_t thread_id;
	//用于接收前来链接的客户端文件描述符
	int connect = -1;

	printf("[%d]>goto while...\n",__LINE__);

	signal(SIGALRM,alrmFunc);
	alarm(1);
	
	while(1){

			//等待客户端连接
			connect = accept(listenfd,(struct sockaddr*)&client_tmp_attr,&cli_len);
			if(connect < 0){
				perror("accept client error!");
				continue;
			}
			
			printf("[%d]>有客户端接入! socket:%d\n",__LINE__,connect);

			//每接入一个客户端就创建一个线程,单独处理业务
			ret= pthread_create(&thread_id,NULL,(void *)(cli_data_handle),(void *)connect);
			if(ret == -1){
				perror("ceate thread failed!");
			}
	}
}


/*
	线程处理函数
*/
static void *cli_data_handle(void *arg){

	int clifd_in_handle =(int)arg;
	int search_state;
	
	//用于接收客户端发送过来的数据缓冲
	char client_info_string[128] = {0};

	//json数据包
	struct client_info cli_info_tmp;

	printf("[%d]>#########新线程已启动！###########\nsocket:%d\n",__LINE__,clifd_in_handle);

	/*接收客户端信息*/
	ret = recv(clifd_in_handle,client_info_string,128,0);
	if(ret < 0){
		perror("recv data in thread error!");
		pthread_exit(NULL);
	}else if(ret == 0){
		printf("[%d]>客户端已断开连接.\n",__LINE__);
		pthread_exit(NULL);
	}

	//printf("[%d]>从客户端读到的数据为：%s\n",__LINE__,client_info_string);

	/*解析json字符串*/
	json_string_unpack(client_info_string,&cli_info_tmp);

	//识别业务类型
	switch(cli_info_tmp.state)
	{
		case SERTYPE_APP_REG:
		{
			printf("[%d]>收到住户端的注册请求\n",__LINE__);
	
			/*查找数据库判断是否已经存在该用户的信息*/
			search_table(TABEL_NAME,"USRNAME",cli_info_tmp.usrname,&search_state,NULL);
			
			//该用户已经存在
			if(search_state == 1){

				printf("[%d]>该用户已经存在.\n",__LINE__);

				//TAG_REG 表示注册操作的回应信息  REG_ACK_ER 表示注册失败
				json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_ER,0,TAG_REG);

				ret = send(clifd_in_handle,json_string,strlen(json_string),0);
				if(ret < 0){
					perror("send 1 error!");
					pthread_exit(NULL);
				}

				printf("[%d]>终止本次连接.\n",__LINE__);

				pthread_exit(NULL);
				
			}else if(search_state == 0){

				printf("[%d]>将用户信息写入到数据库...\n",__LINE__);
			
				/*插入新用户数据到数据库*/
				insert(&cli_info_tmp,TABEL_NAME);

				printf("[%d]>写入成功.\n",__LINE__);

				//TAG_REG 表示注册操作的回应信息 REG_ACK_OK 表示注册成功
				json_string_pack2(json_string,HOUSENUM,REG_ACK_OK,LOGIN_ACK_ER,0,TAG_REG);
				
				ret = send(clifd_in_handle,json_string,strlen(json_string),0);
				if(ret < 0){
					perror("send 0 error!");
					pthread_exit(NULL);
				}
			}

			printf("[%d]>注册处理完成，线程结束.\n",__LINE__);
			printf("##############################\n");
			pthread_exit(NULL);
			
		}break;

		case SERTYPE_APP_LOG:
		{
			printf("[%d]>收到住户端的登陆请求\n",__LINE__);
			
			int state1,state2;

			struct client_node *pnode = NULL;
			pnode = InlinUsr_Check(cli_info_tmp.usrname,NULL);
			if(pnode != NULL){
				printf("[%d]>该用户已登录.\n",__LINE__);
				printf("[%d]>本线程结束.\n",__LINE__);
				printf("################################\n");
				pthread_exit(NULL);
			}

			/*查询数据库判断该用户的登录信息是否正确*/
			search_table(TABEL_NAME,"USRNAME",cli_info_tmp.usrname,&state1,cli_info_tmp.housenum);
			search_table(TABEL_NAME,"TELENUM",cli_info_tmp.telenum,&state2,NULL);
			
			//用户名和密码都匹配
			if(state1 && state2)
			{		
				//TAG_LOG 表示登录操作的回应信息  LOGIN_ACK_OK 表示登录成功
				json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_OK,0,TAG_LOG);

				//发送登录应答
				ret = send(clifd_in_handle,json_string,strlen(json_string),0);
				if(ret < 0){
					perror("send ok failed in case 1!");
					pthread_exit(NULL);
				}
				
				printf("[%d]>登陆成功\n",__LINE__);

				//将该用户的相关信息加入到在线用户链表中 
				ret = addInlinUsr(cli_info_tmp.usrname,cli_info_tmp.housenum,cli_info_tmp.telenum,clifd_in_handle);
				if(ret < 0){
					printf("[%d]>用户添加失败.\n",__LINE__);

					//删除该用户的信息(在线链表)
					//delInlinUsr(cli_info_tmp.usrname);
						
					pthread_exit(NULL);
				}

				printf("[%d]>当前有 %d 个用户在线.\n",__LINE__,ret);

				//每一个住户端上线时 都需要接收大门端的videoID，以支撑后续的视频通话业务
				if(FrontVedioID != 0){

					printf("[%d]>转发大门端的VedioID...\n",__LINE__);
					
					//TAG_FORT_ID 表示大门端的videoID信息
					json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_ER,FrontVedioID,TAG_FORT_ID);

					//将信息发到该客户端
					ret = send(clifd_in_handle,json_string,strlen(json_string),0);
					if(ret < 0){
						perror("send open failed in case3!");
						//删除该用户的信息(在线链表)
						delInlinUsr(cli_info_tmp.usrname);
						pthread_exit(NULL);
					}
				}

				struct client_vnode *pnode = NULL;
				
				//检查是否有与该用户有关的家门端videoID
				pnode =  House_Video_Check(cli_info_tmp.housenum,NULL);
				if(pnode != NULL){

					printf("[%d]>检测到有该用户的videoID.\n",__LINE__);
					
					//TAG_HOUS_ID 表示小门端的videoID信息
					json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_ER,pnode->vedioID,TAG_HOUS_ID);

					//下发videoID给该用户
					ret = send(clifd_in_handle,json_string,strlen(json_string),0);
					if(ret < 0){
						perror("send open failed in case3!");
						//删除该用户的信息(在线链表)
						delInlinUsr(cli_info_tmp.usrname);
						//删除该用户的vedioID
						House_Video_del(cli_info_tmp.housenum);
						pthread_exit(NULL);
					}

					//转发完成之后需要将这个videoID节点信息删除
					ret = House_Video_del(cli_info_tmp.housenum);
					if(ret < 0){
						printf("[%d]>videoID 节点删除失败.\n",__LINE__);
						//删除该用户的信息(在线链表)
						delInlinUsr(cli_info_tmp.usrname);
						pthread_exit(NULL);
					}

					printf("[%d]>videoID 转发成功 剩余 %d 个.\n",__LINE__,ret);
				}
				
				//伺服处理
				Servo_MobileTerminal(clifd_in_handle,cli_info_tmp.housenum,cli_info_tmp.usrname);

			}
			else
			{
				printf("[%d]>账户信息不匹配，登陆失败.\n",__LINE__);

				//TAG_LOG 表示住户端登录请求的回应信息  LOGIN_ACK_ER 表示登录失败 
				json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_ER,0,TAG_LOG);

				ret = send(clifd_in_handle,json_string,strlen(json_string),0);
				if(ret < 0){
					perror("send 0 failed in case 1!");
				}

				pthread_exit(NULL);
			}
			
		}break;

		case SERTYPE_HUS_LOG:
		{
			printf("[%d]>家门端已登陆.\n",__LINE__);
			printf("[%d]>门牌号: %s\n",__LINE__,cli_info_tmp.housenum);

			//TAG_LOG 表示登录请求的回应信息  LOGIN_ACK_OK 表示登录成功
			json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_OK,0,TAG_LOG);

			//发送登录应答
			ret = send(clifd_in_handle,json_string,strlen(json_string),0);
			if(ret < 0){
				perror("send open failed in case3!");
				pthread_exit(NULL);
			}

			//伺服处理
			Servo_HouseDoor(clifd_in_handle,cli_info_tmp.housenum);
			
		}break;
		
		case SERTYPE_FORT_LOG:
		{
			printf("[%d]>大门端已登陆.\n",__LINE__);
			printf("[%d]>门牌号: %s\n",__LINE__,cli_info_tmp.housenum);

			//TAG_LOG 表示登录请求的回应信息  LOGIN_ACK_OK 表示登录成功
			json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_OK,0,TAG_LOG);

			//发送登录应答
			ret = send(clifd_in_handle,json_string,strlen(json_string),0);
			if(ret < 0){
				perror("send open failed in case3!");
				pthread_exit(NULL);
			}

			//伺服处理
			Servo_FrontDoor(clifd_in_handle,cli_info_tmp.housenum);
			
		}break;
		
		default:
		{
			printf("[%d]>未定义的数据包.\n",__LINE__);

		}break;
		
	}

}

/*
	移动端伺服处理
	服务器需要处理来自移动端的开门指令
	socket - 接入的套接字
	housname - 住户对应的门牌号(登陆后由服务器提供 即服务器从数据库中提取)
	usrname - 用户名
*/
static void Servo_MobileTerminal(int socket,char *housname,char *usrname){

	struct client_info mjson_pack;
	struct client_node *unode = NULL; 

	//用于记录发出 开门请求 的客户端socket
	int request_sockt = -1;
		
	printf("[%d]>移动端伺服环节...\n",__LINE__);

	while(1){
		
		//清零json数据包存储空间
		memset(&mjson_pack,0,sizeof(mjson_pack));

		printf("[%d]>等待用户端的信息.\n",__LINE__);

		//阻塞等待数据，收到数据自动解析json字符串
		ret = Private_DataTreating(socket,&mjson_pack);
		if(ret <= 0){

			printf("[%d]>有住户下线.\n",__LINE__);
			printf("[%d]>用户名: %s\n",__LINE__,usrname);

			//清理与该用户相关的信息
			usrOutlineClear(usrname,housname,socket);

			printf("[%d]>终止用户信息处理线程.\n",__LINE__);

			printf("##############################\n");

			//最后结束本线程
			pthread_exit(NULL);
			
		}else if(mjson_pack.state == 3){

			//接收到用户端下发的开门指令
			printf("[%d]>接收到用户端下发的开门指令.\n",__LINE__);

			struct client_reqnode *rnode = NULL;
			int source = 0;
			//根据门牌号查找是否有请求对象为该用户的开门请求
			rnode = House_requster_check(housname,&request_sockt,&source);
			if(rnode != NULL){
				
				printf("[%d]>找到与该用户匹配的请求.\n",__LINE__);

				if(source | SOURCE_FRONT){
					printf("[%d]>请求源为大门端.\n",__LINE__);
				}else if(source | SOURCE_HUS){
					printf("[%d]>请求源为家门端.\n",__LINE__);
				}
				
				//TAG_OPEN_CMD 表示开门命令
				json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_ER,0,TAG_OPEN_CMD);

				//将开门指令发到对应的门端
				ret = send(request_sockt,json_string,strlen(json_string),0);
				if(ret <= 0){
					perror("send open failed in case!");
					
					//清理与该用户相关的信息
					usrOutlineClear(usrname,housname,socket);
					
					pthread_exit(NULL);
				}

				//成功响应了该请求后，就得将该请求节点删除
				ret = House_requster_del(housname,source);
				if(ret >= 0){
					printf("[%d]>当前还有 %d 个开门请求没有被响应.\n",__LINE__,ret);
				}
				
			}else{

				printf("[%d]>开门指令无效,当前没有该用户的开门请求.\n",__LINE__);
			}
			
		}
		
	}
}

/*
	大门端伺服处理
	服务器需要处理来自大门端的开门请求以及上传的videoID
	socket - 接入的套接字
	housname - 大门端的门牌号(这里固定是 "FRONT" 在登录时上传的)
*/
static void Servo_FrontDoor(int socket,char *housname){

	int tag_socetk = 0;
	struct client_info fjson_pack;
	
	printf("[%d]>大门端伺服环节...\n",__LINE__);

	while(1){

		//清零json数据包存储空间
		memset(&fjson_pack,0,sizeof(fjson_pack));

		//阻塞等待数据，收到数据自动解析json字符串
		ret = Private_DataTreating(socket,&fjson_pack);
		if(ret <= 0){
			printf("[%d]>大门端已掉线.\n",__LINE__);
			//清理大门端的相关信息
			doorOutlineClear(fjson_pack.housenum,SOURCE_FRONT);
			printf("##############################\n");
			pthread_exit(NULL);
				
		}else if(fjson_pack.state == 3){

			//接收到大门端发来的开门请求
			printf("[%d]>接收到大门端发来的开门请求.\n",__LINE__);
			
			//根据门牌号找到对应的住户端套接字(大门端发送请求的时候同时会将目标用户的门牌号发送过来)
			clientlist_getsocket_by_housenum(client_node_pointer_h,fjson_pack.housenum,&tag_socetk);

			if(tag_socetk == -1){
				printf("[%d]>门牌号为 %s 的住户没在线.\n",__LINE__,fjson_pack.housenum);
				printf("[%d]>大门端本次开门请求无效.\n",__LINE__);
			}else {

				//首先将大门端的请求添加到链表中
				//这里之所以要保存是方便后面用户下发了开门指令时的匹配操作
				//根据这里的请求链表，可以跟轻松的确认当前是否有用于对应的请求
				ret = House_requster_add(fjson_pack.housenum,socket,SOURCE_FRONT);
				if(ret < 0){
					printf("[%d]>请求添加失败.\n",__LINE__);
					//清理大门端的相关信息
					doorOutlineClear(fjson_pack.housenum,SOURCE_FRONT);
					printf("##############################\n");
					pthread_exit(NULL);
				}

				//TAG_FORT_OPEN_REQ 表示大门端的开门请求
				json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_ER,0,TAG_FORT_OPEN_REQ);
				
				//转发请求信息 		
				ret= send(tag_socetk,json_string,strlen(json_string),0);
				if(ret < 0){
					perror("send open failed in case3!");
					//清理大门端的相关信息
					doorOutlineClear(fjson_pack.housenum,SOURCE_FRONT);
					printf("##############################\n");
					pthread_exit(NULL);
				}
			}
			
		}else if(fjson_pack.state == 2)	{
		
			printf("[%d]>接收到大门端发来的 vedioID 信息,vedioID: %d\n",__LINE__,fjson_pack.videoid);

			//遍历在线用户链表 将大门端的videoID转发到每一个用户端
			ret = updataFrontVideoID(fjson_pack.videoid);

			printf("[%d]>成功给 %d 个住户端转发了大门端的videoID.\n",__LINE__,ret);

			//记录大门端的videoID 供后面登录的住户端同步
			FrontVedioID = fjson_pack.videoid;				
		}
	}
}

/*
	小门端伺服处理
	服务器需要处理来自小门端的开门请求以及上传的videoID
	socket - 接入的套接字
	housname - 小门端的门牌号(在登录时上传的)
*/
static void Servo_HouseDoor(int socket,char *housname){

	int tag_socetk = -1;
	struct client_info hjson_pack;
	
	printf("[%d]>家门端伺服环节...\n",__LINE__);

	while(1){
		
		//清零json数据包存储空间
		memset(&hjson_pack,0,sizeof(hjson_pack));
		
		//阻塞等待数据，收到数据自动解析json字符串
		ret = Private_DataTreating(socket,&hjson_pack);
		if(ret <= 0){
			printf("[%d]>小门端已掉线.\n",__LINE__);
			//清理小门端的相关信息
			doorOutlineClear(housname,SOURCE_HUS);
			printf("##############################\n");
			pthread_exit(NULL);
				
		}else if(hjson_pack.state == 3){

			//接收到小门端发来的开门请求
			printf("[%d]>接收到门牌号为:%s 的小门端发来的开门请求.\n",__LINE__,housname);

			//查找对应住户端的套接字(根据门牌号查找)
			clientlist_getsocket_by_housenum(client_node_pointer_h,housname,&tag_socetk);

			if(tag_socetk == -1){
				printf("[%d]>门牌号为 %s 的住户没在线.\n",__LINE__,housname);
				printf("[%d]>小门端的本次开门请求无效.\n",__LINE__);
			}else {

				printf("[%d]>匹配到住户端.\n",__LINE__);

				//首先将客户端的请求添加到链表中
				//这里之所以要保存是方便后面用户下发了开门指令时的匹配操作
				//根据这里的请求链表，可以跟轻松的确认当前是否有用于对应的请求
				ret = House_requster_add(housname,socket,SOURCE_HUS);
				if(ret < 0){
					printf("[%d]>请求添加失败.\n",__LINE__);
					//清理小门端的相关信息
					doorOutlineClear(housname,SOURCE_HUS);
					printf("##############################\n");
					pthread_exit(NULL);
				}

				//TAG_HUS_OPEN_REQ 表示家门端的开门请求
				json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_ER,0,TAG_HUS_OPEN_REQ);
				
				//然后转发请求信息 		
				ret = send(tag_socetk,json_string,strlen(json_string),0);
				if(ret < 0){
					perror("send open failed in case3!");
					//清理小门端的相关信息
					doorOutlineClear(housname,SOURCE_HUS);
					printf("##############################\n");
					pthread_exit(NULL);
				}

				printf("[%d]>开门请求转发完毕.\n",__LINE__);
			}
			
		}else if(hjson_pack.state == 2)	{
		
			printf("[%d]>接收到小门端发来的 vedioID 信息,vedioID: %d\n",__LINE__,hjson_pack.videoid);
			
			//查找对应住户端的套接字
			clientlist_getsocket_by_housenum(client_node_pointer_h,housname,&tag_socetk);

			if(tag_socetk == -1){
				
				printf("[%d]>门牌号为 %s 的住户没在线.\n",__LINE__,housname);
				printf("[%d]>服务器暂存该videoID.\n",__LINE__);

				//打印暂存的信息
				printf("[%d]>housenum:%s videoid:%d socket:%d\n",__LINE__,hjson_pack.housenum,hjson_pack.videoid,socket);

				ret =  House_Video_add(hjson_pack.housenum,hjson_pack.videoid,socket);
				if(ret < 0){
					printf("[%d]>videoID 暂存失败.\n",__LINE__);
					//清理小门端的相关信息
					doorOutlineClear(housname,SOURCE_HUS);
					printf("##############################\n");
					pthread_exit(NULL);
				}	
				
			}else {

				//TAG_HOUS_ID 表示家门端的videoID信息
				json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_ER,hjson_pack.videoid,TAG_HOUS_ID);
			
				//转发请求信息 		
				ret = send(tag_socetk,json_string,strlen(json_string),0);
				if(ret < 0){
					perror("videoid send failed!");
					//清理小门端的相关信息
					doorOutlineClear(housname,SOURCE_HUS);
					printf("##############################\n");
					pthread_exit(NULL);
				}
			}				
		}
	}
	
}




/*
	私有数据接收以及解包
	return 1 表示正常
*/
static int Private_DataTreating(int socket,struct client_info *sjson){
	
	//用户缓存客户端发送过来的json数据
	char client_data[128] = {0};

	if(sjson == NULL){
		printf("[%d]>参数错误.\n",__LINE__);
		return -1;
	}

	printf("[%d]>阻塞...\n",__LINE__);
	
	/*读取客户端信息*/
	ret = read(socket,client_data,128);
	if(ret <= 0){
		return ret;
	}

	printf("[%d]>从客户端读到的数据为：%s\n",__LINE__,client_data);
	
	/*解析json字符串 得到json数据包*/
	json_string_unpack(client_data,sjson);

	return 1;
}



/*
 * 遍历在线用户链表 将大门端的videoID转发到每一个用户端
 * 返回 成功转发的个数 
 */
static int updataFrontVideoID(int videoID){

		int sucs_cnt = 0;
		struct client_node *newn = NULL;

		list_for_each_entry(newn,&client_node_pointer_h->list,list){
		
			//TAG_FORT_ID 表示大门端的videoID信息
			json_string_pack2(json_string,HOUSENUM,REG_ACK_ER,LOGIN_ACK_ER,videoID,TAG_FORT_ID);
		
			//转发请求信息 		
			ret = send(newn->client_socket,json_string,strlen(json_string),0);
			if(ret < 0){
				perror("videoid send failed!");
				FrontVedioID = 0;
				pthread_exit(NULL);
			}

			sucs_cnt++;
		}

		return sucs_cnt;

}


/*
 * 添加一位上线的用户信息
 * 添加成功返回 在线用户的总人数
 * 失败返回 -1
 */
static int addInlinUsr(char *usrname,char *housenum,char *telenum,int socket) {

		struct client_node *newn = NULL;

		if(usrname == NULL || housenum == NULL || telenum == NULL){
			printf("[%d]>参数错误.\n",__LINE__);
			return -1;
		}

		//首先遍历链表 若已经存在则直接返回
		list_for_each_entry(newn,&client_node_pointer_h->list,list){

			//根据用户名来匹配
			if(strncmp(usrname,newn->usrname,strlen(usrname)) == 0){
				printf("[%d]>发现有同名用户.\n",__LINE__);
				return usr_cnt;
			}
		}
	
		/*创建一个新节点*/
		newn = (struct client_node *)malloc(sizeof(struct client_node));
		if(NULL == newn){
			return -1;
		}

		//必须先清空节点 
		memset(newn,0,sizeof(struct client_node));

		/*把数据放入结构体*/
		strncpy(newn->usrname,usrname,strlen(usrname));
		strncpy(newn->housenum,housenum,strlen(housenum));
		strncpy(newn->telenum,telenum,strlen(telenum));
		newn->client_socket = socket;
		
		/*插入节点到头节点后面*/
		list_add_tail(&newn->list,&client_node_pointer_h->list);


#if 1
		printf("[%d]>添加的用户信息:usrname:%s  housenum:%s  telenum:%s  socket:%d\n",__LINE__,newn->usrname,
																							   newn->housenum,
																							   newn->telenum,
																							   newn->client_socket);

#endif
		//数量加一
		usr_cnt++;

		return usr_cnt;
				
}

 /*
 * 删除一个已登陆的用户信息
 * 成功删除时返回剩余的在线用户个数 
 * 删除失败时 返回-1
 */
static int delInlinUsr(char *usrname){
	
		struct client_node *vnode = NULL;

		if(usrname == NULL){
			printf("[%d]>参数错误.\n",__LINE__);
			return -1;
		}

		list_for_each_entry(vnode,&client_node_pointer_h->list,list){

			//printf("[%d]delusr>usrname:%s\n",__LINE__,vnode->usrname);

			//根据用户名来匹配
			if(strncmp(usrname,vnode->usrname,strlen(usrname)) == 0){

				//printf("[%d]>删除成功.\n",__LINE__);
				
				//直接删除
				list_del(&vnode->list);

				//释放该节点的空间
				free(vnode);

				//数量减一
				usr_cnt--;

				return usr_cnt;
			}
		}

		return -1;
}

/*
 * 查找对应的在线用户 
 * 查找成功返回对应的套接字,以及节点地址
 * 查找失败时 返回NULL
 */
static struct client_node * InlinUsr_Check(char *usrname,int *socket){
	
		struct client_node *vnode = NULL;

		if(usrname == NULL){
			printf("[%d]>参数错误.\n",__LINE__);
			return NULL;
		}

		list_for_each_entry(vnode,&client_node_pointer_h->list,list){

			//根据用户名来匹配
			if(strncmp(usrname,vnode->usrname,strlen(usrname)) == 0){

				if(socket != NULL){
					*socket = vnode->client_socket;
				}
				
				return vnode;
			}
		}
		
		if(socket != NULL){
			*socket = -1;
		}

		return NULL;
}


/*
 * 添加一个videoID
 * 添加成功返回请求的个数，失败返回-1
 */
static int House_Video_add(char *housname,int vedioID,int socket){
	
		struct client_vnode *newn = NULL;

		if(housname == NULL || socket < 0){
			printf("[%d]>参数错误.\n",__LINE__);
			return -1;
		}

		//首先遍历链表 若已经存在则直接返回
		list_for_each_entry(newn,&client_videoid_node_h->list,list){

			//根据门牌号来匹配
			if(strncmp(housname,newn->housenum,strlen(housname)) == 0){
				return vde_cnt;
			}
		}

		//新建一个节点
		newn = (struct client_vnode *)malloc(sizeof(struct client_vnode));
		if(NULL == newn){
			return -1;
		}

		//首先必须清空节点
		memset(newn,0,sizeof(struct client_vnode));
		
		strncpy(newn->housenum,housname,strlen(housname));
		newn->vedioID = vedioID;
		newn->client_socket= socket;
		
		/*插入节点到头节点后面*/
		list_add_tail(&newn->list,&client_videoid_node_h->list);

		//数量加一
		vde_cnt++;

		return vde_cnt;
}

 /*
 * 删除vedioID
 * 成功删除时返回剩余的videoID个数 
 * 删除失败时 返回-1
 */
static int House_Video_del(char *housname){
	
		struct client_vnode *vnode = NULL;

		if(housname == NULL){
			printf("[%d]>参数错误.\n",__LINE__);
			return -1;
		}

		list_for_each_entry(vnode,&client_videoid_node_h->list,list){

			//根据门牌号来匹配
			if(strncmp(housname,vnode->housenum,strlen(housname)) == 0){

				//删除节点
				list_del(&vnode->list);

				//释放空间
				free(vnode);

				//数量减一
				vde_cnt--;

				return vde_cnt;
			}
		}

		return -1;
}


/*
 * 查找对应的videoID
 * 查找成功返回对应的套接字,以及节点地址
 * 查找失败时 返回NULL
 */
static struct client_vnode * House_Video_Check(char *housname,int *socket){
	
		struct client_vnode *vnode = NULL;
		
		if(housname == NULL ){
			printf("[%d]>参数错误.\n",__LINE__);
			return NULL;
		}
		
		list_for_each_entry(vnode,&client_videoid_node_h->list,list){

			//根据门牌号来匹配
			if(strncmp(housname,vnode->housenum,strlen(housname)) == 0){

				if(socket != NULL){
					*socket = vnode->client_socket;
				}
				
				return vnode;
			}
		}

		if(socket != NULL){
			*socket = -1;
		}

		return NULL;
}

/*
 * 添加一个开门请求
 * 添加成功返回请求的个数，失败返回-1
 */
static int House_requster_add(char *housenum,int socket,int source){

		struct client_reqnode *newn = NULL;

		if(housenum == NULL){
			printf("[%d]>参数错误.\n",__LINE__);
			return -1;
		}

		//首先遍历链表 若已经存在则直接返回(避免重复添加)
		list_for_each_entry(newn,&client_requst_node_h->list,list){

			//根据门牌号来匹配
			if(strncmp(housenum,newn->housenum,strlen(housenum)) == 0){

				//匹配源 
				if(newn->source == source){
					return vde_cnt;
				}
			}
		}

		newn = (struct client_reqnode *)malloc(sizeof(struct client_reqnode));
		if(NULL == newn){

			return -1;
		}

		//首先必须清空节点
		memset(newn,0,sizeof(struct client_reqnode));
		
		strncpy(newn->housenum,housenum,strlen(housenum));
		newn->client_socket = socket;
		newn->source = source;

		/*插入节点到头节点后面*/
		list_add_tail(&newn->list,&client_requst_node_h->list);

		//请求的数量加一
		req_cnt++;

		return req_cnt;
		
}


/*
 * 删除小门端的无效请求
 * 成功删除时返回剩余的请求个数 
 * 删除失败时 返回-1
 */
static int House_requster_del(char *housname,int source){
	
		struct client_reqnode *rnode = NULL,*bnode = NULL;

		if(housname == NULL){
			printf("[%d]>参数错误.\n",__LINE__);
			return -1;
		}
		
		list_for_each_entry(rnode,&client_requst_node_h->list,list){

			//匹配门牌号
			if(strncmp(housname,rnode->housenum,strlen(housname)) == 0){

				//匹配来源 1 or 2
				if(rnode->source & source){

					//先获取到下一个节点的地址
					bnode = list_entry(rnode->list.next, struct client_reqnode, list);
					
					//删除节点
					list_del(&rnode->list);

					//释放空间
					free(rnode);

					//数量减一
					req_cnt--;

					if(source != SOURCE_ALL){
						rnode = bnode;
						continue;
					}

					return req_cnt;
				}
			}
		}

		if(source != SOURCE_ALL){
			return -1;
		}else{
			return req_cnt;
		}
}


/*
 * 查找对应的请求
 * 查找成功返回对应的套接字,以及节点地址
 * 查找失败时 返回NULL
 */
static struct client_reqnode * House_requster_check(char *housname,int *socket,int *source){
	
		struct client_reqnode *rnode = NULL;

		if(housname == NULL || socket == NULL || source == NULL){
			printf("[%d]>参数错误.\n",__LINE__);
			return NULL;
		}

		list_for_each_entry(rnode,&client_requst_node_h->list,list){

			//根据门牌号来匹配
			if(strncmp(housname,rnode->housenum,strlen(housname)) == 0){

				//提取套接字
				*socket = rnode->client_socket;
				//提取源
				*source = rnode->source;
				
				//返回节点地址 
				return rnode;
			}
		}
		
		*socket = -1;
		*source = -1;
		
		return NULL;
}

/*
 * 用户下线处理
 * housname - 该用户对应的门牌号
 * socket - 对应的套接字
 */
static int usrOutlineClear(char * usrname,char *housname,int socket){

	printf("[%d]>用户下线处理...\n",__LINE__);

	//删除与该用户相关的所有请求
	House_requster_del(housname,SOURCE_ALL);
	
	printf("[%d]>与该用户相关的请求清除成功...\n",__LINE__);

	House_Video_del(housname);

	printf("[%d]>与该用户相关的videoID清除成功...\n",__LINE__);

	//删除该用户的在线信息
	ret = delInlinUsr(usrname);
	if(ret >= 0){
		printf("[%d]>当前还有 %d 个用户在线.\n",__LINE__,ret);
	}
	
	printf("[%d]>用户下线处理完毕.\n",__LINE__);

	return 0;
}

/*
 * 门端下线处理
 * housname - 该用户对应的门牌号
 * socket - 对应的套接字
 */
static int doorOutlineClear(char *housname,int source){

	printf("[%d]>门端下线处理...\n",__LINE__);

	//清除门端已经发出但还未得到响应的开门请求 
	ret = House_requster_del(housname,source);
	if(ret >= 0){
		printf("[%d]>当前剩余 %d 个开门请求没有响应.\n",__LINE__,ret);
	}

	//清除家门端发出的所有未得到转发的videoID
	if(source | SOURCE_HUS){
		ret = House_Video_del(housname);
		if(ret >= 0){
			printf("[%d]>当前剩余 %d 个VideoID没有得到转发.\n",__LINE__,ret);
		}
	}

	//清除大门端发出的videoID
	if(source | SOURCE_FRONT){
		FrontVedioID = 0;
	}

	printf("[%d]>门端下线处理完成.\n",__LINE__);
	
	return 0;
}

/*
 * 检查数据库以及表 若没有则创建
 */
static int sqlCheck(void){

	sqlite3 *db;
	char sql[128] = {0};
	char *errmsg;
	
	printf("[%d]>检查数据库.\n",__LINE__);

	/*打开数据库 若没有则会自动创建*/
	ret = sqlite3_open("./clientdatabase.db",&db);
	if(SQLITE_OK != ret){
		sqlite3_close(db);
		printf("[%d]>数据库异常!\n",__LINE__);
		exit(1);
	}

	bzero(sql,128);
	
	//打开数据库后 在测试是否有数据表 "cliinfo"  若没有则创建
	sprintf(sql, "create table cliinfo(id int primary key, usrname text, housenum text, telenum text)");
	ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);

	sqlite3_close(db);
}


/*
 * 定时打印T，表示服务器运行正常，方便开发者判断.
 * 如果没有这个，当客户端连接出现异常时，因为服务器没有任何动静因此无法判断异常出在哪里
 */
static void alrmFunc(int sign_no){
	printf("[%d]>T\n",__LINE__);
	alarm(5);
}

