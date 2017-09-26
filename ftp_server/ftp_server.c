#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h> 
#include <dirent.h>

#define _XOPEN_SOURCE
#include <unistd.h>


#include "config.h"
#include "ftp_server.h"
#include "data_trans.h"
#include "adapter.h"

extern int h_errno;

struct sockaddr_in server, client;

void error_exit(char * err_msg, int err_code)
{
	printf("%s\n", err_msg);
	exit(err_code);
}

int check_usr_passwd(struct usr_info * my_usr)
{
	char sys_usr_info[512];
	char pass[35];
	char salt[13];
	char * get_salt;
	FILE * fp_usrinfo;
	int pass_len;
	char *crypt_offset, *home_offset, *level_offset;
	if(my_usr->usr[0] == '\0')
		return -1;
	fp_usrinfo = fopen(USRINFO, "r"); //read usr's infomation from system confige file
	if(fp_usrinfo == NULL)
	{
		error_exit("fopen() error!", 600);
	}
	while(fgets(sys_usr_info, sizeof(sys_usr_info), fp_usrinfo))
	{
		if(strlen(my_usr->usr) < 63)
		{
			if(sys_usr_info[strlen(my_usr->usr)] != ':')
				continue;
		}
		if(strncmp(my_usr->usr, sys_usr_info, strlen(my_usr->usr)))
		{
			continue;
		}
		else
		{
			if(my_usr->passwd[0] == '\0')
			{
				fclose(fp_usrinfo);
				return 1;
			}
			get_salt = strstr(sys_usr_info, "$");
			strncpy(salt, get_salt, sizeof(salt));
			strncpy(pass, get_salt, sizeof(pass));
			salt[12] = '\0';
			pass[34] = '\0';
			pass_len = strlen(pass);
			if(!strncmp(pass, (char *)crypt(my_usr->passwd, salt), pass_len))
			{
				crypt_offset = strchr(sys_usr_info, ':');
				home_offset = strchr(crypt_offset+1, ':');
				level_offset = strchr(home_offset+1, ':');
				*level_offset = '\0';
				strncpy(my_usr->home_dir, home_offset+1, strlen(home_offset)-1);
				strncpy(my_usr->curr_dir, home_offset+1, strlen(home_offset)-1);
				my_usr->level = atoi(level_offset+1);
				chdir(my_usr->home_dir);
				fclose(fp_usrinfo);
				return 2;
			}
			else
			{
				fclose(fp_usrinfo);
				return 3;

			}
		}
	}
	fclose(fp_usrinfo);
	return 0;
}

void server_pwd(int control_fd, struct usr_info * my_usr)
{
        char curr_dir[512];
	char * usr_tmp_dir;
	char usr_now_dir[512];
        int size = sizeof(curr_dir);
        if(getcwd(curr_dir, size) == NULL)
                send_reply("550 command PWD failed", NULL, control_fd);
        else
	{
		usr_tmp_dir = strstr(curr_dir,my_usr->curr_dir);
		snprintf(usr_now_dir, sizeof(usr_now_dir), "257 \"%s", usr_tmp_dir + strlen(my_usr->home_dir));
		strncat(usr_now_dir, "/\" ", 3);
		
                send_reply(usr_now_dir, "is current directory.", control_fd);
	}
}

void server_cwd(int control_fd, struct usr_info * my_usr, char * usr_cmd)
{
	char ch_dir[512];
	char * dir_tmp;
	memset(ch_dir, '\0', sizeof(ch_dir));
	if(usr_cmd[4] == '/')
		strncpy(ch_dir, my_usr->home_dir,strlen(my_usr->home_dir));
	else
		strncpy(ch_dir, my_usr->curr_dir, strlen(my_usr->curr_dir));
	strncat(ch_dir, "/", 1);
	strncat(ch_dir, usr_cmd + 4, strlen(usr_cmd + 4));
	strncat(ch_dir, "/", 1);
	if(chdir(ch_dir) < 0)
	{
		send_reply("550 ", "change directory error", control_fd);
	}
	else
	{
		char curr_dir[512];
        	int size = sizeof(curr_dir);
		if(getcwd(curr_dir, size) == NULL)
			send_reply("550 ", "you can't open this directory.",control_fd);
		else
		{
			if((dir_tmp = strstr(curr_dir, my_usr->home_dir)) == NULL)
			{
				send_reply("550 ", "you can't open this directory.",control_fd);
				chdir(my_usr->curr_dir);
			}
			else
			{
				send_reply("250 ", "CWD command successful.", control_fd);
				memset(my_usr->curr_dir, '\0', sizeof(my_usr->curr_dir));
				strncpy(my_usr->curr_dir, curr_dir, strlen(curr_dir));
			//	printf("my_usr->curr_dir: %s\n", my_usr->curr_dir);
			}
		}
	}
}

