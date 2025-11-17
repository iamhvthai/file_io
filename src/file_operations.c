#include "file_operations.h"
#include <fnmatch.h>
#include <pwd.h>
#include <grp.h>

// Global statistics for tracking
static CopyStats global_stats;

// Check if a path exists
int path_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

// Check if a path is a directory
int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

// Get file size in bytes
long get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    return st.st_size;
}

// Create directory with parent directories if needed
int create_directory(const char *path) {
    char tmp[MAX_PATH];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (!path_exists(tmp)) {
                if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                    return ERROR_DIR_CREATE;
                }
            }
            *p = '/';
        }
    }
    
    if (!path_exists(tmp)) {
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            return ERROR_DIR_CREATE;
        }
    }
    
    return SUCCESS;
}

// Display copy progress
void display_progress(long current, long total, const char *filename) {
    if (total <= 0) {
        printf("\rCopying: %s... ", filename);
        fflush(stdout);
        return;
    }
    
    int percent = (int)((current * 100) / total);
    int bar_width = 50;
    int filled = (bar_width * current) / total;
    
    printf("\r[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf("=");
        else if (i == filled) printf(">");
        else printf(" ");
    }
    printf("] %d%% - %s", percent, filename);
    fflush(stdout);
}

// Copy a single file from source to destination
int copy_file(const char *src_path, const char *dest_path) {
    int src_fd, dest_fd;
    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    long total_size, copied = 0;
    char final_dest_path[MAX_PATH];
    struct stat dest_stat;

    // Get source file size for progress
    total_size = get_file_size(src_path);

    // Open source file
    src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        return ERROR_FILE_OPEN;
    }

    // Check if destination is a directory
    if (stat(dest_path, &dest_stat) == 0 && S_ISDIR(dest_stat.st_mode)) {
        // Destination is a directory, extract filename from source
        const char *filename = strrchr(src_path, '/');
        if (filename == NULL) {
            filename = src_path;  // No path separator, use whole string
        } else {
            filename++;  // Skip the '/'
        }

        // Build final destination path: dest_dir/filename
        snprintf(final_dest_path, MAX_PATH, "%s/%s", dest_path, filename);
    } else {
        // Destination is a file path, use as-is
        strncpy(final_dest_path, dest_path, MAX_PATH - 1);
        final_dest_path[MAX_PATH - 1] = '\0';
    }

    // Open/create destination file
    dest_fd = open(final_dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        close(src_fd);
        return ERROR_FILE_OPEN;
    }
    
    // Copy file content
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            close(src_fd);
            close(dest_fd);
            return ERROR_FILE_WRITE;
        }
        copied += bytes_written;
        display_progress(copied, total_size, src_path);
    }
    
    printf("\n");
    
    if (bytes_read < 0) {
        close(src_fd);
        close(dest_fd);
        return ERROR_FILE_READ;
    }
    
    // Copy file permissions
    struct stat st;
    if (fstat(src_fd, &st) == 0) {
        fchmod(dest_fd, st.st_mode);
    }
    
    close(src_fd);
    close(dest_fd);
    
    return SUCCESS;
}

// Copy a directory recursively from source to destination
int copy_directory(const char *src_path, const char *dest_path) {
    DIR *dir;
    struct dirent *entry;
    char src_file[MAX_PATH];
    char dest_file[MAX_PATH];
    int result;

    // Create destination directory
    result = create_directory(dest_path);
    if (result != SUCCESS) {
        return result;
    }

    // Open source directory
    dir = opendir(src_path);
    if (dir == NULL) {
        return ERROR_DIR_OPEN;
    }

    printf("Copying directory: %s -> %s\n", src_path, dest_path);

    // Iterate through directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Build full paths
        snprintf(src_file, MAX_PATH, "%s/%s", src_path, entry->d_name);
        snprintf(dest_file, MAX_PATH, "%s/%s", dest_path, entry->d_name);

        // Check if entry is a directory
        if (is_directory(src_file)) {
            // Recursively copy subdirectory
            result = copy_directory(src_file, dest_file);
            if (result != SUCCESS) {
                closedir(dir);
                return result;
            }
        } else {
            // Copy file
            result = copy_file(src_file, dest_file);
            if (result != SUCCESS) {
                closedir(dir);
                return result;
            }
        }
    }

    closedir(dir);
    printf("Directory copied successfully: %s\n", dest_path);

    return SUCCESS;
}

