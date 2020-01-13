#ifndef __HTTP_H_
#define __HTTP_H_


void bad_request(int); // 无法处理请求,回写400到client
void cannot_execute(int); // 不可以执行CGI程序,回写500到client
void headers(int, const char *); // 返回200 OK给客户端
void not_found(int); // 返回404状态给客户端
void unimplemented(int); // 返回501状态给客户端
void disposal(int cfd, char *buf, int len); //http回应
void execCGI(int, const char *, const char *, const char *);// 执行cgi程序
void catfile(int cfd, const char* filename);// 发送文件内容到client
#endif