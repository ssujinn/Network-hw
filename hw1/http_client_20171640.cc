#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 1000

int main(int argc, char *argv[]) {
	int sockfd;
	int numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo;
	int rv;
	char s[INET_ADDRSTRLEN];

	// lab4
	fd_set master, read_fds;

	// hw1
	char http[10];
	char host[100];
	char port[10];
	char path[500];
	char status[100];
	FILE *fp;
	int datasize;


	if (argc != 2){
		fprintf(stderr, "usage: http_client http://hostname[:port][/path/to/file]\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// memory set to 0
	memset(http, 0, sizeof http);
	memset(host, 0, sizeof host);
	memset(port, 0, sizeof port);
	memset(path, 0, sizeof path);
	memset(buf, 0, sizeof buf);
	memset(status, 0, sizeof status);
	
	// not "http://" 
	strncpy(http, argv[1], 7);
	if (strcmp(http, "http://")){
		fprintf(stderr, "usage: http_client http://hostname[:port][/path/to/file]\n");
		exit(1);
	}

	// argv parsing (host, port, path)
	int idx;
	for (int i = 7, j = 0; argv[1][i] != '/' && argv[1][i] != '\0'; i++, j++){
		host[j] = argv[1][i];
		idx = i;
	}

	if (argv[1][++idx] == '/'){
		for (int i = idx, j = 0; argv[1][i] != '\0'; i++, j++){
			path[j] = argv[1][i];
		}
	}

	int port_flag = 0;
	for (int i = 0, j = 0; i < strlen(host); i++){
		if (port_flag)
			port[j++] = host[i];
		if (host[i] == ':'){
			port_flag = 1;
			idx = i;
		}
	}

	if (port_flag)
		memset(host + idx, '\0', sizeof (host + idx));
	else
		strcpy(port, "80");

	if (path[0] == 0)
		path[0] = '/';

	 //printf("host: %s / port: %s / path: %s\n", host, port, path);

	// get IP address
	if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// create socket
	if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1){
		perror("socket");
		return 2;
	}

	// connect to server
	if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		close(sockfd);
		perror("connect");
		return 2;
	}

	
	// print to screen
	inet_ntop(servinfo->ai_family, &((struct sockaddr_in*)servinfo->ai_addr)->sin_addr, s, sizeof s);
	// printf("connecting to %s\n", s);

	// free allocated memory for servinfo
	freeaddrinfo(servinfo);

	// write http request message
	sprintf(buf, "GET %s HTTP/1.1\r\nHost: %s:%s\r\n\r\n", path, host, port);

	// send message to server
	if (send(sockfd, buf, strlen(buf), 0) == -1) {
		perror("send");
		close(sockfd);
		exit(1);
	}

	// open file to write data
	fp = fopen("20171640.out", "w");

	// recv message from server
	memset(buf, 0, sizeof buf);
	if ((numbytes = recv(sockfd, buf, sizeof buf, 0)) == -1) {
		perror("recv");
		close(sockfd);
		exit(1);
	}

	buf[strlen(buf)] = '\0';
	if (strlen(buf) > MAXDATASIZE)
		buf[MAXDATASIZE] = '\0';



	// print satus
	memset(status, 0, sizeof status);
	for (int i = 0; i < strlen(buf); i++){
		if (buf[i] == '\r' && buf[i + 1] == '\n'){
			idx = i;
			break;
		}
	}
	memcpy(status, buf, idx);
	printf("%s\n", status);

	// save data
	char header[MAXDATASIZE];
	char data[MAXDATASIZE];
	memset(header, 0, sizeof header);
	memset(data, 0, sizeof data);
	for (int i = 0; i < strlen(buf); i++){
		if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n'){
		idx = i + 4;
		break;
	}
		else
		header[i] = buf[i];
	}
	strcpy(data, buf + idx);

	// change to lowercase
	int length_flag = 0;
	for (int i = 0; i < strlen(header); i++){
	if (header[i] >= 65 && header[i] <= 90)
		header[i] += 32;
	}
	// read content-length
	char len[100];
	char tmp[100];
	int idx1, idx2;
	int len_flag = 0;
	for (int i = 0; i < strlen(header); i++){
		if (header[i] == '\r' && header[i + 1] == '\n'){
			idx1 = i + 2;
			if (len_flag) break;
		}
		if (header[i] == ':'){
			idx2 = i;
			memset(tmp, 0, sizeof(tmp));
			strncpy(tmp, header + idx1, idx2 - idx1);
			if (!strcmp(tmp, "content-length")){
				memset(len, 0, sizeof len);
				idx2++;
				if (header[idx2] == ' ') idx2++;
				len_flag = 1;
			}
		}
	}


	if (len_flag){
		if (idx1 > idx2){
			strncpy(len, header + idx2, (idx1 - 2) - idx2);
			printf("%s bytes written to 20171640.out\n", len);

			// write data
			if (strlen(data) != 0) data[strlen(data)] = '\0';
			fprintf(fp, "%s", data);
			datasize = strlen(data);
			//printf("datasize %d", datasize);


			while (atoi(len) > datasize + MAXDATASIZE){
				// recv from server
				memset(buf, 0, sizeof buf);
				if ((numbytes = recv(sockfd, buf, sizeof buf, 0)) == -1) {
					perror("recv");
					close(sockfd);
					exit(1);
				}
				if (strlen(buf) == 0) break;

				buf[strlen(buf)] = '\0';
				if (strlen(buf) > MAXDATASIZE)
					buf[MAXDATASIZE] = '\0';
				fprintf(fp, "%s", buf);
				datasize += strlen(buf);

				if (strlen(buf) == 0)
					break;
			}

		
			if (atoi(len) > datasize){	
				memset(buf, 0, sizeof buf);
				if ((numbytes = recv(sockfd, buf, sizeof buf, 0)) == -1) {
					perror("recv");
					close(sockfd);
					exit(1);
				}
				buf[strlen(buf)] = '\0';
				if (strlen(buf) > MAXDATASIZE)
					buf[MAXDATASIZE] = '\0';
				fprintf(fp, "%s", buf);

			}


		}
		else
			printf("Content-Length not specified\n");
	}
	else
		printf("Content-Length not specified.\n");
	
	
	// printf("connection closed.\n");
	close(sockfd);
	
	// close file
	fclose(fp);

	return 0;
}
