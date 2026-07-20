
int verify_user_name(char *user) {
    char *str =  "dat_wil";
    puts("verifying username....\n");
    int i = strncmp(str, user, 7);
    return (i);
}

int verify_user_pass (char *pass){
    char *str = "admin";
    int i = strncmp(str, pass, 5);
    return (i);
}

int main(){
    char *str;
    char *str2;
    char *buff1;
    char *buff2;
    
    memset(str, 0, 16);
    puts("********* ADMIN LOGIN PROMPT *********");
    printf("Enter Username: ");
    
    str = fgets(buff1, 256, 0);
    int res = verify_user_name(str);
    
    if(res == 0) {
        puts("Enter Password: ");
        str2 = fgets(buff2, 100 ,0);
        
        int res2 = verify_user_pass(str2);
        if(res2 == 0)
        {
            puts("Incorrect password."); 
            return 1;
        }
        return 0;
    }
    else {
        puts("Incorrect username.");
        return 1;
    }
}