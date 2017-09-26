#ifndef _ADAPTER_H
#define _ADAPTER_H


#define STOR_CMD 0
#define RETR_CMD 1
#define REST_CMD 2


void adapter(char* user_cmd, int cmd_type);

void clean_file_name();

void clean_rest_offset();

char* get_file_name();

int get_rest_offset();

void clean_pasv_sock();
	
int get_pasv_sock();

void set_pasv_sock(int sockfd);

char* get_rest_reply();

#endif
