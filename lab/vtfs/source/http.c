#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <linux/uio.h>
#include <linux/errno.h>
#include <linux/string.h>

const char *SERVER_IP = "10.0.2.2";
const int SERVER_PORT = 8089;

// формирует HTTP-запрос, возвращает 0 или отрицательный код ошибки
static int fill_request(struct kvec *vec, const char *token, const char *method,
                        size_t arg_size, va_list args)
{
    char *request_buffer = kzalloc(2048 + 64, GFP_KERNEL);
    if (!request_buffer)
        return -ENOMEM;

    strcpy(request_buffer, "GET /api/");
    strcat(request_buffer, method);
    strcat(request_buffer, "?token=");
    strcat(request_buffer, token);

    for (size_t i = 0; i < arg_size; i++) {
        strcat(request_buffer, "&");
        strcat(request_buffer, va_arg(args, char *));
        strcat(request_buffer, "=");
        strcat(request_buffer, va_arg(args, char *));
    }

    strcat(request_buffer, " HTTP/1.1\r\nHost:");
    strcat(request_buffer, SERVER_IP);
    strcat(request_buffer, "\r\nConnection: close\r\n\r\n");

    vec->iov_base = request_buffer;
    vec->iov_len = strlen(request_buffer);

    return 0;
}

// читает весь ответ в буфер
static int receive_all(struct socket *sock, char *buffer, size_t buffer_size)
{
    struct msghdr msg;
    struct kvec vec;
    int total = 0;

    while (total < buffer_size) {
        memset(&msg, 0, sizeof(msg));
        vec.iov_base = buffer + total;
        vec.iov_len = buffer_size - total;

        int ret = kernel_recvmsg(sock, &msg, &vec, 1, vec.iov_len, 0);
        if (ret == 0) break; // EOF
        if (ret < 0) return ret;

        total += ret;
    }

    return total;
}

// парсит HTTP-ответ, возвращает int64_t из тела
static int64_t parse_http_response(char *raw, size_t raw_size, char *resp, size_t resp_size)
{
    char *p = raw;
    char *line, *status;
    int content_length = -1;

    line = strsep(&p, "\r\n");
    if (!line) return -6;

    status = strsep(&line, " ");
    if (!status) return -6;
    status = strsep(&line, " ");
    if (!status) return -6;

    if (strcmp(status, "200") != 0)
        return -5;

    while (p && *p) {
        line = strsep(&p, "\r\n");
        if (!line || line[0] == '\0') break;

        if (!strncmp(line, "Content-Length: ", 16)) {
            if (kstrtoint(line + 16, 0, &content_length))
                return -6;
        }
    }

    if (content_length < (int)sizeof(int64_t))
        return -7;
    content_length -= sizeof(int64_t);

    if (content_length > (int)resp_size)
        return -ENOSPC;

    int64_t ret_val;
    memcpy(&ret_val, p, sizeof(int64_t));
    p += sizeof(int64_t);
    memcpy(resp, p, content_length);

    return ret_val;
}

// делает HTTP-вызов
int64_t vtfs_http_call(const char *token, const char *method,
                       char *response_buffer, size_t buffer_size,
                       size_t arg_size, ...)
{
    struct socket *sock;
    struct sockaddr_in saddr;
    int64_t ret;
    int error;

    error = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (error < 0) return -1;

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVER_PORT);
    saddr.sin_addr.s_addr = in_aton(SERVER_IP);

    if ((error = kernel_connect(sock, (struct sockaddr *)&saddr, sizeof(saddr), 0)) < 0) {
        sock_release(sock);
        return -2;
    }

    struct kvec vec;
    va_list args;
    va_start(args, arg_size);
    error = fill_request(&vec, token, method, arg_size, args);
    va_end(args);

    if (error) {
        kernel_sock_shutdown(sock, SHUT_RDWR);
        sock_release(sock);
        return error;
    }

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));


    error = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
    kfree(vec.iov_base);
    if (error < 0) {
        kernel_sock_shutdown(sock, SHUT_RDWR);
        sock_release(sock);
        return -3;
    }

    char *raw_buffer = kmalloc(buffer_size + 1024, GFP_KERNEL);
    if (!raw_buffer) {
        kernel_sock_shutdown(sock, SHUT_RDWR);
        sock_release(sock);
        return -ENOMEM;
    }

    int read_bytes = receive_all(sock, raw_buffer, buffer_size + 1024);
    kernel_sock_shutdown(sock, SHUT_RDWR);
    sock_release(sock);
    if (read_bytes < 0) {
        kfree(raw_buffer);
        return -4;
    }

    ret = parse_http_response(raw_buffer, read_bytes, response_buffer, buffer_size);
    kfree(raw_buffer);
    return ret;
}

// URL-encode
void encode(const char *src, char *dst)
{
    while (*src) {
        if ((*src >= '0' && *src <= '9') ||
            (*src >= 'a' && *src <= 'z') ||
            (*src >= 'A' && *src <= 'Z')) {
            *dst++ = *src;
        } else {
            sprintf(dst, "%%%02X", (unsigned char)*src);
            dst += 3;
        }
        src++;
    }
    *dst = '\0';
}
