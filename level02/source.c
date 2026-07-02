
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char username[100] = {0};
    char password_file_content[48] = {0};
    char input_password[112] = {0};
    FILE *file_ptr = NULL;
    size_t read_bytes;

    file_ptr = fopen("/home/users/level03/.pass", "r");
    if (file_ptr == NULL) {
        fwrite("ERROR: failed to open password file\n", 1, 36, stderr);
        exit(1);
    }
    read_bytes = fread(password_file_content, 1, 41, file_ptr);
    
    password_file_content[strcspn(password_file_content, "\n")] = '\0';

    if (read_bytes != 41) {
        fwrite("ERROR: failed to read password file\n", 1, 36, stderr);
        exit(1);
    }
    fclose(file_ptr);

    puts("===== [ Secure Access System v1.0 ] =====");
    puts("/***************************************\\");
    puts("| You must login to access this system. |");
    puts("\\**************************************/");


    printf("--[ Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0'; 

  
    printf("--[ Password: ");
    fgets(input_password, sizeof(input_password), stdin);
    input_password[strcspn(input_password, "\n")] = '\0';

    puts("*****************************************");


    if (strncmp(password_file_content, input_password, 41) == 0) {
        printf("Greetings, %s!\n", username);
        system("/bin/sh");
        return 0;
    }

    printf(username); 
    puts(" does not have access!");

    exit(1);
}