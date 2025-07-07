#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/epoll.h>
#include <dirent.h>

#define BACK_LOG 10
#define POOL_TIMEOUT 2500 // 2.5 sec
#define WELCOME_MSG "Ender directory for download: "
#define POOL_SIZE 1000

int main(const int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }

    const char* ip = "127.0.0.1"; // ignore argv[2];
    const char* port = argv[3];

    struct addrinfo hints = {0};
    struct addrinfo* res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(ip, port, &hints, &res) < 0)
    {
        printf("%s", strerror(errno));
        return -1;
    }

    const int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd < 0)
    {
        printf("%s", strerror(errno));
        return -1;
    }
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
    const int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (bind(server_fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        printf("%s", strerror(errno));
        return -1;
    }

    if (listen(server_fd, BACK_LOG) < 0)
    {
        printf("%s", strerror(errno));
        return -1;
    }


    const int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
    {
        printf("%s", strerror(errno));
        close(server_fd);
        return -1;
    }
    struct epoll_event server_event = {0};
    server_event.events = EPOLLIN;
    server_event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_event) < 0)
    {
        printf("%s", strerror(errno));
        return -1;
    }

    while (1)
    {
        struct epoll_event client_events[POOL_SIZE];
        const int n = epoll_wait(epoll_fd, client_events, POOL_SIZE, -1); // ждать бесконечно
        if (n < 0)
        {
            printf("%s", strerror(errno));
            continue;
        }
        for (int i = 0; i < n; i++)
        {
            if (client_events[i].data.fd == server_fd)
            {
                struct sockaddr_storage client_addr;
                socklen_t client_len = sizeof(client_addr);
                const int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd < 0)
                {
                    printf("%s", strerror(errno));
                    continue;
                }
                fcntl(client_fd, F_SETFL, O_NONBLOCK); // сделать клиента неблокирующим
                if (send(client_fd, WELCOME_MSG, strlen(WELCOME_MSG), 0) < 0)
                {
                    printf("%s", strerror(errno));
                    close(client_fd);
                    continue;
                }
                struct epoll_event client_event = {
                    .events = EPOLLIN | EPOLLET, // EPOLLIN — данные можно читать
                    .data.fd = client_fd // храним fd клиента
                };
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) < 0)
                {
                    printf("%s", strerror(errno));
                    close(client_fd);
                    continue;
                }
            }
            else
            {
                char buf[PATH_MAX];
                ssize_t total = 0;
                ssize_t current = 0;
                while ((current = read(client_events[i].data.fd, buf + total, sizeof(buf) - total)) > 0)
                {
                    total += current;
                }
                if (current == 0) {
                    // клиент закрыл соединение (EOF)
                    printf("Client disconnected: fd=%d\n", client_events[i].data.fd);
                    close(client_events[i].data.fd);
                }
                else if (current == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    } else {
                        const char * msg = strerror(errno);
                        send(client_events[i].data.fd, msg, strlen(msg), 0);
                        close(client_events[i].data.fd);
                    }
                }
                if (total <= 0)
                {
                    continue;
                }
                buf[total - 1] = '\0';
                DIR * dir = opendir(buf);
                if (dir == NULL)
                {
                    const char * msg = strerror(errno);
                    send(client_events[i].data.fd, msg, strlen(msg), 0);
                    close(client_events[i].data.fd);
                    continue;
                }
                struct dirent *entry = NULL;
                while ((entry = readdir(dir)) != NULL)
                {
                    const char * msg = strcat(entry->d_name, "\n");
                    send(client_events[i].data.fd, msg, strlen(msg), 0);
                }
            }
        }
    }
    close(server_fd);
    return 0;
}
