#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>

int writetofile(const char* filepath, const char* str) {
    openlog("finder-app", LOG_PID, LOG_USER);
    int fd = open(filepath, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        syslog(LOG_ERR, "unable to create file, error code: %d", fd);
        return -1;
    }
    char tmp;
    size_t count = 0;
    do {
        tmp = str[count];
        count++;
    } while (tmp != '\0');
    
    write(fd, str, count);
    syslog(LOG_DEBUG, "Writing \"%s\" to \"%s\" file", str, filepath);
    closelog();
    int err = close(fd);
    if (err == -1) {
        syslog(LOG_ERR, "unable to close file, error code: %d", err);
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("you have to define first and second arguments\n");
        return 1;
    }
    int err = writetofile(argv[1], argv[2]);
    return err;
}