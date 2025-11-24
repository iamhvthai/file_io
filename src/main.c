#include "file_operations.h"

// Clear screen (cross-platform approach)
void clear_screen() {
    printf("\033[2J\033[H");
}

// Display menu
void display_menu() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║     SYSTEM PROGRAMMING - FILE/FOLDER COPY UTILITY      ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  📁 BASIC OPERATIONS\n");
    printf("  [1] Copy a File\n");
    printf("  [2] Copy a Directory (Recursive)\n");
    printf("  [3] Move a File\n");
    printf("  [4] Move a Directory\n");
    printf("\n");
    printf("  🔍 ADVANCED OPERATIONS\n");
    printf("  [5] Copy with Pattern Filter\n");
    printf("  [6] Compare Two Files\n");
    printf("  [7] Calculate File Checksum\n");
    printf("  [8] Verify File Checksum\n");
    printf("\n");
    printf("  ℹ️  INFORMATION & NAVIGATION\n");
    printf("  [9] Check if Path Exists\n");
    printf("  [10] Get File/Directory Information\n");
    printf("  [11] File Explorer (Browse Files/Folders)\n");
    printf("  [12] List Directory Contents\n");
    printf("\n");
    printf("  [0] Exit\n");
    printf("\n");
    printf("────────────────────────────────────────────────────────\n");
    printf("Enter your choice: ");
}

// Get user input with prompt
void get_input(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin) != NULL) {
        // Remove trailing newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
    }
}

