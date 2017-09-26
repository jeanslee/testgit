#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "ftp_server.h"

#define BUFSIZE 1024


int s_send_reply(char* reply, int sockfd)
{
	int write_size = 0;
	if (!reply || -1 == sockfd)
	{
		return (-1);
	}
	write_size = write(sockfd, reply, strlen(reply));
	return ((write_size == -1) ? -1 : 0);
}

int file_copy(int srcfd, int destfd, int* psize)
{
	char* buf;
	char* ptr;
	int read_size = 0;
	int write_size = 0;
	*psize = 0;
	if (-1 == srcfd || -1 == destfd)
	{
		fprintf(stderr, "fd error");
		return (-1);
	}
	buf = (char*)malloc(sizeof(char) * BUFSIZE);
	memset(buf, 0, sizeof(char) * BUFSIZE);
	while (0 != (read_size = read(srcfd, buf, sizeof(char) * BUFSIZE)))
	{
		if (read_size == -1 && errno != EINTR)
		{
			break;
		}
		else if (read_size > 0)
		{
			ptr = buf;
			while (0 != (write_size = write(destfd, ptr, read_size)))
			{
				*psize += write_size;
				if (write_size == -1 && errno != EINTR)
				{
					break;
				}
				else if (write_size == read_size)
				{
					break;
				}
				else if (write_size > 0)
				{
					ptr += write_size;
					read_size -= write_size;
				}
			}
			if (write_size == -1)
			{
				break;
			}
		}
	}
	free(buf);
	return (0);
	
}

int store_file(char* file_name, int rest_offset, int sock_data, int sock_control)
{
	int filefd = -1;
	int get_size = 0;
	if (-1 == sock_data)
	{
		s_send_reply("425 Cannot open data connection.\r\n", sock_control);
		return (-1);
	}
	if (0 == access(file_name, F_OK))
		filefd = open(file_name, O_APPEND | O_WRONLY);
	else
		filefd = open(file_name, O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR | S_IRGRP | S_IROTH);
	if (-1 == filefd)
	{
		perror("open");
		s_send_reply("550 No such file or directory.\r\n", sock_control);
		close(sock_data);
		return (-1);
	}
	if (rest_offset > 0 && (-1 == (lseek(filefd, rest_offset, SEEK_SET))))
	{
		perror("lseek");
		s_send_reply("550 Cannot RESTart beyond end-of-file.\r\n", sock_control);
		close(sock_data);
		close(filefd);
		return (-1);
	}
	s_send_reply("150 Opening BINARY mode data connection.\r\n", sock_control);	
	file_copy(sock_data, filefd, &get_size);
	if (-1 == get_size)
	{
		s_send_reply("425 Cannot open data connection.\r\n", sock_control);
	}
	close(filefd);
	s_send_reply("226 Transfer complete.\r\n", sock_control);
	close(sock_data);
	clean_pasv_sock();	
	return (0);
}
int retr_file(char* file_name, int rest_offset, int sock_data, int sock_control)
{
	int filefd = -1;
	int get_size = 0;
	if (-1 == sock_data)
	{
		s_send_reply("425 Cannot open data connection.\r\n", sock_control);
		return (-1);
	}
	if (-1 == (filefd = open(file_name, O_RDONLY)))	
	{
		perror("open");
		s_send_reply("550 No such file or directory.\r\n", sock_control);
		close(sock_data);
		return (-1);
	}
	if (rest_offset > 0 && (-1 == (lseek(filefd, rest_offset, SEEK_SET))))
	{
		perror("lseek");
		s_send_reply("550 Cannot RESTart beyond end-of-file.\r\n", sock_control);
		close(sock_data);
		close(filefd);
		return (-1);
	}
	s_send_reply("150 Opening BINARY mode data connection.\r\n", sock_control);	
	file_copy(filefd, sock_data, &get_size);
	if (-1 == get_size)
	{
		s_send_reply("425 Cannot open data connection.\r\n", sock_control);
	}
	close(filefd);
	s_send_reply("226 Transfer complete.\r\n", sock_control);
	close(sock_data);
	clean_pasv_sock();	
	return (0);
}

// change this function to make pasv mode
int rand_local_port()
{
	int local_port;
	srand((unsigned)time(NULL));
	local_port = rand() % 40000 + 1025;
	return local_port;
}

