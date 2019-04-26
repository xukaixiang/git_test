#include "sqlite3_tool.h"

/*插入客户数据*/
void insert(struct client_info *cli_info,char *t_name)
{
		int ret = -1;
		sqlite3 *db;

		/*打开数据库*/
		ret = sqlite3_open("./clientdatabase.db", &db);
		if(SQLITE_OK != ret){
			sqlite3_close(db);
			printf("open database failed!\n");
			pthread_exit(NULL);
		}

		bzero(sql,128);

		sprintf(sql, "insert into %s(id,usrname,housenum,telenum) values(null,'%s', '%s', '%s')",
																				t_name,
																				cli_info->usrname,
																				cli_info->housenum,
																				cli_info->telenum);
		ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
		if(SQLITE_OK != ret){
			sqlite3_close(db);
			printf("insert:%s\n", errmsg);
			exit(0);
		}

		sqlite3_close(db);

}
#if 0
/*根据用户名查询对应用户信息*/
void search_table(sqlite3 *db,char *t_name,char *usrname,int *state){

		char **result;
		int nrow = -1;
		int ncolumn = -1;
		int ret=-1;
		int i=0;

		//查询food表所有信息
		sprintf(sql, "select * from %s where USRNAME like '%s'", t_name,usrname);
		ret = sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg);

		//打印查询结果
//		printf(RED"菜单信息: \n");
		*state = 0;
		for(i=0; i<(nrow+1)*ncolumn; i++){
				if(!strcmp(usrname,result[i])){
					*state = 1;
					printf("该用户存在!\n");
				}

				printf("   %s\t",result[i]);
		}
		if(*state == 0){
			printf("没有该用户信息！\n");
		}
		
}
#else
/*根据信息查询对应用户状态*/
void search_table(char *t_name,char *key_val,char *msg,int *state,char *out){

		char **result;
		int nrow = -1;
		int ncolumn = -1;
		int ret=-1;
		int i=0;
		
		sqlite3 *db;

		/*打开数据库*/
		ret = sqlite3_open("./clientdatabase.db", &db);
		if(SQLITE_OK != ret){
			sqlite3_close(db);
			printf("open database failed!\n");
			pthread_exit(NULL);
		}

		//查询表所有信息
		sprintf(sql, "select * from %s where %s like '%s'", t_name,key_val,msg);
		ret = sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg);

		int idx = -1;

		if(strncmp(key_val,"USRNAME",strlen("USRNAME")) == 0)
		{
			idx = 1;
		}
		else if(strncmp(key_val,"TELENUM",strlen("TELENUM")) == 0)
		{
			idx = 3;
		}

		*state = 0;
		
		for(i=4; i<(nrow+1)*ncolumn; i+=4){

#if 0
			printf("[%d]>id:%s\t",__LINE__,result[i+0]);
			printf("[%d]>name:%s\t",__LINE__,result[i+1]);
			printf("[%d]>housrnum:%s\t",__LINE__,result[i+2]);
			printf("[%d]>telenum:%s\n",__LINE__,result[i+3]);
#endif

			if(strcmp(msg,result[i+idx]) == 0){
				*state = 1;
				sqlite3_close(db);

				//返回门牌号信息
				if(out != NULL){
					strncpy(out,result[i+2],strlen(result[i+2]));
				}
				
				return;
			}
		}
		
		sqlite3_close(db);
		
}
#endif

