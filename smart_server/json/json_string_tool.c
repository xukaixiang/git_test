#include"json_string_tool.h"

/*josn字符串对象解包函数 并将数据放入结构体中*/
/*
*参数1：json格式字符串
*参数2：结构体
*/
void json_string_unpack(char *json_string,struct client_info *cli_info){

	struct json_object *fat,*json_usrname,*json_housenum,*json_telenum,*json_videoid;
	struct json_object *json_state;

	char *cli_usrname;
	char *cli_housenum;
	char *cli_telenum;
	int cli_videoid;
	int state;

	fat = json_tokener_parse(json_string);

	json_usrname = json_object_object_get(fat,"usrname");
	json_housenum = json_object_object_get(fat,"housenum");
	json_telenum = json_object_object_get(fat,"telenum");
	json_videoid = json_object_object_get(fat,"videoid");
	json_state = json_object_object_get(fat,"state");

	cli_usrname = (char *)json_object_get_string(json_usrname);
	cli_housenum = (char *)json_object_get_string(json_housenum);
	cli_telenum = (char *)json_object_get_string(json_telenum);
	cli_videoid = json_object_get_int(json_videoid);		//changed by xkx
	state = json_object_get_int(json_state);

	printf("usrname:%s\nhousenum:%s\ntelenum:%s\nvideoid:%d\nstate:%d\n",
	cli_usrname, cli_housenum,cli_telenum,cli_videoid,state);

	memset(cli_info,0,sizeof(*cli_info));//add by xkx

	strncpy(cli_info->usrname,cli_usrname,strlen(cli_usrname));
	strncpy(cli_info->housenum,cli_housenum,strlen(cli_housenum));
	strncpy(cli_info->telenum,cli_telenum,strlen(cli_telenum));
	
	cli_info->videoid = cli_videoid;				//changed by xkx
	
	cli_info->state = state;

}


/*json字符串打包函数 将结构体转换成json字符串*/
void json_string_pack(char *json_string,struct client_info *cli_info){

	    struct json_object *fat,*json_usrname,*json_housenum,*json_telenum,*json_videoid;
		struct json_object *json_state;

		//创建JSON对象
		fat = json_object_new_object();

		//将不同类型数据转换成json对应的类型
		json_usrname = json_object_new_string((const char *)cli_info->usrname);
		json_housenum = json_object_new_string((const char *)cli_info->housenum);
		json_telenum = json_object_new_string((const char *)cli_info->telenum);
		json_videoid = json_object_new_int(cli_info->videoid);
		json_state = json_object_new_int(cli_info->state);


		//将json类型的数据加入到json对象中
		json_object_object_add(fat,"USRNAME",json_usrname);
		json_object_object_add(fat,"HOUSENUM",json_housenum);
		json_object_object_add(fat,"TELENUM",json_telenum);
		json_object_object_add(fat,"STATE",json_state);
		json_object_object_add(fat,"VIDEOID",json_videoid);


		//将json对象转换成字符串
		json_string = (char *)json_object_to_json_string(fat);
		printf("json_string=%s\n",json_string);

}


/*新json字符串打包函数 将结构体转换成json字符串*/
void json_string_pack2(char *json_string,char *housenum,char *register_string,char *login,int videoid,int state){

	    struct json_object *fat,*json_housenum,*json_videoid,*json_register,*json_login;
		struct json_object *json_state;

		const char *buf = NULL;

		//创建JSON对象
		fat = json_object_new_object();

		//将不同类型数据转换成json对应的类型
		json_register = json_object_new_string(register_string);
		json_housenum = json_object_new_string(housenum);
		json_login = json_object_new_string(login);
		json_videoid = json_object_new_int(videoid);
		json_state = json_object_new_int(state);

		//将json类型的数据加入到json对象中
		json_object_object_add(fat,"housenum",json_housenum);
		json_object_object_add(fat,"register",json_register);
		json_object_object_add(fat,"login",json_login);
		json_object_object_add(fat,"state",json_state);
		json_object_object_add(fat,"videoid",json_videoid);

		//将json对象转换成字符串
		buf = json_object_to_json_string(fat);

		//在接收新的json字符串之前得先清空
		bzero(json_string,strlen(json_string));

		strncpy(json_string,buf,strlen(buf));
		
		//printf("json_string=%s len=%d\n",json_string,strlen(json_string));

}

