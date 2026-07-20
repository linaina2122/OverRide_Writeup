#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char buf[100];  
    int i;

    fgets(buf, 100, stdin); 

    for (i = 0; buf[i] != 0; i++) {
        if (buf[i] > 0x40 && buf[i] <= 0x5a) {  
            buf[i] ^= 0x20;                       
        }
    }

    printf(buf);   
    exit(0);
}
