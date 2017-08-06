#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int main(void) {
        char str[20] = "she is yi ! ge";
        //str[20] = '\0';
        char *token;
        char delim[3] = " !";
        
        token = strtok(str, delim);
        while(token != NULL) {
                printf("token = %s\n", token);
                token = strtok(NULL, delim);
        }
        return 0;
} 
