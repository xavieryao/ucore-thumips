#include <ulib.h>
#include <file.h>
#include <stat.h>
#include <unistd.h>

#define BUFSIZE                         4096

int
put(int fd) {
    static char buffer[BUFSIZE];
    int fileSize = 0;
    if(read(STDIN_FILENO, &fileSize, sizeof(fileSize)) < sizeof(fileSize)) {
      return EXIT_FAILURE;
    }
    int ret1, ret2;
    while ((ret1 = read(STDIN_FILENO, buffer, sizeof(buffer) > fileSize ? fileSize : sizeof(buffer))) > 0) {
        if ((ret2 = write(fd, buffer, ret1)) != ret1) {
            return ret2;
        }
        fileSize -= ret1;
    }
    return ret1;
}

int
main(int argc, char **argv) {
    if (argc == 1) {
        return put(STDOUT_FILENO);
    }
    else {
        int i, ret;
        for (i = 1; i < argc; i ++) {
            if ((ret = open(argv[i], O_WRONLY)) < 0) {
                return ret;
            }
            if ((ret = put(ret)) != 0) {
                return ret;
            }
        }
    }
    return 0;
}

