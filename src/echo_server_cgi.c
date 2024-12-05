/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "parse.h"
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/select.h>
#define ECHO_PORT 9999
#define BUF_SIZE 20000


/**************** BEGIN CONSTANTS ***************/
#define FILENAME "/usr/bin/python3"


/* note: null terminated arrays (null pointer) */
char* ARGV[] = {
                    FILENAME,
                    "static_site/cgi_script.py",
                    NULL
               };

char* ENVP[] = {
                    "CONTENT_LENGTH=",
                    "CONTENT-TYPE=",
                    "GATEWAY_INTERFACE=CGI/1.1",
                    "QUERY_STRING=action=opensearch&search=HT&namespace=0&suggest=",
                    "REMOTE_ADDR=128.2.215.22",
                    "REMOTE_HOST=gs9671.sp.cs.cmu.edu",
                    "REQUEST_METHOD=GET",
                    "SCRIPT_NAME=/cgi/cgi_script.py",
                    "HOST_NAME=en.wikipedia.org",
                    "SERVER_PORT=80",
                    "SERVER_PROTOCOL=HTTP/1.1",
                    "SERVER_SOFTWARE=Liso/1.0",
                    "HTTP_ACCEPT=application/json, text/javascript, */*; q=0.01",
                    "HTTP_REFERER=http://en.wikipedia.org/w/index.php?title=Special%3ASearch&search=test+wikipedia+search",
                    "HTTP_ACCEPT_ENCODING=gzip,deflate,sdch",
                    "HTTP_ACCEPT_LANGUAGE=en-US,en;q=0.8",
                    "HTTP_ACCEPT_CHARSET=ISO-8859-1,utf-8;q=0.7,*;q=0.3",
                    "HTTP_COOKIE=clicktracking-session=v7JnLVqLFpy3bs5hVDdg4Man4F096mQmY; mediaWiki.user.bucket%3Aext.articleFeedback-tracking=8%3Aignore; mediaWiki.user.bucket%3Aext.articleFeedback-options=8%3Ashow; mediaWiki.user.bucket:ext.articleFeedback-tracking=8%3Aignore; mediaWiki.user.bucket:ext.articleFeedback-options=8%3Ashow",
                    "HTTP_USER_AGENT=Mozilla/5.0 (X11; Linux i686) AppleWebKit/535.1 (KHTML, like Gecko) Chrome/14.0.835.186 Safari/535.1",
                    "HTTP_CONNECTION=keep-alive",
                    "HTTP_HOST=http://127.0.0.1:9999",
                    NULL
               };

char* POST_BODY = "This is the stdin body...\n";
/**************** END CONSTANTS ***************/



/**************** BEGIN UTILITY FUNCTIONS ***************/
/* error messages stolen from: http://linux.die.net/man/2/execve */
void execve_error_handler()
{
    switch (errno)
    {
        case E2BIG:
            fprintf(stderr, "The total number of bytes in the environment \
(envp) and argument list (argv) is too large.\n");
            return;
        case EACCES:
            fprintf(stderr, "Execute permission is denied for the file or a \
script or ELF interpreter.\n");
            return;
        case EFAULT:
            fprintf(stderr, "filename points outside your accessible address \
space.\n");
            return;
        case EINVAL:
            fprintf(stderr, "An ELF executable had more than one PT_INTERP \
segment (i.e., tried to name more than one \
interpreter).\n");
            return;
        case EIO:
            fprintf(stderr, "An I/O error occurred.\n");
            return;
        case EISDIR:
            fprintf(stderr, "An ELF interpreter was a directory.\n");
            return;
        case ELIBBAD:
            fprintf(stderr, "An ELF interpreter was not in a recognised \
format.\n");
            return;
        case ELOOP:
            fprintf(stderr, "Too many symbolic links were encountered in \
resolving filename or the name of a script \
or ELF interpreter.\n");
            return;
        case EMFILE:
            fprintf(stderr, "The process has the maximum number of files \
open.\n");
            return;
        case ENAMETOOLONG:
            fprintf(stderr, "filename is too long.\n");
            return;
        case ENFILE:
            fprintf(stderr, "The system limit on the total number of open \
files has been reached.\n");
            return;
        case ENOENT:
            fprintf(stderr, "The file filename or a script or ELF interpreter \
does not exist, or a shared library needed for \
file or interpreter cannot be found.\n");
            return;
        case ENOEXEC:
            fprintf(stderr, "An executable is not in a recognised format, is \
for the wrong architecture, or has some other \
format error that means it cannot be \
executed.\n");
            return;
        case ENOMEM:
            fprintf(stderr, "Insufficient kernel memory was available.\n");
            return;
        case ENOTDIR:
            fprintf(stderr, "A component of the path prefix of filename or a \
script or ELF interpreter is not a directory.\n");
            return;
        case EPERM:
            fprintf(stderr, "The file system is mounted nosuid, the user is \
not the superuser, and the file has an SUID or \
SGID bit set.\n");
            return;
        case ETXTBSY:
            fprintf(stderr, "Executable was open for writing by one or more \
processes.\n");
            return;
        default:
            fprintf(stderr, "Unkown error occurred with execve().\n");
            return;
    }
}
/**************** END UTILITY FUNCTIONS ***************/


