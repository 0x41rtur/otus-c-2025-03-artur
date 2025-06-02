#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#define SOCKET_PATH "/tmp/home_work07.socket"

/* демона дебажить */
void log_msg(const char* msg)
{
    FILE* log = fopen("/tmp/daemon.log", "a");
    if (log)
    {
        fprintf(log, "%s\n", msg);
        fclose(log);
    }
}

double get_file_size_in_kb(FILE* file)
{
    if (-1 == fseek(file, 0, SEEK_END))
    {
        return -1;
    }
    long size_in_bytes;
    if (-1 == (size_in_bytes = ftell(file)))
    {
        return -1;
    }
    fclose(file);
    return (double)size_in_bytes / 1024;
}

void print_file_size_by_path(const char* file_path)
{
    if (NULL == file_path)
    {
        printf("File path is incorrect\n");
        exit(1);
    }
    FILE* fd = fopen(file_path, "r");
    if (NULL == fd)
    {
        printf("%s\n", strerror(errno));
        exit(1);
    }
    double size_in_kb;
    if (-1 == (size_in_kb = get_file_size_in_kb(fd)))
    {
        printf("%s\n", strerror(errno));
        exit(1);
    }
    printf("File size: %lf kB\n", size_in_kb);
}

/* клиент он же терминал общаеться с юзером принимая имена файлов и пересылает в демона*/
void start_client_socket()
{
    char* line = NULL;
    size_t size = 0;
    size_t line_size = 0;

    while (-1 != (line_size = getline(&line, &size, stdin)))
    {
        if (line[line_size - 1] == '\n')
            line[line_size - 1] = '\0';

        const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (-1 == fd)
        {
            printf("%s\n", strerror(errno));
            continue;
        }

        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            printf("%s\n", strerror(errno));
            close(fd);
            continue;
        }

        if (write(fd, line, strlen(line)) < 0)
        {
            printf("%s\n", strerror(errno));
            close(fd);
            continue;
        }

        char buffer[PATH_MAX] = {0};
        ssize_t read_bytes = read(fd, &buffer, PATH_MAX - 1);
        if (read_bytes <= 0)
        {
            printf("Read failed or connection closed\n");
            close(fd);
            continue;
        }

        buffer[read_bytes] = '\0';
        double value = strtod(buffer, NULL);
        printf("%f kB\n", value);

        close(fd);
    }

    free(line);
}


/* демон получает от клиента имя файла, расчитывает его размер и возвращает обратно клиенту */
void start_server_socket()
{
    log_msg("Prepare server socket");
    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == fd)
    {
        log_msg(strerror(errno));
        exit(1);
    }
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCKET_PATH);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) < 0)
    {
        log_msg(strerror(errno));
        close(fd);
        exit(1);
    }
    if (listen(fd, 1) < 0)
    {
        log_msg(strerror(errno));
        close(fd);
        exit(1);
    }
    log_msg("Server socket ready");
    while (1)
    {
        const int client_fd = accept(fd, NULL, NULL);
        log_msg("Accepted connection");
        if (-1 == client_fd)
        {
            log_msg(strerror(errno));
            continue;
        }
        char buffer[PATH_MAX] = {0};
        const ssize_t bytes_read = read(client_fd, &buffer, PATH_MAX);
        if (bytes_read < 0)
        {
            log_msg(strerror(errno));
            continue;
        }
        buffer[bytes_read] = '\0';
        log_msg(buffer);
        const FILE* file = fopen(buffer, "r");
        if (NULL == file)
        {
            log_msg(strerror(errno));
            write(client_fd, strerror(errno), strlen(strerror(errno)));
            continue;
        }
        const double file_size_in_kb = get_file_size_in_kb(file);
        /* хардкод в надежде, что размер файла в килобайтах не превышает 10 цифр */
        char output[64];
        snprintf(output, sizeof(output), "%f", file_size_in_kb);
        log_msg(output);
        write(client_fd, output, strlen(output));
    }
    // закрыть сокеты и все такое
}

void run_as_demon()
{
    const pid_t pid = fork();
    if (-1 == pid)
    {
        printf("Fork failed\n, you can try again soon.");
        exit(1);
    }
    /* демон как сервер */
    if (0 == pid)
    {
        setsid();
        umask(0);
        chdir("/tmp"); // можно просить размер файла логов daemon.log )))))
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);
        start_server_socket();
    }
    else
    {
        // родитель стал клиентов, но чисто в учебных целях
        start_client_socket();
    }
}

/**
 * -i Взаимодействие происходит в интерактивном режиме
 *  INPUT -> PUTPUT все в основном процессе
 **/

/**
 * -d Взаимодействие происходит в режиме демона
 * INPUT -> SOSKET DEMON INPUT -> SOCKET DEMON OUTPUT -> OUTPUT
 * При выходе из терминала демон убиваеться
 **/

int main(const int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s [-d | -i] <file_name>\n", argv[0]);
        printf("Options:\n -i - interactive mode\n -d - demon mode");
        exit(0);
    }
    if (strncmp(argv[1], "-d", 2) == 0)
    {
        run_as_demon();
    }
    else if (strncmp(argv[1], "-i", 2) == 0)
    {
        /* завершится только после неправильного ввода пути или Ctrl+C */
        char* line = NULL;
        size_t size = 0;
        size_t line_size = 0;
        while (-1 != (line_size = getline(&line, &size, stdin)))
        {
            if (line[line_size - 1] == '\n')
            {
                line[line_size - 1] = '\0';
            }
            print_file_size_by_path(line);
        }
    }
    else
    {
        printf("Unexpected mode [%s].\n Usage: %s [-d | -i] <file_name>\n", argv[1], argv[0]);
    }
    return 0;
}
