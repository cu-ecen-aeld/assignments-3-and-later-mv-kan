/*
1 - getaddrinfo() to get address info automatically
2 - socket() get file descriptor
3 - bind() bind socket to address port
4 - listen()
5 - accept() creates new socket file descriptor dedicated for client
 
*/
// 
// 
/*
showip.c -- show IP addresses for a host given on the command line
*/
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

#define MYPORT "9000"
#define BACKLOG 10
#define BUFFER_SIZE 1024
void log_debug(const char *format, ...)
{
    // Create a buffer to hold the formatted log message
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Format the log message using variadic arguments
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Print the log message
    syslog(LOG_DEBUG, "%s", buffer);
}

int socktopeer(int sockfd, char*buf, size_t buf_size) {
    // Get the peer address information
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    if (getpeername(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1)
    {
        perror("getpeername failed");
        return 1;
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


int main(int argc, char *argv[])
{
    openlog("aesdsocket", LOG_PID, LOG_USER);
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(NULL, MYPORT, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        // syslog(LOG_ERR, gai_strerror(status));
        return -1;
    }
    int errnum;
    int socketfd;
    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socketfd == -1) {
        errnum = errno;
        fprintf(stderr, "socket(): %s\n", strerror(errnum));
        // syslog(LOG_ERR, gai_strerror(status));
        return -1;
    }
    log_debug("socket()");

    status = bind(socketfd, res->ai_addr, res->ai_addrlen);
    if (status == -1) {
        errnum = errno;
        fprintf(stderr, "bind(): %s\n", strerror(errnum));
        return -1;
    }
    log_debug("binded");

    status = listen(socketfd, BACKLOG);
    if (status == -1)
    {
        errnum = errno;
        fprintf(stderr, "listen(): %s\n", strerror(errnum));
        return -1;
    }
    log_debug("listening...");

    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);

    int acceptfd = accept(socketfd, (struct sockaddr*)&their_addr, &addr_size);
    if (acceptfd == -1)
    {
        errnum = errno;
        fprintf(stderr, "accept(): %s\n", strerror(errnum));
        return -1;
    }
    char buf[BUFFER_SIZE];
    log_debug("Accepted connection from %s", socktopeer(acceptfd, buf, INET_ADDRSTRLEN));
    FILE* file = fopen("/var/tmp/aesdsocketdata", "a");
    do {
        ssize_t bytesRead = recv(acceptfd, buf, sizeof(buf) - 1, 0);
        log_debug("Bytes read %d", bytesRead);
        if (bytesRead == -1)
        {
            perror("recv failed");
            return -1;
        }
        status = bytesRead;
        fwrite(buf, 1, bytesRead, file);
        fprintf(file, "\n");
    } while (status != 0);
    fclose(file);
    // Print the received data
    printf("Received data: %s\n", buf);
    size_t bytesRead;
    ssize_t bytesSent;
    FILE*readfile = fopen("/var/tmp/aesdsocketdata", "rb");
    while ((bytesRead = fread(buf, 1, sizeof(buf), readfile)) > 0)
    {
        bytesSent = send(sockfd, buf, bytesRead, 0);
        if (bytesSent == -1)
        {
            perror("send failed");
            fclose(file);
            close(sockfd);
            return -1;
        }
    }
    fclose(readfile);
    closelog();
    // Close the socket
    if (close(socketfd) == -1)
    {
        perror("close failed");
        return -1;
    }
    if (close(acceptfd) == -1)
    {
        perror("close failed");
        return -1;
    }
    log_debug("Close connection from %s", socktopeer(acceptfd, buf, INET_ADDRSTRLEN));
    freeaddrinfo(res); // free the linked list
    return 0;
}