#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#define PERMS 0666
#define FILESIZE 4096
const char buf[FILESIZE];

int main(int argc , char *argv[]) {
    int ret;
    if(argc != 3) {
        ret = -1;
        goto error;
    }
    char *dir = argv[1];
    int file_num = atoi(argv[2]);
    int fn;
    char num_str[20];
    char path[100];
    int i;

    for(i=0; i<file_num; i++) {
        memset(num_str, 0, sizeof num_str);
        memset(path, 0, sizeof path);
        strcat(path, dir);
        int last_c = strlen(path) - 1;
        if(path[last_c] != '/')
            path[last_c+1] = '/';
        sprintf(num_str, "%d", i);
        strcat(path, num_str);
        printf("going to creat file %s\n", path);
        if((fn = creat(path, PERMS)) == -1) {
            ret = -2;
            goto error;
        }
        if(write(fn, buf, FILESIZE) != FILESIZE) {
            ret = -3;
            goto error;
        }
        if(close(fn) < 0) {
            ret = -4;
            goto error;
        }
    }
    return 0;

error:
    printf("Error ret = %d\n", ret);
    return ret;
}
