
#include <sys/types.h> //头文件
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

int main() {
    int fd = open("/home/zhang/Search_Engine/data/ripepage.dat",O_RDWR);
    if (fd == -1) {
        printf("open file error %d \n",errno);
    }
    //3674 2002
    int ret = lseek(fd,3674, SEEK_SET);
    char buf[65535] = {0,};
    size_t len = 2002;
    read(fd, buf, len);
    printf("%s", buf);
    close(fd);
    return 0;
}
