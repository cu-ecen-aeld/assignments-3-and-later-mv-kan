#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>

#define THEPORT "9000"
#define BUF_SIZE 1024
#define VARTMPFILE "/var/tmp/aesdsocketdata"
#define BACKLOG 10

int server_socket;
int graceful_stop = 0;

int sock_to_peer(int sockfd, char *buf, size_t buf_size)
{
    // Get the peer address information
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    if (getpeername(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1)
    {
        perror("getpeername failed");
        return -1;
    }

    // Convert the client address to a string representation
    char clientIP[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN) == NULL)
    {
        perror("inet_ntop failed");
        return -1;
    }
    strncpy(buf, clientIP, buf_size);
    return 0;
}

// returns socket fd or -1 on error 
int get_listening_socket(char*port) {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        // syslog(LOG_ERR, gai_strerror(status));
        return -1;
    }
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        perror("socket()");
        return -1;
    }
    syslog(LOG_DEBUG, "socket() ok");

    status = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (status == -1)
    {
        perror("bind()");
        return -1;
    }
    syslog(LOG_DEBUG, "bind() ok");

    status = listen(sockfd, BACKLOG);
    if (status == -1)
    {
        perror("listen()");
        return -1;
    }
    syslog(LOG_DEBUG, "listening on %s...", port);
    freeaddrinfo(res);
    return sockfd;
}

int send_file_content(char *filename, int sockfd)
{
    char buf[BUF_SIZE];
    size_t bytesRead;
    ssize_t bytesSent;
    FILE *readfile = fopen(filename, "rb");
    while ((bytesRead = fread(buf, 1, sizeof(buf), readfile)) > 0)
    {
        bytesSent = send(sockfd, buf, bytesRead, 0);
        if (bytesSent == -1)
        {
            perror("send failed");
            fclose(readfile);
            return -1;
        }
    }
    fclose(readfile);
    return 0;
}

// return client fd
int accept_connection(int sockfd)
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    int status;
    char buf[BUF_SIZE];

    int clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    if (clientfd == -1)
    {
        perror("accept()");
        return -1;
    }
    status = sock_to_peer(clientfd, buf, INET_ADDRSTRLEN);
    if (status == -1)
    {
        fprintf(stderr, "sock_to_peer(): failed to get peer from socket\n");
        return -1;
    }

    syslog(LOG_DEBUG, "Accepted connection from %s", buf);
    return clientfd;
}

int handle_client_connection(int clientfd) {
    char buf[BUF_SIZE];
    char peername[INET_ADDRSTRLEN + 10];
    int status;
    status = sock_to_peer(clientfd, peername, INET_ADDRSTRLEN);
    if (status == -1)
    {
        fprintf(stderr, "sock_to_peer(): failed to get peer from socket\n");
        return -1;
    }

    do
    {
        FILE *file = fopen(VARTMPFILE, "a");
        ssize_t bytesRead = recv(clientfd, buf, sizeof(buf) - 1, 0);
        syslog(LOG_DEBUG, "Bytes read %ld", bytesRead);
        
        if (bytesRead == -1)
        {
            perror("recv failed");
            return -1;
        }
        fwrite(buf, 1, bytesRead, file);
        fclose(file);

        status = send_file_content(VARTMPFILE, clientfd);
        if (status == -1)
        {
            fprintf(stderr, "handle_client_connection(): error writing to file %s\n", VARTMPFILE);
            return -1;
        }

        status = bytesRead;
    } while (status != 0); // bytesRead != 0
    syslog(LOG_DEBUG, "convert sock to peer");

    if (close(clientfd) == -1)
    {
        perror("close failed");
        return -1;
    }
    syslog(LOG_DEBUG, "Close connection from %s", peername);
    return 0;
}

int run() {
    int status;
    openlog("aesdsocket", LOG_PID, LOG_USER);
    server_socket = get_listening_socket(THEPORT);
    if (server_socket == -1)
    {
        perror("get_listening_socket()");
        return -1;
    }

    while (1)
    {
        int clientfd = accept_connection(server_socket);
        if (clientfd == -1)
        {
            fprintf(stderr, " accept_connection() failed\n");
            return -1;
        }

        if (!graceful_stop)
            status = handle_client_connection(clientfd);
        
        if (status != 0)
        {
            fprintf(stderr, "handle_client_connection, status = %d\n", status);
            return -1;
        }
    }

    closelog();
    // Close the socket
    if (close(server_socket) == -1)
    {
        perror("close failed");
        return -1;
    }
    return 0;
}

// Signal handler function
void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        close(server_socket);
    }
}

int main()
{
    return run();
}