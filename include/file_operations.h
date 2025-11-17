#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

// Buffer size for file operations
#define BUFFER_SIZE 8192
#define MAX_PATH 4096

// Return codes
#define SUCCESS 0
#define ERROR_FILE_OPEN -1
#define ERROR_FILE_READ -2
#define ERROR_FILE_WRITE -3
#define ERROR_DIR_CREATE -4
#define ERROR_DIR_OPEN -5
#define ERROR_INVALID_PATH -6
#define ERROR_MOVE_FAILED -7
#define ERROR_FILES_DIFFER -8

// Pattern matching
#define MAX_PATTERNS 10

/**
 * Copy a single file from source to destination
 * @param src_path: Source file path
 * @param dest_path: Destination file path or directory
 *                   - If dest_path is a directory, the file will be copied into it
 *                   - If dest_path is a file path, it will be used as the destination filename
 * @return SUCCESS on success, error code on failure
 */
int copy_file(const char *src_path, const char *dest_path);

/**
 * Copy a directory recursively from source to destination
 * @param src_path: Source directory path
 * @param dest_path: Destination directory path
 * @return SUCCESS on success, error code on failure
 */
int copy_directory(const char *src_path, const char *dest_path);

/**
 * Check if a path is a directory
 * @param path: Path to check
 * @return 1 if directory, 0 otherwise
 */
int is_directory(const char *path);

/**
 * Check if a path exists
 * @param path: Path to check
 * @return 1 if exists, 0 otherwise
 */
int path_exists(const char *path);

/**
 * Create directory with parent directories if needed
 * @param path: Directory path to create
 * @return SUCCESS on success, error code on failure
 */
int create_directory(const char *path);

/**
 * Get file size in bytes
 * @param path: File path
 * @return File size or -1 on error
 */
long get_file_size(const char *path);

/**
 * Print error message based on error code
 * @param error_code: Error code from operations
 * @param context: Additional context message
 */
void print_error(int error_code, const char *context);

/**
 * Display copy progress
 * @param current: Current bytes copied
 * @param total: Total bytes to copy
 * @param filename: Name of file being copied
 */
void display_progress(long current, long total, const char *filename);

// ============================================================================
// NEW FEATURES - Progress Statistics
// ============================================================================

/**
 * Structure to hold copy statistics
 */
typedef struct {
    long total_files;
    long total_dirs;
    long total_bytes;
    long copied_bytes;
    time_t start_time;
    time_t current_time;
    double transfer_speed;  // bytes per second
} CopyStats;

/**
 * Initialize copy statistics
 * @param stats: Pointer to CopyStats structure
 */
void init_stats(CopyStats *stats);

/**
 * Update copy statistics
 * @param stats: Pointer to CopyStats structure
 * @param bytes: Bytes copied in this update
 */
void update_stats(CopyStats *stats, long bytes);

/**
 * Display copy statistics
 * @param stats: Pointer to CopyStats structure
 */
void display_stats(const CopyStats *stats);

/**
 * Calculate transfer speed
 * @param stats: Pointer to CopyStats structure
 * @return Speed in bytes per second
 */
double calculate_speed(const CopyStats *stats);

/**
 * Estimate time remaining
 * @param stats: Pointer to CopyStats structure
 * @return Estimated seconds remaining
 */
long estimate_time_remaining(const CopyStats *stats);

// ============================================================================
// NEW FEATURES - File Comparison & Checksum
// ============================================================================

/**
 * Compare two files byte-by-byte
 * @param file1: First file path
 * @param file2: Second file path
 * @return SUCCESS if identical, ERROR_FILES_DIFFER if different, error code on failure
 */
int compare_files(const char *file1, const char *file2);

/**
 * Calculate MD5 checksum of a file
 * @param filepath: Path to file
 * @param checksum: Buffer to store checksum (must be at least 33 bytes)
 * @return SUCCESS on success, error code on failure
 */
int calculate_md5(const char *filepath, char *checksum);

/**
 * Verify file integrity using checksum
 * @param filepath: Path to file
 * @param expected_checksum: Expected MD5 checksum
 * @return SUCCESS if match, ERROR_FILES_DIFFER if mismatch, error code on failure
 */
int verify_checksum(const char *filepath, const char *expected_checksum);

// ============================================================================
// NEW FEATURES - Move Operations
// ============================================================================

/**
 * Move a file from source to destination
 * @param src_path: Source file path
 * @param dest_path: Destination file path
 * @return SUCCESS on success, error code on failure
 */
int move_file(const char *src_path, const char *dest_path);

/**
 * Move a directory from source to destination
 * @param src_path: Source directory path
 * @param dest_path: Destination directory path
 * @return SUCCESS on success, error code on failure
 */
int move_directory(const char *src_path, const char *dest_path);

/**
 * Remove directory recursively
 * @param path: Directory path to remove
 * @return SUCCESS on success, error code on failure
 */
int remove_directory(const char *path);

// ============================================================================
// NEW FEATURES - Pattern Matching & Filtering
// ============================================================================

/**
 * Check if filename matches pattern
 * @param filename: Filename to check
 * @param pattern: Pattern (supports * and ?)
 * @return 1 if matches, 0 otherwise
 */
int match_pattern(const char *filename, const char *pattern);

/**
 * Copy file with pattern filtering
 * @param src_path: Source file path
 * @param dest_path: Destination file path
 * @param include_patterns: Array of include patterns (NULL terminated)
 * @param exclude_patterns: Array of exclude patterns (NULL terminated)
 * @param stats: Pointer to statistics structure (can be NULL)
 * @return SUCCESS on success, error code on failure
 */
int copy_file_filtered(const char *src_path, const char *dest_path,
                       const char **include_patterns, const char **exclude_patterns,
                       CopyStats *stats);

/**
 * Copy directory with pattern filtering
 * @param src_path: Source directory path
 * @param dest_path: Destination directory path
 * @param include_patterns: Array of include patterns (NULL terminated)
 * @param exclude_patterns: Array of exclude patterns (NULL terminated)
 * @param stats: Pointer to statistics structure (can be NULL)
 * @return SUCCESS on success, error code on failure
 */
int copy_directory_filtered(const char *src_path, const char *dest_path,
                            const char **include_patterns, const char **exclude_patterns,
                            CopyStats *stats);

/**
 * Check if file should be copied based on patterns
 * @param filename: Filename to check
 * @param include_patterns: Array of include patterns (NULL terminated)
 * @param exclude_patterns: Array of exclude patterns (NULL terminated)
 * @return 1 if should copy, 0 otherwise
 */
int should_copy_file(const char *filename, const char **include_patterns,
                     const char **exclude_patterns);

/**
 * List directory contents with details
 * @param path: Directory path to list
 * @return SUCCESS on success, error code on failure
 */
int list_directory(const char *path);

/**
 * Browse filesystem interactively (simple file explorer)
 * @param start_path: Starting directory path
 */
void browse_filesystem(const char *start_path);

/**
 * Get parent directory path
 * @param path: Current path
 * @param parent: Buffer to store parent path
 * @param size: Buffer size
 */
void get_parent_directory(const char *path, char *parent, size_t size);

#endif // FILE_OPERATIONS_H