// Display file/directory information
void display_info(const char *path) {
    struct stat st;
    
    if (!path_exists(path)) {
        printf("\n❌ Path does not exist: %s\n", path);
        return;
    }
    
    if (stat(path, &st) != 0) {
        printf("\n❌ Cannot get information for: %s\n", path);
        return;
    }
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                   PATH INFORMATION                     ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  Path: %s\n", path);
    printf("  Type: %s\n", S_ISDIR(st.st_mode) ? "Directory" : "File");
    
    if (S_ISREG(st.st_mode)) {
        printf("  Size: %ld bytes", st.st_size);
        if (st.st_size >= 1024 * 1024) {
            printf(" (%.2f MB)", st.st_size / (1024.0 * 1024.0));
        } else if (st.st_size >= 1024) {
            printf(" (%.2f KB)", st.st_size / 1024.0);
        }
        printf("\n");
    }
    
    printf("  Permissions: ");
    printf((S_ISDIR(st.st_mode)) ? "d" : "-");
    printf((st.st_mode & S_IRUSR) ? "r" : "-");
    printf((st.st_mode & S_IWUSR) ? "w" : "-");
    printf((st.st_mode & S_IXUSR) ? "x" : "-");
    printf((st.st_mode & S_IRGRP) ? "r" : "-");
    printf((st.st_mode & S_IWGRP) ? "w" : "-");
    printf((st.st_mode & S_IXGRP) ? "x" : "-");
    printf((st.st_mode & S_IROTH) ? "r" : "-");
    printf((st.st_mode & S_IWOTH) ? "w" : "-");
    printf((st.st_mode & S_IXOTH) ? "x" : "-");
    printf("\n");
    
    char time_str[100];
    struct tm *timeinfo = localtime(&st.st_mtime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    printf("  Last Modified: %s\n", time_str);
    printf("\n");
}

// Handle file copy operation
void handle_file_copy() {
    char src[MAX_PATH], dest[MAX_PATH];
    int result;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                      COPY FILE                         ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  Source file path: ", src, MAX_PATH);

    if (!path_exists(src)) {
        printf("\n❌ Source file does not exist!\n");
        return;
    }

    if (is_directory(src)) {
        printf("\n❌ Source is a directory! Use option 2 to copy directories.\n");
        return;
    }

    get_input("  Destination path (file or folder): ", dest, MAX_PATH);

    // Check if destination is a directory
    if (path_exists(dest) && is_directory(dest)) {
        const char *filename = strrchr(src, '/');
        if (filename == NULL) {
            filename = src;
        } else {
            filename++;
        }
        printf("\n💡 Destination is a folder. File will be copied as: %s/%s\n", dest, filename);
    }

    printf("\n");
    printf("📋 Copying file...\n");
    printf("────────────────────────────────────────────────────────\n");

    clock_t start = clock();
    result = copy_file(src, dest);
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    if (result == SUCCESS) {
        printf("✅ File copied successfully!\n");
        printf("⏱️  Time taken: %.3f seconds\n", time_spent);
    } else {
        print_error(result, "File copy failed");
    }
}

// Handle directory copy operation
void handle_directory_copy() {
    char src[MAX_PATH], dest[MAX_PATH];
    int result;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                   COPY DIRECTORY                       ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  Source directory path: ", src, MAX_PATH);

    if (!path_exists(src)) {
        printf("\n❌ Source directory does not exist!\n");
        return;
    }

    if (!is_directory(src)) {
        printf("\n❌ Source is not a directory! Use option 1 to copy files.\n");
        return;
    }

    get_input("  Destination directory path: ", dest, MAX_PATH);

    printf("\n");
    printf("📁 Copying directory recursively...\n");
    printf("────────────────────────────────────────────────────────\n");

    clock_t start = clock();
    result = copy_directory(src, dest);
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    if (result == SUCCESS) {
        printf("✅ Directory copied successfully!\n");
        printf("⏱️  Time taken: %.3f seconds\n", time_spent);
    } else {
        print_error(result, "Directory copy failed");
    }
}

// Handle path existence check
void handle_path_check() {
    char path[MAX_PATH];

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                  CHECK PATH EXISTS                     ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  Enter path to check: ", path, MAX_PATH);

    printf("\n");
    if (path_exists(path)) {
        printf("✅ Path exists: %s\n", path);
        printf("   Type: %s\n", is_directory(path) ? "Directory" : "File");
    } else {
        printf("❌ Path does not exist: %s\n", path);
    }
}

// Handle file move operation
void handle_file_move() {
    char src[MAX_PATH], dest[MAX_PATH];
    int result;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                      MOVE FILE                         ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  Source file path: ", src, MAX_PATH);

    if (!path_exists(src)) {
        printf("\n❌ Source file does not exist!\n");
        return;
    }

    if (is_directory(src)) {
        printf("\n❌ Source is a directory! Use option 4 to move directories.\n");
        return;
    }

    get_input("  Destination file path: ", dest, MAX_PATH);

    printf("\n");
    printf("📦 Moving file...\n");
    printf("────────────────────────────────────────────────────────\n");

    result = move_file(src, dest);

    if (result == SUCCESS) {
        printf("✅ File moved successfully!\n");
    } else {
        print_error(result, "File move failed");
    }
}

// Handle directory move operation
void handle_directory_move() {
    char src[MAX_PATH], dest[MAX_PATH];
    int result;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                   MOVE DIRECTORY                       ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  Source directory path: ", src, MAX_PATH);

    if (!path_exists(src)) {
        printf("\n❌ Source directory does not exist!\n");
        return;
    }

    if (!is_directory(src)) {
        printf("\n❌ Source is not a directory! Use option 3 to move files.\n");
        return;
    }

    get_input("  Destination directory path: ", dest, MAX_PATH);

    printf("\n");
    printf("📁 Moving directory...\n");
    printf("────────────────────────────────────────────────────────\n");

    result = move_directory(src, dest);

    if (result == SUCCESS) {
        printf("✅ Directory moved successfully!\n");
    } else {
        print_error(result, "Directory move failed");
    }
}

// Handle filtered copy operation
void handle_filtered_copy() {
    char src[MAX_PATH], dest[MAX_PATH];
    char include_input[256], exclude_input[256];
    const char *include_patterns[MAX_PATTERNS + 1] = {NULL};
    const char *exclude_patterns[MAX_PATTERNS + 1] = {NULL};
    CopyStats stats;
    int result;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║              COPY WITH PATTERN FILTER                  ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  Source directory path: ", src, MAX_PATH);

    if (!path_exists(src)) {
        printf("\n❌ Source directory does not exist!\n");
        return;
    }

    get_input("  Destination directory path: ", dest, MAX_PATH);

    printf("\n  Include patterns (comma-separated, e.g., *.txt,*.pdf):\n");
    get_input("  ", include_input, sizeof(include_input));

    printf("  Exclude patterns (comma-separated, e.g., *.tmp,*.log):\n");
    get_input("  ", exclude_input, sizeof(exclude_input));

    // Parse include patterns
    int inc_count = 0;
    if (strlen(include_input) > 0) {
        char *token = strtok(include_input, ",");
        while (token != NULL && inc_count < MAX_PATTERNS) {
            // Trim whitespace
            while (*token == ' ') token++;
            include_patterns[inc_count++] = token;
            token = strtok(NULL, ",");
        }
    }

    // Parse exclude patterns
    int exc_count = 0;
    if (strlen(exclude_input) > 0) {
        char *token = strtok(exclude_input, ",");
        while (token != NULL && exc_count < MAX_PATTERNS) {
            while (*token == ' ') token++;
            exclude_patterns[exc_count++] = token;
            token = strtok(NULL, ",");
        }
    }

    printf("\n");
    printf("📁 Copying with filters...\n");
    printf("────────────────────────────────────────────────────────\n");

    init_stats(&stats);

    if (is_directory(src)) {
        result = copy_directory_filtered(src, dest,
                                        inc_count > 0 ? include_patterns : NULL,
                                        exc_count > 0 ? exclude_patterns : NULL,
                                        &stats);
    } else {
        result = copy_file_filtered(src, dest,
                                   inc_count > 0 ? include_patterns : NULL,
                                   exc_count > 0 ? exclude_patterns : NULL,
                                   &stats);
    }

    if (result == SUCCESS) {
        printf("✅ Copy completed successfully!\n");
        display_stats(&stats);
    } else {
        print_error(result, "Filtered copy failed");
    }
}

// Handle file comparison
void handle_file_comparison() {
    char file1[MAX_PATH], file2[MAX_PATH];
    int result;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                   COMPARE TWO FILES                    ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  First file path: ", file1, MAX_PATH);
    get_input("  Second file path: ", file2, MAX_PATH);

    if (!path_exists(file1) || !path_exists(file2)) {
        printf("\n❌ One or both files do not exist!\n");
        return;
    }

    printf("\n");
    printf("🔍 Comparing files...\n");
    printf("────────────────────────────────────────────────────────\n");

    clock_t start = clock();
    result = compare_files(file1, file2);
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    if (result == SUCCESS) {
        printf("✅ Files are identical!\n");
        printf("⏱️  Comparison time: %.3f seconds\n", time_spent);
    } else if (result == ERROR_FILES_DIFFER) {
        printf("❌ Files are different!\n");
    } else {
        print_error(result, "File comparison failed");
    }
}

// Handle checksum calculation
void handle_checksum_calculation() {
    char filepath[MAX_PATH];
    char checksum[33];
    int result;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                CALCULATE FILE CHECKSUM                 ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  File path: ", filepath, MAX_PATH);

    if (!path_exists(filepath)) {
        printf("\n❌ File does not exist!\n");
        return;
    }

    printf("\n");
    printf("🔐 Calculating checksum...\n");
    printf("────────────────────────────────────────────────────────\n");

    result = calculate_md5(filepath, checksum);

    if (result == SUCCESS) {
        printf("✅ Checksum calculated successfully!\n");
        printf("📝 Checksum: %s\n", checksum);
        printf("\n💡 Save this checksum to verify file integrity later.\n");
    } else {
        print_error(result, "Checksum calculation failed");
    }
}

// Handle checksum verification
void handle_checksum_verification() {
    char filepath[MAX_PATH];
    char expected_checksum[33];
    int result;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                 VERIFY FILE CHECKSUM                   ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  File path: ", filepath, MAX_PATH);

    if (!path_exists(filepath)) {
        printf("\n❌ File does not exist!\n");
        return;
    }

    get_input("  Expected checksum: ", expected_checksum, sizeof(expected_checksum));

    printf("\n");
    printf("🔍 Verifying checksum...\n");
    printf("────────────────────────────────────────────────────────\n");

    result = verify_checksum(filepath, expected_checksum);

    if (result == SUCCESS) {
        printf("✅ Checksum verified! File integrity is intact.\n");
    } else if (result == ERROR_FILES_DIFFER) {
        printf("❌ Checksum mismatch! File may be corrupted or modified.\n");
    } else {
        print_error(result, "Checksum verification failed");
    }
}

// Handle file explorer
void handle_file_explorer() {
    char start_path[MAX_PATH];

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                    FILE EXPLORER                       ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  Enter starting path (or press Enter for current directory): ", start_path, MAX_PATH);

    if (strlen(start_path) == 0) {
        getcwd(start_path, MAX_PATH);
    }

    if (!path_exists(start_path)) {
        printf("\n❌ Path does not exist!\n");
        return;
    }

    if (!is_directory(start_path)) {
        printf("\n❌ Path is not a directory!\n");
        return;
    }

    browse_filesystem(start_path);
}

// Handle list directory
void handle_list_directory() {
    char dir_path[MAX_PATH];
    int result;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                  LIST DIRECTORY                        ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    get_input("  Directory path: ", dir_path, MAX_PATH);

    if (!path_exists(dir_path)) {
        printf("\n❌ Directory does not exist!\n");
        return;
    }

    if (!is_directory(dir_path)) {
        printf("\n❌ Path is not a directory!\n");
        return;
    }

    result = list_directory(dir_path);

    if (result != SUCCESS) {
        print_error(result, "Failed to list directory");
    }
}

// Main function
int main(int argc, char *argv[]) {
    int choice;
    char input[10];
    char path[MAX_PATH];

    // Handle command line arguments
    if (argc == 3) {
        printf("Command line mode: Copying %s to %s\n", argv[1], argv[2]);

        if (!path_exists(argv[1])) {
            fprintf(stderr, "Error: Source path does not exist!\n");
            return 1;
        }

        // Check if destination is a directory and source is a file
        if (!is_directory(argv[1]) && path_exists(argv[2]) && is_directory(argv[2])) {
            const char *filename = strrchr(argv[1], '/');
            if (filename == NULL) {
                filename = argv[1];
            } else {
                filename++;
            }
            printf("💡 Destination is a folder. File will be copied as: %s/%s\n", argv[2], filename);
        }

        int result;
        if (is_directory(argv[1])) {
            result = copy_directory(argv[1], argv[2]);
        } else {
            result = copy_file(argv[1], argv[2]);
        }

        if (result == SUCCESS) {
            printf("✅ Copy completed successfully!\n");
            return 0;
        } else {
            print_error(result, "Copy failed");
            return 1;
        }
    }

    // Interactive mode
    while (1) {
        clear_screen();
        display_menu();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        choice = atoi(input);

        switch (choice) {
            case 1:
                handle_file_copy();
                break;
            case 2:
                handle_directory_copy();
                break;
            case 3:
                handle_file_move();
                break;
            case 4:
                handle_directory_move();
                break;
            case 5:
                handle_filtered_copy();
                break;
            case 6:
                handle_file_comparison();
                break;
            case 7:
                handle_checksum_calculation();
                break;
            case 8:
                handle_checksum_verification();
                break;
            case 9:
                handle_path_check();
                break;
            case 10:
                printf("\n");
                get_input("  Enter path: ", path, MAX_PATH);
                display_info(path);
                break;
            case 11:
                handle_file_explorer();
                break;
            case 12:
                handle_list_directory();
                break;
            case 0:
                printf("\n👋 Goodbye! Thank you for using File Copy Utility.\n\n");
                return 0;
            default:
                printf("\n❌ Invalid choice! Please try again.\n");
                break;
        }

        printf("\nPress Enter to continue...");
        getchar();
    }

    return 0;
}

