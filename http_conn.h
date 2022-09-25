#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"
#include "threadpool.h"


class http_conn {
public:
    static const int FILENAME_LEN = 200;//文件名最大长度
    static const int READ_BUFFER_SIZE = 2048;//读缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024;//写缓冲区大小
    /* HTTP请求方法 */
    enum METHOD {
        GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH
    };
    /* 解析客户请求时，主状态机所处的状态 */
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    /* 服务器处理HTTP请求的可能结果 */
    enum HTTP_CODE {
        NO_RESQUEST, GET_RESQUEST, BAD_RESQUEST, NO_RESOURCE, FORBIDDEN_REQUEST,
        FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION
    };
    /* 行的读取状态 */
    enum LINE_STATE {
        LINE_OK = 0, LINE_BAD, LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    /* 初始化新接受的连接 */
    void init(int sockfd, const sockaddr_in &addr);
    /* 关闭连接 */
    void close_conn(bool real_close = true);
    /* 处理客户请求 */
    void process();
    /* 非阻塞读操作 */
    bool read();
    /* 非阻塞写操作 */
    bool write();

private:
    /* 初始化连接 */
    void init();
    /* 解析HTTP请求 */
    HTTP_CODE process_read();
    /* 填充HTTP应答 */
    bool process_write(HTTP_CODE ret);

    /* 下面这一组函数被process_read调用以分析HTTP请求 */
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; }
    LINE_STATE parse_line();

    /* 下面这一组函数被process_write调用以填充HTTP应答 */
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    /* 所有socket上的事件都被注册到同一个epoll内核事件表中，所以epoll文件描述符设置为静态的 */
    static int m_epollfd;
    static int m_user_count;//统计用户数量
    
private:
    /* 该HTTP连接的socket和对方的socket地址 */
    int m_sockfd;
    sockaddr_in m_address;

    char m_read_buf[READ_BUFFER_SIZE];//读缓冲区
    int m_read_idx;//标识读缓冲中已经读入的客户数据的最后一个字节的下一个位置
    int m_checked_idx;//当前正在分析的字符在读缓冲区中的位置
    int m_start_line;//当前正在解析的行的起始位置
    char m_write_buf[WRITE_BUFFER_SIZE];//写缓冲区
    int m_write_idx;//写缓冲区中待发送的字节数

    CHECK_STATE m_check_state;//主状态机当前所处的状态
    METHOD m_method;//请求方法

    char m_real_file[FILENAME_LEN];//文件完整路径
    char *m_url;//客户请求的目标文件的文件名
    char *m_version;//HTTP协议版本号
    char *m_host;//主机名
    int m_content_length;//HTTP请求的消息长度
    bool m_linger;//HTTP请求是否要保持连接

    char *m_file_address;//客户请求的文件被mmap到内存中的起始位置
    struct stat m_file_stat;//目标文件的状态,通过此判断文件是否存在、是否为目录、是否可读，并获取文件大小等
    struct iovec m_iv[2]; 
    int m_iv_count;
     
};

#endif