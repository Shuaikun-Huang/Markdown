#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include "wrap.h"
#define SERVER_STRING "Server: hskhttp/0.1.0\r\n"

void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

void cannot_execute(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}



void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    //set cookie
    sprintf(buf, "Set-Cookie: name=\"HSK\"; expires= Thu, 17 OCT 2019 11:00:00 GMT\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}


void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}



void unimplemented(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

void catfile(int cfd, const char *filename) {
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024];

	buf[0] = 'A'; buf[1] = '\0';
	// 从client中读取,直到遇到两个换行(起始行start line和首部header的间隔)
	while ((numchars > 0) && strcmp("\n", buf)){  //read & discard headers
		numchars = Readline(cfd, buf, sizeof(buf));
	}
	
	resource = fopen(filename, "r");
	if (resource == NULL)
		not_found(cfd);  // 404错误
	else{
		headers(cfd, filename);  // 向客户端回写200 OK
		//得到文件内容，发送
        fgets(buf, sizeof(buf), resource);
        while (!feof(resource)) {
            send(cfd, buf, strlen(buf), 0);
            fgets(buf, sizeof(buf), resource);
        }
	}
	fclose(resource);
}

void execCGI(int cfd, const char *path, const char *method, const char *query_string) {
	char    buf[1024];
	int     cgi_output[2];  // CGI程序输出管道
	int     cgi_input[2];   // CGI程序输入管道

	pid_t   pid;
	int     status;
	int     i;
	char    c;
	int     numchars = 1;
	int     content_length = -1;

	buf[0] = 'A'; buf[1] = '\0';

	if (strcasecmp(method, "GET") == 0){
		// 从client中读取,直到遇到两个换行
		while ((numchars > 0) && strcmp("\n", buf)) 
			numchars = Readline(cfd, buf, sizeof(buf));
	}
	else { 
		// 获取请求主体的长度
		numchars = Readline(cfd, buf, sizeof(buf));
		while ((numchars > 0) && strcmp("\n", buf)) {
			buf[15] = '\0';
			if (strcasecmp(buf, "Content-Length:") == 0){
				content_length = atoi(&(buf[16]));
			}
			numchars = Readline(cfd, buf, sizeof(buf));
		}
		if (content_length == -1) {
			bad_request(cfd);
			return;
		}
	}

	// 向client回写200 OK
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(cfd, buf, strlen(buf), 0);

	// 创建两个匿名管道
	if (pipe(cgi_output) < 0) {
		cannot_execute(cfd);
		return;
	}
	if (pipe(cgi_input) < 0) {
		cannot_execute(cfd);
		return;
	}

	// 创建子进程,去执行CGI程序
	if ( (pid = fork()) < 0 ) {
		cannot_execute(cfd);
		return;
	}
	if (pid == 0) { /* child: CGI script */
		char meth_env[255];
		char query_env[255];
		char length_env[255];

		dup2(cgi_output[1], 1); //复制cgi_output[1](读端)到子进程的标准输出
		dup2(cgi_input[0], 0);  //复制cgi_input[0](写端)到子进程的标准输入
		close(cgi_output[0]);   //关闭多余文件描述符
		close(cgi_input[1]);

		sprintf(meth_env, "REQUEST_METHOD=%s", method);
		putenv(meth_env);   // 添加一个环境变量

		if (strcasecmp(method, "GET") == 0) {
			sprintf(query_env, "QUERY_STRING=%s", query_string);
			putenv(query_env);
		}
		else {   /* POST */
			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
			putenv(length_env);
		}
		// 执行CGI程序
		execl(path, path, NULL);
		exit(0);    // 子进程退出
	}
	else {    /* parent */
		close(cgi_output[1]);   // 关闭cgi_output读端
		close(cgi_input[0]);    // 关闭cgi_input写端

		if (strcasecmp(method, "POST") == 0){
			// 请求类型为POST的时候,将POST数据包的主体entity-body部分
			// 通过cgi_input[1](写端)写入到CGI的标准输入
			for (i = 0; i < content_length; i++) {
				recv(cfd, &c, 1, 0);
				write(cgi_input[1], &c, 1);
			}
		}
		// 读取CGI的标准输出,发送到客户端
		while (read(cgi_output[0], &c, 1) > 0){
			send(cfd, &c, 1, 0);
		}
		// 关闭多余文件描述符
		close(cgi_output[0]);
		close(cgi_input[1]);
		// 等待子进程结束
		waitpid(pid, &status, 0);
	}
}

void disposal(int cfd, char *buf, int len){
    char method[255];
    char url[255];
    char path[512];
    int i = 0, j = 0;
    struct stat st;

    int cgi = 0;  //为真时，服务器执行CGI程序
    char *query_string = NULL; //保存get请求时的请求参数

    //取出请求方法
    while (!isspace((int)buf[i]) && (i < sizeof(method) - 1)) { //不为空，以及未到达上限，将buf中Get或Post拷贝进method
        method[i] = buf[i];
        i++;
    }
    j=i;
    method[i] = '\0';

    // 如果是POST请求,调用cgi程序
    if (strcasecmp(method, "POST") == 0) cgi = 1;  

    i = 0;
    while (isspace((int)buf[j]) && (j < len))  //跳过空格
        j++;

    //取出报文头的url，该url不包含host
    while (!isspace((int)buf[j]) && (i < sizeof(url) - 1) && (j < len)) {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

	if (strcasecmp(method, "GET") == 0)
	{
		query_string = url; // 为GET请求,查询语句为url
		// 如果查询语句中含义'?',查询语句为'?'字符后面部分
		while ((*query_string != '?') && (*query_string != '\0')){
			query_string++;
		}
		if (*query_string == '?') {
			cgi = 1;
			*query_string = '\0';   // 从'?'处截断,前半截为url
			query_string++;
		}
	}

    //生成本地路径
    sprintf(path, "host%s", url);   //path = /host/url
    if (path[strlen(path) - 1] == '/')   //如果url最后是以/结束，则默认在 path 中加上 index.html，表示访问主页
        strcat(path, "index.html");   //path+“index.heml”
    
    //判断请求资源的状态
    if (stat(path, &st) == -1) {
        // 从client中读取,直到遇到两个换行(起始行startline和首部header之间间隔)
        while ((len > 0) && strcmp("\n", buf))  // read & discard headers， \n ASCLL为10，buf[0] < 10 为真，针对空格，\t
            len = Readline(cfd, buf, sizeof(buf));   
        not_found(cfd);   //Give a client a  404 not found status message
    }
    else {
        //查看文件类型，如果类型是目录，就默认指向首页
        if ((st.st_mode & S_IFMT) == S_IFDIR)   
            strcat(path, "/index.html");

        // 请求的是一个可执行文件,作为CGI程序
        if ((st.st_mode & S_IXUSR) || 
            (st.st_mode & S_IXGRP) || 
            (st.st_mode & S_IXOTH))   //如果每个组都有执行权限
                cgi = 1;

        if(!cgi) catfile(cfd, path); //返回一个文件内容给客户端
        else execCGI(cfd, path, method, query_string); //执行cgi程序
    }
}