// Print error message based on error code
void print_error(int error_code, const char *context) {
    fprintf(stderr, "Error");
    if (context != NULL && strlen(context) > 0) {
        fprintf(stderr, " (%s)", context);
    }
    fprintf(stderr, ": ");

    switch (error_code) {
        case ERROR_FILE_OPEN:
            fprintf(stderr, "Failed to open file - %s\n", strerror(errno));
            break;
        case ERROR_FILE_READ:
            fprintf(stderr, "Failed to read file - %s\n", strerror(errno));
            break;
        case ERROR_FILE_WRITE:
            fprintf(stderr, "Failed to write file - %s\n", strerror(errno));
            break;
        case ERROR_DIR_CREATE:
            fprintf(stderr, "Failed to create directory - %s\n", strerror(errno));
            break;
        case ERROR_DIR_OPEN:
            fprintf(stderr, "Failed to open directory - %s\n", strerror(errno));
            break;
        case ERROR_INVALID_PATH:
            fprintf(stderr, "Invalid path\n");
            break;
        case ERROR_MOVE_FAILED:
            fprintf(stderr, "Failed to move file/directory - %s\n", strerror(errno));
            break;
        case ERROR_FILES_DIFFER:
            fprintf(stderr, "Files are different\n");
            break;
        default:
            fprintf(stderr, "Unknown error (code: %d)\n", error_code);
            break;
    }
}

// ============================================================================
// PROGRESS STATISTICS IMPLEMENTATION
// ============================================================================

void init_stats(CopyStats *stats) {
    stats->total_files = 0;
    stats->total_dirs = 0;
    stats->total_bytes = 0;
    stats->copied_bytes = 0;
    stats->start_time = time(NULL);
    stats->current_time = stats->start_time;
    stats->transfer_speed = 0.0;
}

void update_stats(CopyStats *stats, long bytes) {
    stats->copied_bytes += bytes;
    stats->current_time = time(NULL);
    stats->transfer_speed = calculate_speed(stats);
}

double calculate_speed(const CopyStats *stats) {
    time_t elapsed = stats->current_time - stats->start_time;
    if (elapsed <= 0) return 0.0;
    return (double)stats->copied_bytes / elapsed;
}

long estimate_time_remaining(const CopyStats *stats) {
    if (stats->transfer_speed <= 0 || stats->total_bytes <= 0) return 0;
    long remaining_bytes = stats->total_bytes - stats->copied_bytes;
    return (long)(remaining_bytes / stats->transfer_speed);
}

void display_stats(const CopyStats *stats) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                  COPY STATISTICS                       ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  Files copied:      %ld\n", stats->total_files);
    printf("  Directories:       %ld\n", stats->total_dirs);
    printf("  Total bytes:       %ld", stats->total_bytes);

    if (stats->total_bytes >= 1024 * 1024 * 1024) {
        printf(" (%.2f GB)", stats->total_bytes / (1024.0 * 1024.0 * 1024.0));
    } else if (stats->total_bytes >= 1024 * 1024) {
        printf(" (%.2f MB)", stats->total_bytes / (1024.0 * 1024.0));
    } else if (stats->total_bytes >= 1024) {
        printf(" (%.2f KB)", stats->total_bytes / 1024.0);
    }
    printf("\n");

    time_t elapsed = stats->current_time - stats->start_time;
    printf("  Time elapsed:      %ld seconds\n", elapsed);

    if (stats->transfer_speed > 0) {
        printf("  Transfer speed:    ");
        if (stats->transfer_speed >= 1024 * 1024) {
            printf("%.2f MB/s", stats->transfer_speed / (1024.0 * 1024.0));
        } else if (stats->transfer_speed >= 1024) {
            printf("%.2f KB/s", stats->transfer_speed / 1024.0);
        } else {
            printf("%.2f B/s", stats->transfer_speed);
        }
        printf("\n");
    }

    if (stats->total_bytes > 0) {
        int percent = (int)((stats->copied_bytes * 100) / stats->total_bytes);
        printf("  Progress:          %d%%\n", percent);

        long eta = estimate_time_remaining(stats);
        if (eta > 0) {
            printf("  ETA:               %ld seconds\n", eta);
        }
    }
    printf("\n");
}