int deal_cgi(Request *request, char *buf)
{
    /*************** BEGIN VARIABLE DECLARATIONS **************/
    pid_t pid;
    int stdin_pipe[2];
    int stdout_pipe[2];
    //char buf[BUF_SIZE];
    int readret;
    /*************** END VARIABLE DECLARATIONS **************/

    /*************** BEGIN PIPE **************/
    /* 0 can be read from, 1 can be written to */
    if (pipe(stdin_pipe) < 0)
    {
        fprintf(stderr, "Error piping for stdin.\n");
        return EXIT_FAILURE;
    }

    if (pipe(stdout_pipe) < 0)
    {
        fprintf(stderr, "Error piping for stdout.\n");
        return EXIT_FAILURE;
    }
    /*************** END PIPE **************/

    /*************** BEGIN FORK **************/
    char *start = strchr(request->http_uri, '?');
    char *ENVP[3];
    if (start != NULL) {
        // 跳过 '?' 字符
        start++;

        // 为了避免修改原始字符串，复制一份字符串
        char *uri_copy = strdup(start);  // strdup 会复制字符串
        if (uri_copy == NULL) {
            perror("strdup failed");
            return 1;
        }

        // 使用 strtok 分割字符串，分隔符为 '&'
        char *separator = strchr(uri_copy, '&');
        if (separator != NULL) {
            // 分隔符找到，替换为 '\0' 来分割字符串
            *separator = '\0';
            
            // 将字符串的两部分分别赋给两个新的指针
            char *part1 = uri_copy;
            char *part2 = separator + 1;
            
            // 现在，使用 ENVP 添加这些环境变量
            // 需要留空间存储两个环境变量和一个结束指针
            
            ENVP[0] = part1;
            ENVP[1] = part2;
            ENVP[2] = NULL; 
        }
        
    } else {
        printf("No '?' found in the URI.\n");
    }
    POST_BODY = buf;
    pid = fork();
    /* not good */
    if (pid < 0)
    {
        fprintf(stderr, "Something really bad happened when fork()ing.\n");
        return EXIT_FAILURE;
    }

    /* child, setup environment, execve */
    if (pid == 0)
    {
        /*************** BEGIN EXECVE ****************/
        close(stdout_pipe[0]);
        close(stdin_pipe[1]);
        dup2(stdout_pipe[1], fileno(stdout));
        dup2(stdin_pipe[0], fileno(stdin));
        /* you should probably do something with stderr */

        /* pretty much no matter what, if it returns bad things happened... */
        if (execve(FILENAME, ARGV, ENVP))
        {
            execve_error_handler();
            fprintf(stderr, "Error executing execve syscall.\n");
            return EXIT_FAILURE;
        }
        /*************** END EXECVE ****************/ 
    }

    if (pid > 0)
    {
        fprintf(stdout, "Parent: Heading to select() loop.\n");
        close(stdout_pipe[1]);
        close(stdin_pipe[0]);

        if (write(stdin_pipe[1], POST_BODY, strlen(POST_BODY)) < 0)
        {
            fprintf(stderr, "Error writing to spawned CGI program.\n");
            return EXIT_FAILURE;
        }

        close(stdin_pipe[1]); /* finished writing to spawn */

        /* you want to be looping with select() telling you when to read */
        while((readret = read(stdout_pipe[0], buf, BUF_SIZE-1)) > 0)
        {
            buf[readret] = '\0'; /* nul-terminate string */
            fprintf(stdout, "Got from CGI: %s\n", buf);
        }

        close(stdout_pipe[0]);
        close(stdin_pipe[1]);

        if (readret == 0)
        {
            fprintf(stdout, "CGI spawned process returned with EOF as \
expected.\n");
            return EXIT_SUCCESS;
        }
    }
    /*************** END FORK **************/

    fprintf(stderr, "Process exiting, badly...how did we get here!?\n");
    return EXIT_FAILURE;
}