void server_list(int control_fd, struct usr_info * my_usr, int mode, char * client_ip, int client_port)
{
	DIR * dp;
	struct dirent * dirp;
	int sock_data =	get_data_sock(mode, client_ip, client_port, control_fd);
	if((dp = opendir(my_usr->curr_dir)) == NULL)
	{
		send_reply(" ", "opendir() error!", control_fd);
		return;
	}
	else
	{
		send_reply("150 ","Opening BINARY mode data connection.", control_fd);	
	}
	while((dirp = readdir(dp)) != NULL)
	{
		if(strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0)
		{
			send_reply(dirp->d_name, NULL, sock_data);
		}
	}
	send_reply("226 ", "Transfer complete.", control_fd);
	close(sock_data);
}

void server_size(char * usr_cmd, int control_fd)
{
	struct stat file_info;
	int local_file;
	char * check_file;
	char filename[512];
	memset(filename, '\0', sizeof(filename));
	check_file = strstr(usr_cmd, " ");
	strncpy(filename, check_file+1, strlen(check_file+1));
	if(!stat(filename, &file_info))
	{
		char file_size[32];
		sprintf(file_size, "%d", file_info.st_size);
		send_reply("213 ", file_size, control_fd);
	}
	else
	{
		send_reply("550 ", "No such file or directory.", control_fd);
	}
}

int check_usr_cmd(char * usr_cmd)
{
	if(!strncmp(usr_cmd, "LIST", 4))
		return 1;
	if(!strncmp(usr_cmd, "PWD", 3))
		return 2;
	if(!strncmp(usr_cmd, "CWD ",4))
		return 3;
	if(!strncmp(usr_cmd, "STOR ",4))
	{
		adapter(usr_cmd, STOR_CMD);
		return 4;
	}
	if(!strncmp(usr_cmd, "RETR", 4))
	{
		adapter(usr_cmd, RETR_CMD);
		return 5;
	}
	if(!strncmp(usr_cmd, "REST", 4))
	{
		adapter(usr_cmd, REST_CMD);
		return 45;
	}
	if(!strncmp(usr_cmd, "QUIT", 4))
		return 6;
	if(!strncmp(usr_cmd, "SIZE ", 5))
		return 7;
	if(!strncmp(usr_cmd, "TYPE ", 5))
		return 8;
	if(!strncmp(usr_cmd, "USER ", 5))
		return 11;
	if(!strncmp(usr_cmd, "PASS ", 5))
		return 12;
	if(!strncmp(usr_cmd, "PASV", 4))
		return 21;
	if(!strncmp(usr_cmd, "PORT", 4))
		return 22;
	return -1;
}

int send_reply(char * s1, char * s2, int sock_fd)
{
	char send_buf[256];
	int send_err, len;
	if(s1) 
	{
		strcpy(send_buf,s1);
		if(s2)
		{	
			strcat(send_buf, s2);
			strcat(send_buf,"\r\n");
			len = strlen(send_buf);
			send_err = send(sock_fd, send_buf, len, 0);
		}
		else 
		{
			strcat(send_buf,"\r\n");
			len = strlen(send_buf);
			send_err = send(sock_fd, send_buf, len, 0);
		}
    	}
	if(send_err < 0)
		printf("send() error!\n");
	return send_err;
}

void do_log(char * mesg)
{
	syslog(LOG_INFO, "%s", mesg);
	closelog();
}

int local_is_dir(char* dirname)
{
	struct stat buf;
	stat(dirname, &buf);
	return S_ISDIR(buf.st_mode);
}

