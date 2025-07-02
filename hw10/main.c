#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#define HOST "telehack.com"

#define BUFSIZE 1024
#define COMMAND "figlet"

char* create_message(const char* font, const char* umsg)
{
    const size_t total_len = strlen(COMMAND) + strlen(font) + strlen(umsg) + 4;
    char* cmd = malloc(sizeof(char) * total_len);
    if (cmd == NULL)
    {
        return NULL;
    }
    // figlet /font text
    strcpy(cmd, COMMAND);
    strcat(cmd, " /");
    strcat(cmd, font);
    strcat(cmd, " ");
    strcat(cmd, umsg);
    return cmd;
}

int main(const int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <font family> <text>\n", argv[0]);
        return 1;
    }
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        return -1;
    }

    struct addrinfo hints = {0};
    struct addrinfo* res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(HOST, "23", &hints, &res) < 0)
    {
        perror("getaddrinfo");
        close(fd);
        return -1;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        printf("Failed to connect to server\n");
        close(fd);
        return -1;
    }

    char* msg = create_message(argv[1], argv[2]);
    if (msg == NULL)
    {
        perror("Failed to create message");
        close(fd);
        freeaddrinfo(res);
        return -1;
    }
    if (send(fd, msg, strlen(msg), 0) == -1)
    {
        printf("Failed to send msg\n");
        close(fd);
        free(msg);
        freeaddrinfo(res);
        return -1;
    }
    // тут представим что нет ошибок и все хорошо и просто обработаем позитивный кейс
    char buf[BUFSIZE];
    ssize_t bytes = 0;
    while ((bytes = recv(fd, buf, BUFSIZE - 1, 0)) > 0)
    {
        buf[bytes] = '\0';
        printf("%s", buf);
    }

    free(msg);
    close(fd);
    freeaddrinfo(res);

    return 0;
}