int open_pasv_mode(int sock_control)
{
	int client_port, get_sock, connect_sock, opt, set;
	int sin_size = sizeof(struct sockaddr_in);
	char cmd_buf[256];
	struct timeval outtime;
	struct sockaddr_in local;
	struct sockaddr_in local_host;
	struct sockaddr_in client;
	char local_ip[24];
	char *ip_1, *ip_2, *ip_3, *ip_4;
	int addr_len =  sizeof(struct sockaddr);
	clean_rest_offset();
	client_port = rand_local_port();
	get_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(get_sock < 0)
	{
		exit(1);
	}

	//set outtime for the data socket
	outtime.tv_sec = 7;
	outtime.tv_usec = 0;
	opt = SO_REUSEADDR;
	set = setsockopt(get_sock, SOL_SOCKET,SO_RCVTIMEO, &outtime,sizeof(outtime));
	if(set !=0)
	{
		printf("set socket %s errno:%d\n",strerror(errno),errno);
		exit(1);
	}
	set = setsockopt(get_sock, SOL_SOCKET,SO_REUSEADDR, &opt,sizeof(opt));
	if(set !=0)
	{
		printf("set socket %s errno:%d\n",strerror(errno),errno);
		exit(1);
	}

	bzero(&local_host,sizeof(local_host));
	local_host.sin_family = AF_INET;
	local_host.sin_port = htons(client_port);
	local_host.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&local, sizeof(struct sockaddr));
	while(1)
	{
		set = bind(get_sock, (struct sockaddr *)&local_host, sizeof(local_host));
		if(set != 0 && errno == 11)
		{
			client_port = rand_local_port();
			continue;
		}
		set = listen(get_sock, 1);
		if(set != 0 && errno == 11)
		{
			perror("listen: ");
			exit(1);
		}
		//get local host's ip
		if(getsockname(sock_control,(struct sockaddr*)&local, (socklen_t *)&addr_len) < 0)
		{
			clean_pasv_sock();
			break;
		}
		snprintf(local_ip, sizeof(local_ip), (char*)inet_ntoa(local.sin_addr));
		//			strcpy(local_ip, (char*)inet_ntoa(local.sin_addr)); 
		//change the format to the PORT command needs.
		local_ip[strlen(local_ip)]='\0';
		ip_1 = local_ip;
		ip_2 = strchr(local_ip, '.');
		*ip_2 = '\0';
		ip_2++;
		ip_3 = strchr(ip_2, '.');
		*ip_3 = '\0';
		ip_3++;
		ip_4 = strchr(ip_3, '.');
		*ip_4 = '\0';
		ip_4++;
		snprintf(cmd_buf, sizeof(cmd_buf), \
				"227 Entering Passive Mode (%s,%s,%s,%s,%d,%d).\r\n", \
				ip_1, ip_2, ip_3, ip_4,	client_port >> 8, client_port&0xff);
		s_send_reply(cmd_buf, sock_control);
		while(1)
		{
			connect_sock = accept(get_sock, (struct sockaddr *)&client, (socklen_t *)&sin_size);        
			if(connect_sock == -1)
			{
				printf("accept() error: %s %d\n", strerror(errno), errno);
				continue;
			}
			break;
		}
		set_pasv_sock(connect_sock);
		break;
	}
	return get_pasv_sock();
}

int open_port_mode(char* client_ip, int client_port)
{
	int sockfd = -1;
	struct hostent* host_ent;
	struct sockaddr_in client_addr;
	if ((host_ent = gethostbyname(client_ip)) == NULL)
	{
		perror("gethostbyname");
		return (-1);
	}
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		return (-1);
	}
	bzero(&(client_addr), sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(client_port);
	client_addr.sin_addr = *((struct in_addr*)host_ent->h_addr);
	if (connect(sockfd, (struct sockaddr*)&client_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("connect");
		return (-1);
	}
	return sockfd;
}

int get_data_sock(int mode, char* client_ip, int client_port, int sock_control)
{
	if (mode == PORT)
	{
		return open_port_mode(client_ip, client_port);
	}
	else if (mode == PASV)
	{
		return get_pasv_sock();
	}
	else
	{
		return (-1);
	}
}