int client_process(int connect_fd, struct sockaddr_in client)
{
	
	struct usr_info my_usr;

	char rcv_buf[512];
	char err_buf[5];
	char port_client_ip[16];
	int port_client_port;
	int count = 0;
	int cmd_type, login = 0;
	int i;
	int mode = PASV;
	int type = TYPEI;
	char log[512];
	char *ip1, *ip2, *ip3, *ip4,*port_h, *port_l;

	snprintf(log, sizeof(log), "connection from %s", inet_ntoa(client.sin_addr));
	do_log(log);
	
	memset(&(my_usr), '\0', sizeof(my_usr));
	send_reply("220 ", "FTP server Ready!", connect_fd);
	while(1)
	{
		for(i = 0; i < 512; i++)
		{
			rcv_buf[i] ='\0';
		}
		count = read(connect_fd, rcv_buf, 510);
		if(count <= 0)
			break;
		rcv_buf[strlen(rcv_buf)-2] = '\0';
		cmd_type = check_usr_cmd(rcv_buf);
		switch(cmd_type)
		{
			case 1:
				if(!login)
				{
					send_reply("550 ", "Please login first!", connect_fd);
					break;
				}
				server_list(connect_fd,&my_usr, mode, port_client_ip, port_client_port);
				break;
			case 2:
				if(!login)
				{
					send_reply("550 ", "Please login first!", connect_fd);
					break;
				}
				server_pwd(connect_fd, &my_usr);
				break;
			case 3:
				if(!login)
				{
					send_reply("550 ", "Please login first!", connect_fd);
					break;
				}
				server_cwd(connect_fd, &my_usr, rcv_buf);
				break;
			case 4:
				if(!login)
				{
					send_reply("550 ", "Please login first!", connect_fd);
					break;
				}
				if (my_usr.level < 2)
				{
					send_reply("550 ", "Permission denied", connect_fd);
				}
				else
				{
					get_file_location(&my_usr);
					store_file(get_file_name(), get_rest_offset(), get_data_sock(mode, port_client_ip, port_client_port, connect_fd), connect_fd);
				}
				break;
			case 5:
			if(!login)
			{
				send_reply("550 ", "Please login first!", connect_fd);
				break;
			}
			if (my_usr.level != 1 && my_usr.level != 3)
			{
				send_reply("550 ", "Permission denied.", connect_fd);
				break;
			}
			get_file_location(&my_usr);
			if (local_is_dir(get_file_name()))
			{
				send_reply("550 ", "Target is directory.", connect_fd);
				break;
			}

			retr_file(get_file_name(), get_rest_offset(), get_data_sock(mode, port_client_ip, port_client_port, connect_fd), connect_fd);

    break;
			case 45:
				if(!login)
				{
					send_reply("550 ", "Please login first!", connect_fd);
					break;
				}
				send_reply(get_rest_reply(), NULL, connect_fd);
				break;
			case 6:
				if(!login)
				{
					send_reply("550 ", "Please login first!", connect_fd);
					break;
				}
				send_reply("221 ", "Goodbye~", connect_fd);
				close(connect_fd);
				snprintf(log,sizeof(log),"user %s from %s, quit",\
					 my_usr.usr, inet_ntoa(client.sin_addr));
				do_log(log);
				exit(0);
			case 7:
				if(!login)
				{
					send_reply("550 ", "Please login first!", connect_fd);
					break;
				}
				server_size(rcv_buf, connect_fd);
				break;
			case 8:
				if(!login)
				{
					send_reply("550 ", "Please login first!", connect_fd);
					break;
				}
				if(rcv_buf[6] == 'A')
				{
					type = TYPEA;
					send_reply("200 ", "TYPE set to A", connect_fd);
				}
				else
				{
					type =  TYPEI;
					send_reply("200 ", "TYPE set to I", connect_fd);
				}
				break;
			case 11:
				memset(&(my_usr.usr), '\0', sizeof(my_usr.usr));
				snprintf(my_usr.usr, strlen(rcv_buf) - 4, rcv_buf+5);
				if(check_usr_passwd(&my_usr) == 1)
				{
					send_reply("331 ", "username OK, please send your password" ,connect_fd);
				}
				else
				{	
					send_reply("332 ","username ERROR!", connect_fd);
				
				}
				break;
			case 12:
				memset(&(my_usr.passwd), '\0', sizeof(my_usr.passwd));
				snprintf(my_usr.passwd, strlen(rcv_buf) - 4, rcv_buf+5);
				if(check_usr_passwd(&my_usr) == 2)
				{
					send_reply("230 ", "User login!" ,connect_fd);
					snprintf(log,sizeof(log),"user %s from %s, login",\
						 my_usr.usr, inet_ntoa(client.sin_addr));
					do_log(log);
					login = 1;
				}
				else
				{
					send_reply("530 ","password ERROR!", connect_fd);	
					snprintf(log,sizeof(log),"user %s from %s, password error",\
						 my_usr.usr, inet_ntoa(client.sin_addr));
					do_log(log);
				
				}
				break;
			case 21:
				mode = PASV;
				open_pasv_mode(connect_fd);
				break;
			case 22:
				ip1 = strchr(rcv_buf, ' ');
				ip1 ++;
				ip2 = strchr(ip1, ',');
				*ip2 = '\0';
                        	ip2++;
	                        ip3 = strchr(ip2, ',');
        	                *ip3 = '\0';
                	        ip3++;
                        	ip4 = strchr(ip3, ',');
	                        *ip4 = '\0';
        	                ip4++;
				port_h = strchr(ip4, ',');
				*port_h = '\0';
				port_h ++;
				port_l =  strchr(port_h, ',');
				*port_l = '\0';
				port_l ++;
				port_client_port = atoi(port_h)*256 + atoi(port_l);
                		snprintf(port_client_ip, sizeof(port_client_ip), \
					"%s.%s.%s.%s", ip1, ip2, ip3, ip4);
				if(strncmp(port_client_ip, (char *)inet_ntoa(client.sin_addr),strlen(port_client_ip)))
				{
					send_reply("500 Illegal PORT command: ", NULL, connect_fd);
				}
				mode = PORT;
				send_reply("200 ", "PORT command successful", connect_fd);
				break;
			default:
				snprintf(err_buf, 4, rcv_buf);
				send_reply("500 Illegal command: ", err_buf, connect_fd);
		}
	}

	return 1;	
}