int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}
enum{
        Bad = 0, Not_Found, Not_Implemented, Version_not_supported ,HEAD // BAD_REQ = 1 , UNDEFINED_REQ = 2 三种请求类型： Echo、没实现、格式错误
    };

enum{
        HTML = 0, CSS, Image, cgi_py
    };

char *respon[] = {  
                    "HTTP/1.1 400 Bad request\r\n\r\n",
                    "HTTP/1.1 404 Not Found\r\n\r\n",
                    "HTTP/1.1 501 Not Implemented\r\n\r\n",
                    "HTTP/1.1 505 HTTP Version not supported\r\n\r\n",
                    "HTTP/1.1 200 OK\r\n\r\n"//只针对HEAD
                    };
long get_file_size(int fd) {
    off_t current_pos = lseek(fd, 0, SEEK_CUR);  // 获取当前文件指针位置
    off_t file_size = lseek(fd, 0, SEEK_END);    // 移动到文件末尾获取文件大小
    lseek(fd, current_pos, SEEK_SET);            // 恢复到原来的位置
    return file_size;
}

// 读取文件内容到动态分配的内存中
char* read_image(const char* filename, int* size, char *image_head) {
    int fd = open(filename, O_RDONLY);  // 以只读模式打开文件
    if (fd == -1) {
        perror("Unable to open image file");
        return NULL;
    }
    // 获取文件大小
    *size = get_file_size(fd);
    // 分配足够大的内存
    char* buffer = (char*)malloc(*size);
    if (buffer == NULL) {
        perror("Unable to allocate memory for image");
        close(fd);
        return NULL;
    }
    char str[20];
    memset(str, 0, 20); 
    sprintf(str, "%d", *size);
    sprintf(image_head, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n"
                            "Cache-Control: public, max-age=86400, immutable\r\n"
                            "Content-Type: image/png\r\n"
                            "Content-Length: ");
    sprintf(image_head + strlen(image_head), "%s", str);
    sprintf(image_head + strlen(image_head), "\r\n\r\n");
    // 使用 read() 逐块读取文件数据
    ssize_t total_read = 0;
    while (total_read < *size) {
        ssize_t bytes_read = read(fd, buffer + total_read, *size - total_read);
        if (bytes_read == -1) {
            perror("Error reading file");
            free(buffer);
            close(fd);
            return NULL;
        }
        total_read += bytes_read;
    }

    close(fd);  // 关闭文件
    return buffer;
}



int checkfile(char *filepath){
    size_t len = strlen(filepath);
    // 检查字符串是否以 ".css" 结尾
    if (len > 4 && strcmp(filepath + len - 4, ".css") == 0) {
        return CSS;
    }
    if (len > 4 && strcmp(filepath + len - 4, ".png") == 0) {
        return Image;
    }
    if (strstr(filepath, ".py?") != NULL) {
        //printf("The string contains '.py?'\n");
        return cgi_py;
    } 

    return -1;
}


int levelsend(int readRet, char *buf, int client_sock, int sock, char *data, int sort_file){
    char str[20];
    memset(str, 0, 20); 
    sprintf(str, "%d", readRet);
    sprintf(buf, "HTTP/1.1 200 OK\r\n"
                    "Connection: keep-alive\r\n");
    switch (sort_file)
    {
    case 0:
        sprintf(buf + strlen(buf), "Content-Type: text/html\r\n");
        break;
    case 1:
        sprintf(buf + strlen(buf), "Content-Type: text/css\r\n");
        break;
    case 2:
        sprintf(buf + strlen(buf), "Content-Type: image/png\r\n");
        break;
    default:
        break;
    }
    sprintf(buf + strlen(buf), "Content-Length: ");
    sprintf(buf + strlen(buf), "%s", str);
    sprintf(buf + strlen(buf), "\r\n\r\n");
    sprintf(buf + strlen(buf), "%s", data);
    fprintf(stdout, "%s\n", buf);
    if (send(client_sock, buf, strlen(buf), 0) != strlen(buf))
    {
        close_socket(client_sock);
        //close_socket(sock);
        fprintf(stderr, "Error sending to client.\n");
        return EXIT_FAILURE;
    }
    memset(buf, 0, BUF_SIZE);
    return 0;
}

int do_error_respon(int reps, char *buf, int client_sock, int sock){
    if(reps < 0 || reps > 5){return EXIT_FAILURE;}
    if (send(client_sock, respon[reps], strlen(respon[reps]), 0) != strlen(respon[reps]))
    {
        close_socket(client_sock);
        //close_socket(sock);
        fprintf(stderr, "Error sending to client.\n");
        return EXIT_FAILURE;
    }
    memset(buf, 0, BUF_SIZE);
    return 0;
}

