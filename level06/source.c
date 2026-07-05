int auth(char *input, unsigned int expected_hash) {
    input[strcspn(input, "\n")] = '\0';
    int len = strnlen(input, 32);
    if (len <= 5) return 1;
    if (ptrace(0, 0, 0, 0) == -1) {
        puts("..."); puts("..."); puts("...");
        return 1;
    }
    unsigned int hash = (input[3] ^ 0x1337) + 0x5eeded;
    for (int i = 0; i < len; i++) {
        if (input[i] <= 0x1f) return 1; 
        
        unsigned int intermediate = input[i] ^ hash;
        unsigned int remainder = intermediate % 1337; 
        hash += remainder;
    }
    if (hash == expected_hash) {
        return 0; 
    }
    return 1; 
}
int main(int argc, char **argv) {
    char str[32];
    unsigned int number;


    puts("***********************************");
    puts("*             level06             *");
    puts("***********************************");
    printf("-> Enter Login: ");

    fgets(str, 32, 1);


    puts("***********************************");
    puts("***** NEW ACCOUNT DETECTED ********");
    puts("***********************************");
    printf("-> Enter Serial: ");
    scanf("%u", &number);
   if (auth(str, number) == 0) {
        puts("Authenticated successfully!");
        system("/bin/sh");                 
        }

    return 1;
}