


void log_wrapper(FILE *log_file, const char *prefix, const char *filename) {
    char log_buffer[264];
    size_t newline_pos;
    strcpy(log_buffer, prefix);
    size_t prefix_len = strlen(log_buffer);
    snprintf(log_buffer + prefix_len, 254 - prefix_len, filename);
    newline_pos = strcspn(log_buffer, "\n");
    log_buffer[newline_pos] = '\0';
    fprintf(log_file, "LOG: %s\n", log_buffer);
}

int main(int argc, char **argv) {
    FILE *log_stream;
    FILE *src_stream;
    int dest_fd;
    int ch;
    char target_path[104];

    if (argc != 2) {
        printf("Usage: %s filename\n", argv[0]);
    }

    log_stream = fopen("./backups/.log", "w");
    if (log_stream == NULL) {
        printf("ERROR: Failed to open %s\n", "./backups/.log");
        exit(1);
    }

    log_wrapper(log_stream, "Starting back up: ", argv[1]);

    src_stream = fopen(argv[1], "r");
    if (src_stream == NULL) {
        printf("ERROR: Failed to open %s\n", argv[1]);
        exit(1);
    }

    strncpy(target_path, "./backups/", 11);
    
    size_t current_len = strlen(target_path);
    strncat(target_path, argv[1], 99 - current_len);

    dest_fd = open(target_path, O_WRONLY | O_CREAT | O_TRUNC, 0660);
    if (dest_fd < 0) {
        printf("ERROR: Failed to open %s%s\n", "./backups/", argv[1]);
        exit(1);
    }

    while (true) {
        ch = fgetc(src_stream);
        if (ch == EOF) {
            break;
        }
        write(dest_fd, &ch, 1);
    }

    log_wrapper(log_stream, "Finished back up ", argv[1]);
    fclose(src_stream);
    close(dest_fd);
    return 0;
}