int do_200_respon(Request *request, char *buf, int client_sock, int sock){
    //if(reps < 0 || reps > 5){return EXIT_FAILURE;}
    char url[] = "/";
    if(!memcmp(request->http_uri,url,strlen(url)) && request->http_uri[strlen(url)]=='\0'){
        char html[9999];
        memset(html, 0, 9999);
        int fd_in = open("static_site/index.html", O_RDONLY);
	    if(fd_in < 0) {
		    printf("Failed to open the html\n");
		    return 0;
	    }
        int readRet = read(fd_in, html, 9999);
        //int sort_file = checkfile(request->http_uri);
        if(levelsend(readRet, buf, client_sock, sock, html, HTML)){
            return 1;};
        return 0;
    }
    //request->http_uri;  // 文件路径
    // 检查文件是否存在
    char prefix[] = "static_site/";
        // 确保目标数组有足够的空间存储前缀和原路径
        // 使用 memmove 来将原数组的内容向右移动
    memmove(request->http_uri + strlen(prefix), request->http_uri, strlen(request->http_uri) + 1); // 移动原始路径
    memcpy(request->http_uri, prefix, strlen(prefix));
    char *pos = strchr(request->http_uri, '?');
    
    if (pos != NULL) {
        
        size_t len = pos - request->http_uri;
        char *result = malloc(len + 1);  // +1 为了存储 '\0'
        strncpy(result, request->http_uri, len);
        // 手动添加字符串终止符 '\0'
        result[len] = '\0';
        pos = result;
        //printf("Part before '?': %s\n", result);
    } else {
        pos = request->http_uri;
        // 如果没有找到 '?'，直接输出整个字符串
        //printf("No '?' found. The string is: %s\n", str);
    }
    fprintf(stdout, "%s\n", pos);
    int sort_file = checkfile(request->http_uri);
    if (access(pos, F_OK) == 0) {
        if(sort_file == HTML || sort_file == CSS){
            char data[9999];
            memset(data, 0, 9999);
            int fd_in = open(request->http_uri, O_RDONLY);
	        if(fd_in < 0) {
		        //printf("Failed to open the html\n");
                fprintf(stdout, "Failed to open the %s\n", buf);
		        return 0;
	        }
            int readRet = read(fd_in, data, 9999);
            int sort_file = checkfile(request->http_uri);
            if(levelsend(readRet, buf, client_sock, sock, data, sort_file)){return 1;};
            return 0;

        //printf("File exists: %s\n", file_path);
        }else if(sort_file == Image){
            int image_size;
            char image_head[BUF_SIZE];
            memset(image_head, 0, BUF_SIZE);
            char* image_data = read_image(request->http_uri, &image_size, image_head);
            //char buf_image[image_size];
            if (image_data == NULL) {
                return 1;  // 读取失败
            }
            if (send(client_sock, image_head, strlen(image_head), 0) != strlen(image_head))
            {
                close_socket(client_sock);
                close_socket(sock);
                fprintf(stderr, "Error sending to client.\n");
                return EXIT_FAILURE;
            }
            if (send(client_sock, image_data, image_size, 0) != image_size)
            {
                close_socket(client_sock);
                close_socket(sock);
                fprintf(stderr, "Error sending to client.\n");
                return EXIT_FAILURE;
            }

            memset(buf, 0, BUF_SIZE);       
            //if(levelsend(image_size, buf_image, client_sock, sock, image_data, sort_file)){return 1;};
            free(image_data);
            return 0;
           
        }else if(sort_file == cgi_py){
            deal_cgi(request, buf);
            if (send(client_sock, buf, strlen(buf), 0) != strlen(buf))
            {
                close_socket(client_sock);
                //close_socket(sock);
                fprintf(stderr, "Error sending to client.\n");
                return EXIT_FAILURE;
            }
            memset(buf, 0, BUF_SIZE);
        }
        
        //printf("File exists: %s\n", file_path);
    } else {
        do_error_respon(Not_Found, buf, client_sock, sock);
    }
    
    //do_error_respon(Not_Found, buf, client_sock, sock);
    return 0;
    

}


int checkmethod(char*a){
    if (a == NULL) return 0;
    char a1[] = "GET";
    char a2[] = "POST";
    char a3[] = "HEAD";
    if(! memcmp(a, a1, 3) && a[3]=='\0'){return 1;};
    if(! memcmp(a, a2, 4) && a[4]=='\0'){return 2;};
    if(! memcmp(a, a3, 4) && a[4]=='\0'){return 3;};
    return 0;
}