// ============================================================================
// FILE COMPARISON & CHECKSUM IMPLEMENTATION
// ============================================================================

int compare_files(const char *file1, const char *file2) {
    int fd1, fd2;
    ssize_t bytes1, bytes2;
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];

    // Check if both files exist
    if (!path_exists(file1) || !path_exists(file2)) {
        return ERROR_FILE_OPEN;
    }

    // Check file sizes first
    long size1 = get_file_size(file1);
    long size2 = get_file_size(file2);

    if (size1 != size2) {
        return ERROR_FILES_DIFFER;
    }

    // Open both files
    fd1 = open(file1, O_RDONLY);
    if (fd1 < 0) {
        return ERROR_FILE_OPEN;
    }

    fd2 = open(file2, O_RDONLY);
    if (fd2 < 0) {
        close(fd1);
        return ERROR_FILE_OPEN;
    }

    // Compare byte by byte
    while (1) {
        bytes1 = read(fd1, buffer1, BUFFER_SIZE);
        bytes2 = read(fd2, buffer2, BUFFER_SIZE);

        if (bytes1 < 0 || bytes2 < 0) {
            close(fd1);
            close(fd2);
            return ERROR_FILE_READ;
        }

        if (bytes1 != bytes2) {
            close(fd1);
            close(fd2);
            return ERROR_FILES_DIFFER;
        }

        if (bytes1 == 0) {
            // End of both files
            break;
        }

        if (memcmp(buffer1, buffer2, bytes1) != 0) {
            close(fd1);
            close(fd2);
            return ERROR_FILES_DIFFER;
        }
    }

    close(fd1);
    close(fd2);

    return SUCCESS;
}

// Simple MD5-like checksum (using a simple hash for demonstration)
// For production, use OpenSSL or similar library
int calculate_md5(const char *filepath, char *checksum) {
    int fd;
    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];
    unsigned long hash = 5381;
    unsigned long hash2 = 0;

    fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        return ERROR_FILE_OPEN;
    }

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            hash = ((hash << 5) + hash) + buffer[i]; // hash * 33 + c
            hash2 = hash2 * 31 + buffer[i];
        }
    }

    close(fd);

    if (bytes_read < 0) {
        return ERROR_FILE_READ;
    }

    // Format as hex string (simplified checksum)
    snprintf(checksum, 33, "%016lx%016lx", hash, hash2);

    return SUCCESS;
}

int verify_checksum(const char *filepath, const char *expected_checksum) {
    char actual_checksum[33];
    int result = calculate_md5(filepath, actual_checksum);

    if (result != SUCCESS) {
        return result;
    }

    if (strcmp(actual_checksum, expected_checksum) != 0) {
        return ERROR_FILES_DIFFER;
    }

    return SUCCESS;
}

// ============================================================================
// MOVE OPERATIONS IMPLEMENTATION
// ============================================================================

int remove_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char filepath[MAX_PATH];

    dir = opendir(path);
    if (dir == NULL) {
        return ERROR_DIR_OPEN;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(filepath, MAX_PATH, "%s/%s", path, entry->d_name);

        if (is_directory(filepath)) {
            remove_directory(filepath);
        } else {
            unlink(filepath);
        }
    }

    closedir(dir);
    rmdir(path);

    return SUCCESS;
}

int move_file(const char *src_path, const char *dest_path) {
    // Try rename first (works if same filesystem)
    if (rename(src_path, dest_path) == 0) {
        return SUCCESS;
    }

    // If rename fails (different filesystem), copy then delete
    if (errno == EXDEV) {
        int result = copy_file(src_path, dest_path);
        if (result != SUCCESS) {
            return result;
        }

        // Verify copy was successful before deleting
        result = compare_files(src_path, dest_path);
        if (result != SUCCESS) {
            unlink(dest_path); // Remove incomplete copy
            return ERROR_MOVE_FAILED;
        }

        // Delete source file
        if (unlink(src_path) != 0) {
            return ERROR_MOVE_FAILED;
        }

        return SUCCESS;
    }

    return ERROR_MOVE_FAILED;
}

