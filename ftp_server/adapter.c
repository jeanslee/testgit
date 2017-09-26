#include <stdio.h>
#include <string.h>
#include "ftp_server.h"
#include "adapter.h"

typedef struct _ag_data_type
{
	char file_name[256];
	char reply[256];
	int rest_offset;
	int pasv_data_sock;
} ag_data_type;

ag_data_type ag_data;

char* process_user_cmd(char* user_cmd)
{
	char* pos_start = NULL;
	char* pos_end = NULL;
	pos_start = strchr(user_cmd, ' ');
	pos_start++;
	pos_end = strstr(user_cmd, "\r\n");
	if (pos_end)
	{
		*pos_end = '\0';
	}
	return pos_start;
}

int combine(char* left, char* right, char* result)
{
	char* left_last;
	if (!result || !left || !right)
	{
		return (-1);
	}
	strcpy(result, left);
	left_last = strrchr(left, '/');
	if (!left_last)
	{
		strcat(result, "/");
	}
	else if ((left_last - left + 1) < strlen(left))
	{
		strcat(result, "/");
	}
	else
	{
	}
	strcat(result, right);
	return (0);
}

// process the file name stored in ag_data 
int get_file_location(struct usr_info* p_usr)
{
	char file_location[256];
	char* pos_start = ag_data.file_name;
	if (*pos_start == '/')
	{
		pos_start++;
		combine(p_usr->home_dir, pos_start, file_location);
	}
	else
	{
		combine(p_usr->curr_dir, pos_start, file_location);
	}
	memset(ag_data.file_name, 0, 256);
	strcpy(ag_data.file_name, file_location);
//	printf("home: %s\ncur: %s\nlocation: %s\n", p_usr->home_dir, p_usr->curr_dir, ag_data.file_name);
	return (0);
	
}

void adapter(char* client_cmd, int cmd_type)
{
	char user_cmd[272];
	strcpy(user_cmd, client_cmd);
	switch (cmd_type)
	{
	case STOR_CMD:
		memset(ag_data.file_name, 0, 256);
		strcpy(ag_data.file_name, process_user_cmd(user_cmd));
		break;
	case RETR_CMD:
		memset(ag_data.file_name, 0, 256);
		strcpy(ag_data.file_name, process_user_cmd(user_cmd));
		break;
	case REST_CMD:
		ag_data.rest_offset = atoi(process_user_cmd(user_cmd));
		memset(ag_data.reply, 0, 256);
		sprintf(ag_data.reply, "350 Restarting at %d.\r\n", ag_data.rest_offset);
		break;
	default:
		break;
	}
}

void clean_file_name()
{
	memset(ag_data.file_name, 0, 256);
}

void clean_rest_offset()
{
	ag_data.rest_offset = 0;
}

char* get_file_name()
{
	return ag_data.file_name;
}

int get_rest_offset()
{
	return ag_data.rest_offset;
}

void clean_pasv_sock()
{
	ag_data.pasv_data_sock = -1;
}

int get_pasv_sock()
{
	return ag_data.pasv_data_sock;
}

void set_pasv_sock(int sockfd)
{
	ag_data.pasv_data_sock = sockfd;
}

char* get_rest_reply()
{
	return ag_data.reply;
}
