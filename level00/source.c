
int main ()
{
    int number;
    puts("***********************************\n");
    puts("*         -level00                *\n");
    puts("***********************************\n");
    printf("Password:");
    scanf("%d", &number);
    if(number != 5276){
        puts("\nInvalid Password!");
        return(1);
    }
    puts("\nAuthenticated!");
    system("/bin/sh");
    return(0);
}