int move_directory(const char *src_path, const char *dest_path) {
    // Try rename first
    if (rename(src_path, dest_path) == 0) {
        return SUCCESS;
    }

    // If rename fails, copy then delete
    if (errno == EXDEV) {
        int result = copy_directory(src_path, dest_path);
        if (result != SUCCESS) {
            return result;
        }

        // Delete source directory
        result = remove_directory(src_path);
        if (result != SUCCESS) {
            return ERROR_MOVE_FAILED;
        }

        return SUCCESS;
    }

    return ERROR_MOVE_FAILED;
}

// ============================================================================
// PATTERN MATCHING & FILTERING IMPLEMENTATION
// ============================================================================

int match_pattern(const char *filename, const char *pattern) {
    // Use fnmatch for pattern matching (supports * and ?)
    return fnmatch(pattern, filename, 0) == 0;
}

int should_copy_file(const char *filename, const char **include_patterns,
                     const char **exclude_patterns) {
    // If exclude patterns exist, check them first
    if (exclude_patterns != NULL) {
        for (int i = 0; exclude_patterns[i] != NULL; i++) {
            if (match_pattern(filename, exclude_patterns[i])) {
                return 0; // Excluded
            }
        }
    }

    // If no include patterns, include by default
    if (include_patterns == NULL || include_patterns[0] == NULL) {
        return 1;
    }

    // Check include patterns
    for (int i = 0; include_patterns[i] != NULL; i++) {
        if (match_pattern(filename, include_patterns[i])) {
            return 1; // Included
        }
    }

    return 0; // Not matched by any include pattern
}

int copy_file_filtered(const char *src_path, const char *dest_path,
                       const char **include_patterns, const char **exclude_patterns,
                       CopyStats *stats) {
    // Extract filename from path
    const char *filename = strrchr(src_path, '/');
    if (filename == NULL) {
        filename = src_path;
    } else {
        filename++; // Skip the '/'
    }

    // Check if file should be copied
    if (!should_copy_file(filename, include_patterns, exclude_patterns)) {
        return SUCCESS; // Skip this file
    }

    // Copy the file
    int result = copy_file(src_path, dest_path);

    // Update statistics if provided
    if (result == SUCCESS && stats != NULL) {
        stats->total_files++;
        long size = get_file_size(dest_path);
        if (size > 0) {
            stats->total_bytes += size;
            update_stats(stats, size);
        }
    }

    return result;
}

int copy_directory_filtered(const char *src_path, const char *dest_path,
                            const char **include_patterns, const char **exclude_patterns,
                            CopyStats *stats) {
    DIR *dir;
    struct dirent *entry;
    char src_file[MAX_PATH];
    char dest_file[MAX_PATH];
    int result;

    // Create destination directory
    result = create_directory(dest_path);
    if (result != SUCCESS) {
        return result;
    }

    if (stats != NULL) {
        stats->total_dirs++;
    }

    // Open source directory
    dir = opendir(src_path);
    if (dir == NULL) {
        return ERROR_DIR_OPEN;
    }

    printf("Copying directory (filtered): %s -> %s\n", src_path, dest_path);

    // Iterate through directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Build full paths
        snprintf(src_file, MAX_PATH, "%s/%s", src_path, entry->d_name);
        snprintf(dest_file, MAX_PATH, "%s/%s", dest_path, entry->d_name);

        // Check if entry is a directory
        if (is_directory(src_file)) {
            // Recursively copy subdirectory
            result = copy_directory_filtered(src_file, dest_file,
                                            include_patterns, exclude_patterns, stats);
            if (result != SUCCESS) {
                closedir(dir);
                return result;
            }
        } else {
            // Copy file with filtering
            result = copy_file_filtered(src_file, dest_file,
                                       include_patterns, exclude_patterns, stats);
            if (result != SUCCESS && result != SUCCESS) {
                closedir(dir);
                return result;
            }
        }
    }

    closedir(dir);

    return SUCCESS;
}

