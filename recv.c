#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 65535

// 为 16 位数据交换大小端
#define swap16(x) ((((x)&0xFF) << 8) | (((x) >> 8) & 0xFF))

char buf[MAX_SIZE + 1];

/*
 * @brief 自定义发信方法，封装了 socket 的 send，并打印己方发送的信息。
 * 多了一个参数 error_msg 是附上错误信息，用于定位
 *
 * @param sockfd socket 函数返回的套接字描述符
 * @param buf 发送数据的缓冲区指针
 * @param len 缓冲区的大小
 * @param flags 一般取 0
 * @error_msg 错误信息，用于定位
 */
int cus_send(int sockfd, void *buf, int len, int flags, char *error_msg)
{
    int ret = -1;
    if ((ret = send(sockfd, buf, len, flags)) == -1)
    {
        perror(error_msg);
        exit(EXIT_FAILURE);
    }

    printf("%s\r\n", (char *)buf);

    return ret;
}

/*
 * @brief 自定义收信方法，封装了 socket 的 recv，并打印接收的信息。
 * 多了一个参数 error_msg 是附上错误信息，用于定位
 *
 * @param sockfd socket 函数返回的套接字描述符
 * @param buf 发送数据的缓冲区指针
 * @param len 缓冲区的大小
 * @param flags 一般取 0
 * @error_msg 错误信息，用于定位
 */
int cus_recv(int sockfd, void *buf, int len, int flags, char *error_msg)
{
    int r_size = -1;
    if ((r_size = recv(sockfd, buf, len, 0)) == -1)
    {
        perror(error_msg);
        exit(EXIT_FAILURE);
    }
    char *tmp_buf = (char *)buf;
    tmp_buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", tmp_buf);
    return r_size;
}

// POP3 协议
void recv_mail()
{
    /********************************* 准备工作 *********************************/
    const char *host_name = "smtp.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 110;       // POP3 server port
    const char *user = "xxxxxxxxx@qq.com"; // TODO: Specify the user
    const char *pass = "xxxxxxxxxxxxxxxx"; // TODO: Specify the password
    char dest_ip[16];
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **)host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i - 1]));
    /********************************* 准备结束 *********************************/

    /********************************* socket 启动 *********************************/
    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    if ((s_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket file descriptor");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;
    socklen_t addrlen = sizeof(servaddr); // addrlen 参数是套接字地址结构（sockaddr）的长度

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = swap16(port);
    servaddr.sin_addr = (struct in_addr){inet_addr(dest_ip)};
    bzero((&servaddr)->sin_zero, addrlen);

    if (connect(s_fd, (struct sockaddr *)(&servaddr), addrlen) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    /********************************* socket 启动 *********************************/

    /**************************** 开始与 POP3 服务器交互 ****************************/
    /*
     * POP3 协议常用的 9 条命令如下：
     * 1. USER <name><CR><LF>
     * 2. PASS <name><CR><LF>
     * 3. STAT<CR><LF>
     * 4. LIST [<msg>]<CR><LF>
     * 5. RETR msg<CR><LF>
     * 6. DELE msg<CR><LF>
     * 7. RSET<CR><LF>
     * 8. NOOP<CR><LF>
     * 9. QUIT<CR><LF>
     */

    // Print welcome message
    // if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    // {
    //     perror("recv");
    //     exit(EXIT_FAILURE);
    // }
    // buf[r_size] = '\0'; // Do not forget the null terminator
    // printf("%s", buf);

    // 1 服务器发送欢迎消息。
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv");

    // 2 客户端输入用户名和密码进行认证，如果正确，服务器会返回成功信息。
    // TODO: Send user and password and print server response
    sprintf(buf, "USER %s\r\n", user);
    cus_send(s_fd, (void *)buf, strlen(buf), 0, "send USER");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv USER");

    sprintf(buf, "PASS %s\r\n", pass);
    cus_send(s_fd, (void *)buf, strlen(buf), 0, "send PASS");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv PASS");

    // 3 认证成功后，客户端可以输入一系列命令获取信息，如 STAT、LIST、RETR、DELE 等。
    // 指导书要求：分别获取总邮件个数及大小、每封邮件的编号及大小、第一封邮件的内容。

    // 总邮件个数及大小
    // TODO: Send STAT command and print server response
    const char *STAT = "STAT\r\n";
    cus_send(s_fd, (void *)STAT, strlen(STAT), 0, "send STAT");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv STAT");

    // 每封邮件的编号及大小
    // TODO: Send LIST command and print server response
    const char *LIST = "LIST\r\n";
    cus_send(s_fd, (void *)LIST, strlen(LIST), 0, "send LIST");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv LIST");

    // 第一封邮件的内容
    // TODO: Retrieve the first mail and print its content
    // 例如：比如总大小是 42054，一共接收了四次
    // total_size = 42054
    // 1. r_size = 14104
    // 2. r_size = 5696
    // 3. r_size = 19936
    // 4. r_size = 2334

    sprintf(buf, "RETR %d\r\n", 1); // first
    cus_send(s_fd, (void *)buf, strlen(buf), 0, "send RETR");
    r_size = cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv RETR 1"); // 先接收了一部分

    // atoi 会从数字开始读，读到非数字停止，取出数字
    // 所以 atoi 刚好取出 +OK (4 字符) 后的数字 total_size 就停止
    int total_size = atoi(buf + 4);
    total_size -= r_size;

    while (total_size > 0) // 继续读剩下的，直到结束
    {
        r_size = cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv RETR");
        total_size -= r_size;
    }

    // 4 客户端发送 QUIT 命令，结束会话。
    // TODO: Send QUIT command and print server response
    const char *QUIT = "QUIT\r\n";
    cus_send(s_fd, (void *)QUIT, strlen(QUIT), 0, "send QUIT");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv QUIT");
    /**************************** 结束与 POP3 服务器交互 ****************************/

    close(s_fd);
    /********************************* socket 关闭 *********************************/
}

int main(int argc, char *argv[])
{
    recv_mail();
    exit(0);
}