int main(int argc, char * argv[])
{
	int check, opt, listen_port;
	int daemon_pid, client_pid;
	int listen_fd, connect_fd;
	int sin_size = sizeof(struct sockaddr_in);
	check = 0;

	//creat daemon process
	daemon_pid = fork();
	switch(daemon_pid)
	{
		case 0:
			break;
		case -1:
			error_exit("fork() error!", 1);
		default:
			exit(0); //parent goes bye-bye :-)
	}

	//child process continues
	setsid();
	chdir(WORK_HOME);
	umask(0);

	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		error_exit("Creating socket failed.", 600);

	opt = SO_REUSEADDR;
	if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))!= 0)
	{
		printf("setsockopt() %s\n", strerror(errno));
	}
	
	//fill the server information
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	if(argv[1] != NULL)
	{
		listen_port = atoi(argv[1]);
		server.sin_port = htons(listen_port);
	}
	else
		server.sin_port = htons(CONTROL_PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listen_fd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
	{
		printf("%s\n", strerror(errno));
		error_exit("bind() error!\n", 601);
	}
	if(listen(listen_fd, MAX_LOGIN) == -1)
		error_exit("listen() error", 602);
	
	do_log("Start ftp_server.");
	while(1)
	{
		connect_fd = accept(listen_fd, (struct sockaddr *)&client, (socklen_t *)&sin_size);
		if(connect_fd == -1)
		{
			printf("accept() error: %s %d\n", strerror(errno), errno);
			continue;
		}
		if((client_pid = fork()) > 0)
		{
			close(connect_fd);
			continue;
		}
		else 
		{
			if(client_pid == 0)
			{
				close(listen_fd);
				client_process(connect_fd, client);
				exit(0);
			}
			else
			{
				printf("fork() error!");
				close(connect_fd);
				continue;
			}
		}
	}
	return 1;
}