// Get parent directory path
void get_parent_directory(const char *path, char *parent, size_t size) {
    strncpy(parent, path, size - 1);
    parent[size - 1] = '\0';

    // Remove trailing slashes
    size_t len = strlen(parent);
    while (len > 1 && parent[len - 1] == '/') {
        parent[len - 1] = '\0';
        len--;
    }

    // Find last slash
    char *last_slash = strrchr(parent, '/');
    if (last_slash != NULL) {
        if (last_slash == parent) {
            // Root directory
            parent[1] = '\0';
        } else {
            *last_slash = '\0';
        }
    } else {
        // No slash found, use current directory
        strcpy(parent, ".");
    }
}

// List directory contents with details
int list_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char full_path[MAX_PATH];
    int count = 0;

    dir = opendir(path);
    if (dir == NULL) {
        return ERROR_DIR_OPEN;
    }

    printf("\n");
    printf("📂 Directory: %s\n", path);
    printf("════════════════════════════════════════════════════════\n");
    printf("%-4s %-10s %-8s %-12s %s\n", "Type", "Perms", "Size", "Modified", "Name");
    printf("────────────────────────────────────────────────────────\n");

    while ((entry = readdir(dir)) != NULL) {
        // Build full path
        snprintf(full_path, MAX_PATH, "%s/%s", path, entry->d_name);

        if (stat(full_path, &st) != 0) {
            continue;
        }

        // Type
        char type[5];
        if (S_ISDIR(st.st_mode)) {
            strcpy(type, "📁");
        } else if (S_ISLNK(st.st_mode)) {
            strcpy(type, "🔗");
        } else {
            strcpy(type, "📄");
        }

        // Permissions
        char perms[11];
        perms[0] = S_ISDIR(st.st_mode) ? 'd' : '-';
        perms[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        perms[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        perms[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        perms[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        perms[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        perms[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
        perms[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        perms[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        perms[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
        perms[10] = '\0';

        // Size
        char size_str[20];
        if (S_ISDIR(st.st_mode)) {
            strcpy(size_str, "<DIR>");
        } else {
            long size = st.st_size;
            if (size < 1024) {
                snprintf(size_str, sizeof(size_str), "%ldB", size);
            } else if (size < 1024 * 1024) {
                snprintf(size_str, sizeof(size_str), "%.1fKB", size / 1024.0);
            } else if (size < 1024 * 1024 * 1024) {
                snprintf(size_str, sizeof(size_str), "%.1fMB", size / (1024.0 * 1024.0));
            } else {
                snprintf(size_str, sizeof(size_str), "%.1fGB", size / (1024.0 * 1024.0 * 1024.0));
            }
        }

        // Modified time
        char time_str[20];
        struct tm *timeinfo = localtime(&st.st_mtime);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", timeinfo);

        // Print entry
        printf("%-4s %-10s %-8s %-12s %s\n", type, perms, size_str, time_str, entry->d_name);
        count++;
    }

    closedir(dir);

    printf("────────────────────────────────────────────────────────\n");
    printf("Total: %d items\n", count);

    return SUCCESS;
}

// Browse filesystem interactively (simple file explorer)
void browse_filesystem(const char *start_path) {
    char current_path[MAX_PATH];
    char input[MAX_PATH];
    char selected_path[MAX_PATH];
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    int choice;

    // Initialize current path
    if (start_path == NULL || strlen(start_path) == 0) {
        getcwd(current_path, MAX_PATH);
    } else {
        strncpy(current_path, start_path, MAX_PATH - 1);
        current_path[MAX_PATH - 1] = '\0';
    }

    while (1) {
        // Clear screen
        printf("\033[2J\033[H");

        printf("\n");
        printf("╔════════════════════════════════════════════════════════╗\n");
        printf("║              FILE EXPLORER - BROWSE MODE               ║\n");
        printf("╚════════════════════════════════════════════════════════╝\n");

        // Display current path
        printf("\n📍 Current Path: %s\n", current_path);

        // List directory contents
        dir = opendir(current_path);
        if (dir == NULL) {
            printf("\n❌ Cannot open directory: %s\n", current_path);
            printf("\nPress Enter to go back...");
            getchar();
            get_parent_directory(current_path, current_path, MAX_PATH);
            continue;
        }

        printf("\n");
        printf("════════════════════════════════════════════════════════\n");
        printf("  #  Type  Size        Name\n");
        printf("────────────────────────────────────────────────────────\n");

        // Show parent directory option
        printf("  0  📁    <UP>        .. (Parent Directory)\n");

        // Read and display entries
        int index = 1;
        char entries[100][MAX_PATH];  // Store up to 100 entries
        int entry_count = 0;

        while ((entry = readdir(dir)) != NULL && entry_count < 100) {
            // Skip . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Build full path
            snprintf(selected_path, MAX_PATH, "%s/%s", current_path, entry->d_name);

            if (stat(selected_path, &st) != 0) {
                continue;
            }

            // Store entry name
            strncpy(entries[entry_count], entry->d_name, MAX_PATH - 1);
            entries[entry_count][MAX_PATH - 1] = '\0';

            // Type and size
            char type[5];
            char size_str[20];

            if (S_ISDIR(st.st_mode)) {
                strcpy(type, "📁");
                strcpy(size_str, "<DIR>");
            } else {
                strcpy(type, "📄");
                long size = st.st_size;
                if (size < 1024) {
                    snprintf(size_str, sizeof(size_str), "%ldB", size);
                } else if (size < 1024 * 1024) {
                    snprintf(size_str, sizeof(size_str), "%.1fKB", size / 1024.0);
                } else {
                    snprintf(size_str, sizeof(size_str), "%.1fMB", size / (1024.0 * 1024.0));
                }
            }

            printf(" %2d  %-4s %-10s  %s\n", index, type, size_str, entry->d_name);

            index++;
            entry_count++;
        }

        closedir(dir);

        printf("────────────────────────────────────────────────────────\n");
        printf("Total: %d items\n", entry_count);
        printf("\n");
        printf("Commands:\n");
        printf("  • Enter number to navigate/select\n");
        printf("  • Type 'p' to show full path\n");
        printf("  • Type 'q' to quit explorer\n");
        printf("\n");
        printf("Enter choice: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Remove newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // Handle commands
        if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) {
            break;
        } else if (strcmp(input, "p") == 0 || strcmp(input, "P") == 0) {
            printf("\n📍 Full Path: %s\n", current_path);
            printf("\nPress Enter to continue...");
            getchar();
            continue;
        }

        // Handle number selection
        choice = atoi(input);

        if (choice == 0) {
            // Go to parent directory
            get_parent_directory(current_path, current_path, MAX_PATH);
        } else if (choice > 0 && choice <= entry_count) {
            // Navigate to selected entry
            snprintf(selected_path, MAX_PATH, "%s/%s", current_path, entries[choice - 1]);

            if (stat(selected_path, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    // It's a directory, navigate into it
                    strncpy(current_path, selected_path, MAX_PATH - 1);
                    current_path[MAX_PATH - 1] = '\0';
                } else {
                    // It's a file, show info and copy path
                    printf("\n");
                    printf("╔════════════════════════════════════════════════════════╗\n");
                    printf("║                    FILE SELECTED                       ║\n");
                    printf("╚════════════════════════════════════════════════════════╝\n");
                    printf("\n");
                    printf("  📄 File: %s\n", entries[choice - 1]);
                    printf("  📍 Full Path: %s\n", selected_path);
                    printf("  📊 Size: %ld bytes\n", st.st_size);
                    printf("\n");
                    printf("  ✅ Path copied! You can use this path for copy/move operations.\n");
                    printf("\n");
                    printf("Press Enter to continue...");
                    getchar();
                }
            }
        } else {
            printf("\n❌ Invalid choice!\n");
            printf("Press Enter to continue...");
            getchar();
        }
    }

    printf("\n✅ Exited file explorer.\n");
}