int deal_respon(char *buf, int i, int readret, int sock, fd_set *master_fds){
    // 处理客户端数据
    //handle_client(i);
    //int keep_alive = 1; 
    
    memset(buf, 0, BUF_SIZE);
    readret = recv(i, buf, BUF_SIZE, 0);
    if(readret == 0){
        close(i);
        fprintf(stderr, "client closed\n");
        FD_CLR(i, master_fds);
        return EXIT_SUCCESS;
    };

    if (readret < 0){
        close(i);
        fprintf(stderr, "Error accepting connection.\n");
        FD_CLR(i, master_fds);
        return EXIT_FAILURE;}
    fprintf(stdout, "===========  Server Received  ========= \n%s", buf);
    Requests *Reques = chunked_parse(buf, readret);
    //memset(buf, 0, BUF_SIZE); 
    Requests *p = Reques;
    //int i = 0;
    while(p != NULL)
    {
        Request *request = p->current_request;
        if(!request){
            if(do_error_respon(Bad, buf, i, sock))
            {
                return EXIT_FAILURE;
            };
            p = (Requests *)p->next_request;
            continue;
        }
        char vision[] = "HTTP/1.1";
        if(memcmp(request->http_version, vision, strlen(vision)) || request->http_version[strlen(vision)] != '\0')
        {
            if(do_error_respon(Version_not_supported, buf, i, sock))
            {
                return EXIT_FAILURE;
            };
            p = (Requests *)p->next_request;
            continue;
        }
        char keepalive[] = "keep-alive";
        // *识别为 keep-alive 时
        if(!memcmp(request->headers[1].header_value,keepalive,strlen(keepalive)))
        {
            
        }
            // *没有识别为 keep-alive 时
        else{
            //keep_alive = 0;
        }
        char* meth = request->http_method;
        int method_type = checkmethod(meth);
        if(method_type){
            if(method_type == 2){
                if (send(i, buf, strlen(buf), 0) != strlen(buf))
            {
                close_socket(i);
                fprintf(stderr, "Error sending to client.\n");
                return EXIT_FAILURE;
            }
            p = (Requests *)p->next_request;
            memset(buf, 0, BUF_SIZE);  
            continue;
            }
            if(method_type == 3){
                if(do_error_respon(HEAD, buf, i, sock)){
                    return EXIT_FAILURE;
                };
            p = (Requests *)p->next_request;
            memset(buf, 0, BUF_SIZE);  
            continue;
            }

            //i++;
            if(do_200_respon(request, buf, i, sock))
            {
                return EXIT_FAILURE;
            };
            p = (Requests *)p->next_request;
            continue;
        }else{
            if(do_error_respon(Not_Implemented, buf, i, sock))
            {
                return EXIT_FAILURE;
            };
            p = (Requests *)p->next_request;
            continue;
        }
            //i++;
            p = (Requests *)p->next_request;

    }
    return 0;
}

int main(int argc, char* argv[])
{
    int sock, client_sock, max_fd;
    ssize_t readret;
    socklen_t cli_size;
    fd_set read_fds, master_fds;
    struct sockaddr_in addr, cli_addr;
    char buf[BUF_SIZE];

    fprintf(stdout, "----- Echo Server -----\n");
    
    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ECHO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }


    if (listen(sock, 5))
    {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    FD_ZERO(&master_fds);
    FD_SET(sock, &master_fds);
    max_fd = sock;

    /* finally, loop waiting for input and then write it back */
    while (1)
    {
        read_fds = master_fds;
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL); // 调用 select()
        if (activity == -1) {
            perror("select error");
            break;
        }

        for (int i = 0; i <= max_fd; i++) 
        {
            if (FD_ISSET(i, &read_fds)) 
            { // 如果 i 是可读的套接字
                if (i == sock) 
                {
                    // 处理新连接
                    cli_size = sizeof(cli_addr);
                    if ((client_sock = accept(sock, (struct sockaddr *) &cli_addr,
                                                &cli_size)) == -1)
                    {
                        close(sock);
                        fprintf(stderr, "Error accepting connection.\n");
                        return EXIT_FAILURE;
                    }

                    FD_SET(client_sock, &master_fds);   // 将新的客户端套接字添加到 master_fds
                    
                    if (client_sock > max_fd) 
                    {
                        max_fd = client_sock;        // 更新最大文件描述符
                    } 
                }
                else
                {
                    readret = 0;
                    deal_respon(buf, i, readret, sock, &master_fds);
                    memset(buf, 0, BUF_SIZE);
                }
            }
        }
    
        

    }
    
    
    
    close_socket(sock);

    return EXIT_SUCCESS;
}
