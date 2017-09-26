#ifndef _DATA_TRANS_H
#define _DATA_TRANS_H

int s_send_reply(char* reply, int sockfd);

int file_copy(int srcfd, int destfd, int* psize);

int store_file(char* file_name, int rest_offset, int sock_data, int sock_control);
int retr_file(char* file_name, int rest_offset, int sock_data, int sock_control);

// change this function to make pasv mode
int rand_local_port();

int open_pasv_mode(int sock_control);

int open_port_mode(char* client_ip, int client_port);

int get_data_sock(int mode, char* client_ip, int client_port, int sock_control);

#endif
