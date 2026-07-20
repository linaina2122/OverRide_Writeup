
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    int pass; 


    srand(time(NULL));
    puts("***********************************\n");    
    puts("*		level03		**\n"); 
    puts("***********************************\n");    
    printf("Password:");  

    scanf("%d", &pass);

    test(pass, 322424845);

    return 0;
}

void decrypt(int code) {
    char encrypted_str[] = "\x51\x7d\x7c\x75"  // 0x757c7d51
                           "\x60\x73\x66\x67"  // 0x67667360
                           "\x7e\x73\x66\x7b"  // 0x7b66737e
                           "\x7d\x7c\x61\x33"; // 0x33617c7d

    int len = strlen(encrypted_str); 

    for (int i = 0; i < len; i++) {
        encrypted_str[i] = encrypted_str[i] ^ code;
    }

    if (strcmp(encrypted_str, (char*)0x80489c3) == 0) {
        system("/bin/sh");
    } else {
        puts((char*)0x80489dc);
    }
    
}

void test(int user_input, int magic_number) {
    int result = magic_number - user_input; 

    if ((unsigned int)result > 21) { 
        int random_val = rand();
        decrypt(random_val);
        return;
    }
    switch (result) {
        case 1:  decrypt(result); break;
        case 2:  decrypt(result); break;
        case 3:  decrypt(result); break;
        case 4:  decrypt(result); break;
        case 5:  decrypt(result); break;
        case 6:  decrypt(result); break;
        case 7:  decrypt(result); break;
        case 8:  decrypt(result); break;
        case 9:  decrypt(result); break;
        case 10: decrypt(result); break;
        case 11: decrypt(result); break;
        case 12: decrypt(result); break;
        case 13: decrypt(result); break;
        case 14: decrypt(result); break;
        case 15: decrypt(result); break;
        case 16: decrypt(result); break;
        case 17: decrypt(result); break;
        case 18: decrypt(result); break;
        case 19: decrypt(result); break;
        case 20: decrypt(result); break;
        case 21: decrypt(result); break;
        default: 
            decrypt(result); 
            break;
    }
}