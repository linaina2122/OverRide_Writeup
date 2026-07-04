
int main(void) {
    char input_buffer[128];
    int status = 0;
    long syscall_num = 0;
    memset(input_buffer, 0, sizeof(input_buffer));
    pid_t child_pid = fork();
    if (child_pid == 0) { // a way that child reconize itself
        prctl(PR_SET_PDEATHSIG, SIGKILL);  //if parent process stops or crash kernel sends a sigkill to child process 
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);  //child process here gives the power to parent process to watch it right and supervise it
        puts("Give me some shellcode, k");
        gets(input_buffer); 
    } else {
        while (1) {
            wait(&status);
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                puts("child is exiting...");
                return 0;
            }
            syscall_num = ptrace(PTRACE_PEEKUSER, child_pid, 0x2c, NULL);

            if (syscall_num == 11) {
                puts("no exec() for you");
                kill(child_pid, SIGKILL);
            }
        }
    }
    return 0;
}