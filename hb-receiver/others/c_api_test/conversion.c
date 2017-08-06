#include <stdio.h>

int main(void) {
        unsigned long a = 0xFFFFFFFFFFFFFFFF;
        int *p_char = &a;
            
        printf("sizes of long and int are %lu %lu\n", sizeof(long), sizeof(int));
        
        printf("a = %lx\n", a);
        
        long _a = *(p_char); 
        printf("_a = %lx\n", _a);
        
        return 0;
}
