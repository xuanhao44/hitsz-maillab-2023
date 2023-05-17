#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

#define MAX_SIZE 4095

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

/*
 * @brief 把文本文件转换为字符串
 *
 * @param path 文本文件地址
 */
char *file2str(const char *path)
{
    FILE *fp = fopen(path, "r");
    fseek(fp, 0, SEEK_END);
    int fplen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *content = (char *)malloc(fplen);
    fread(content, 1, fplen, fp);
    fclose(fp);
    return content;
}

/*
 * @brief 从已有文件转换为 base64 文件
 * 转换的文件名都为 attach_base64
 *
 * @param path 文本文件地址
 */
int gen_base64_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return -1;

    FILE *fp_base64 = fopen("attach_base64", "w");
    encode_file(fp, fp_base64);
    fclose(fp);
    fclose(fp_base64);
    return 0;
}

// SMTP 协议
// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char *receiver, const char *subject, const char *msg, const char *att_path)
{
    /********************************* 准备工作 *********************************/
    const char *end_msg = "\r\n.\r\n";
    const char *host_name = "smtp.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 25;        // SMTP server port
    const char *user = "xxxxxxxxx@qq.com"; // TODO: Specify the user
    const char *pass = "xxxxxxxxxxxxxxxx"; // TODO: Specify the password
    const char *from = "xxxxxxxxx@qq.com"; // TODO: Specify the mail address of the sender
    char dest_ip[16];                      // Mail server IP address
    int s_fd;                              // socket file descriptor
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
    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
    if ((s_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket file descriptor");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = swap16(port);                         // htons(port) 也 OK
    servaddr.sin_addr = (struct in_addr){inet_addr(dest_ip)}; // 看定义这里还是要套一层的
    bzero(servaddr.sin_zero, sizeof(servaddr.sin_zero));      // memset 也 OK

    socklen_t addrlen = sizeof(servaddr); // addrlen 参数是套接字地址结构（sockaddr）的长度
    if (connect(s_fd, (struct sockaddr *)(&servaddr), addrlen) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    /********************************* socket 启动 *********************************/

    /**************************** 开始与 SMTP 服务器交互 ****************************/
    /*
     * SMTP 协议中一共定义了 18 条命令，但是发送一封电子邮件的过程通常只需要 6 条命令：
     * 1. EHLO <domain><CR><LF>
     * 2. AUTH <para><CR><LF>
     * 3. MAIL FROM: <reverse-path><CR><LF>
     * 4. RCPT TO: <forward-path><CR><LF>
     * 5. DATA<CR><LF>
     * 6. QUIT<CR><LF>
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

    // 2 客户端发送 EHLO 命令表明身份，服务器列出它支持的命令。
    // Send EHLO command and print server response
    // TODO: Enter EHLO command here
    // TODO: Print server response to EHLO command
    const char *EHLO = "EHLO qq.com\r\n";
    cus_send(s_fd, (void *)EHLO, strlen(EHLO), 0, "send EHLO");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv EHLO");

    // 3 客户端选择登录认证方式。
    // EHLO 命令后服务器给出的列表会提示支持的认证方式，本实验可选择 login，即输入命令：AUTH login。
    // TODO: Authentication. Server response should be printed out.
    const char *AUTH = "AUTH login\r\n";
    cus_send(s_fd, (void *)AUTH, strlen(AUTH), 0, "send AUTH");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv AUTH");

    // 4 服务器发送经过 Base64 编码的字符串“Username:”
    // 然后客户端发送经过 Base64 编码的用户名。
    char *user_base64 = encode_str(user);
    strcat(user_base64, "\r\n");
    cus_send(s_fd, (void *)user_base64, strlen(user_base64), 0, "send username");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv AUTH");
    free(user_base64);

    // 5 服务器发送经过 Base64 编码的字符串“Password:”
    // 然后客户端发送经过 Base64 编码的口令。如用户名和口令正确，则服务器提示认证成功。
    char *pass_base64 = encode_str(pass);
    strcat(pass_base64, "\r\n");
    cus_send(s_fd, (void *)pass_base64, strlen(pass_base64), 0, "send password");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv AUTH");
    free(pass_base64);

    // 6 客户端指定邮件的发送人和收件人：
    // MAIL FROM:<alice@qq.com>
    // RCPT TO:<bob@163.com>
    // 每次换行后，服务器都会提示成功。
    // TODO: Send MAIL FROM command and print server response
    sprintf(buf, "MAIL FROM:<%s>\r\n", from);
    cus_send(s_fd, (void *)buf, strlen(buf), 0, "send MAIL FROM");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv MAIL FROM");

    // TODO: Send RCPT TO command and print server response
    sprintf(buf, "RCPT TO:<%s>\r\n", receiver);
    cus_send(s_fd, (void *)buf, strlen(buf), 0, "send RCPT TO");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv RCPT TO");

    // 7 客户端输入 DATA 命令，服务器提示输入内容后以“.”表示消息结束。
    // 之后就可以编写要发送的邮件内容。邮件的编写格式遵照 Internet 消息格式。
    // TODO: Send DATA command and print server response
    const char *DATA = "DATA\r\n";
    cus_send(s_fd, (void *)DATA, strlen(DATA), 0, "send DATA");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv DATA");

    // TODO: Send message data
    // 电子邮件消息的格式在 RFC 5322 Internet Message Format 中有详细规定。一些常见的邮件头介绍如下：
    // To: 收信人邮件地址
    // Cc: 抄送人邮件地址
    // Bcc: 密送人邮件地址
    // From: 写信人邮件地址
    // Message-Id : 邮件惟一标识符
    // Keywords: 邮件关键词
    // Subject: 邮件主题

    // 为了能够发送更多的字符及多媒体内容，MIME（Multipurpose Internet Message Extensions）被提出。
    // MIME 在 RFC 2045-2049 中规定。MIME 被广泛用于电子邮件中，也用来描述其他应用的内容，如 Web 浏览。

    // 使用 multipart 的邮件内容通常还需要指定一个边界，用以分隔不同部分内容，如 qwertyuiopasdfghjklzxcvbnm
    // 发送了两个不同格式的信息，所以需要指定 Content-Type 为 multipart/mixed 类型。
    // 而后，boundary 用来指定区分不同内容的边界字符串。后面，两种格式的信息被边界符分开，边界符由两个短横线和边界字符串组成。

    const char *cus_boundary = "qwertyuiopasdfghjklzxcvbnm"; // for mail context

    // initial header
    sprintf(buf,
            "From: %s\r\n"
            "To: %s\r\n"
            "Content-Type: multipart/mixed; boundary=%s\r\n",
            from, receiver, cus_boundary);

    if (subject != NULL)
    {
        strcat(buf, "Subject: ");
        strcat(buf, subject);
        strcat(buf, "\r\n\r\n");
    }

    cus_send(s_fd, (void *)buf, strlen(buf), 0, "send DATA header");

    // message
    if (msg != NULL)
    {
        sprintf(buf,
                "--%s\r\n"
                "Content-Type: text/plain\r\n\r\n",
                cus_boundary);

        cus_send(s_fd, (void *)buf, strlen(buf), 0, "send DATA msg");

        if (access(msg, F_OK) == 0) // 文件路径
        {
            char *content = file2str(msg);
            cus_send(s_fd, (void *)content, strlen(content), 0, "send DATA msg content");
            free(content);
        }
        else // 直接输入文本
        {
            cus_send(s_fd, (void *)msg, strlen(msg), 0, "send DATA msg content");
        }
        cus_send(s_fd, "\r\n", 2, 0, "");
    }

    // path to the attachment，附件
    if (att_path != NULL)
    {
        if (gen_base64_file(att_path) == -1)
        {
            perror("file not exist");
            exit(EXIT_FAILURE);
        }
        else // add header only when file is correct.
        {
            sprintf(buf,
                    "--%s\r\n"
                    "Content-Type: application/octet-stream\r\n"
                    "Content-Transfer-Encoding: base64\r\n"
                    "Content-Disposition: attachment; name=%s\r\n\r\n",
                    cus_boundary, att_path);

            cus_send(s_fd, (void *)buf, strlen(buf), 0, "send DATA attach");

            char *attach = file2str("attach_base64");
            cus_send(s_fd, (void *)attach, strlen(attach), 0, "send DATA attachment files");
            free(attach);
        }
        sprintf(buf, "--%s\r\n", cus_boundary);
        cus_send(s_fd, (void *)buf, strlen(buf), 0, "send DATA end");
    }

    // 8 客户端输入“.”表示邮件内容输入完毕，服务器提示成功。
    // TODO: Message ends with a single period
    cus_send(s_fd, (void *)end_msg, strlen(end_msg), 0, "send .");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv .");

    // 9 客户端输入 QUIT 命令断开与邮件服务器的连接，服务器提示连接中断。
    // 服务器具体回复的消息会根据实际具有的功能及配置有所不同，但是回复的代码都是一致的。
    // TODO: Send QUIT command and print server response
    const char *QUIT = "QUIT\r\n";
    cus_send(s_fd, (void *)QUIT, strlen(QUIT), 0, "send QUIT");
    cus_recv(s_fd, (void *)buf, MAX_SIZE, 0, "recv QUIT");
    /**************************** 结束与 SMTP 服务器交互 ****************************/

    close(s_fd);
    /********************************* socket 关闭 *********************************/
}

int main(int argc, char *argv[])
{
    int opt;
    char *s_arg = NULL;
    char *m_arg = NULL;
    char *a_arg = NULL;
    char *recipient = NULL;
    const char *optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}
