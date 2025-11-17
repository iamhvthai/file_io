# Makefile for File/Folder Copy Utility
# System Programming Project

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Iinclude
LDFLAGS =

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin
DOCS_DIR = docs
SCRIPTS_DIR = scripts
TESTS_DIR = tests

# Target executable
TARGET = $(BIN_DIR)/filecopy

# Source files
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/file_operations.c
HEADERS = $(INC_DIR)/file_operations.h

# Object files (in build directory)
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/file_operations.o

# Default target
all: $(BIN_DIR) $(BUILD_DIR) $(TARGET)

# Create directories if they don't exist
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	@echo "🔗 Linking object files..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✅ Build successful! Executable: $(TARGET)"

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "🔨 Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)/*.o $(BIN_DIR)/filecopy
	@echo "✅ Clean complete!"

# Deep clean (remove all generated directories)
distclean: clean test-clean
	@echo "🧹 Deep cleaning..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "✅ Deep clean complete!"

# Clean and rebuild
rebuild: clean all

# Install (copy to /usr/local/bin - requires sudo)
install: $(TARGET)
	@echo "📦 Installing $(TARGET) to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod 755 /usr/local/bin/$(TARGET)
	@echo "✅ Installation complete!"

# Uninstall
uninstall:
	@echo "🗑️  Uninstalling $(TARGET)..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "✅ Uninstall complete!"

# Run the program
run: $(TARGET)
	@echo "🚀 Running $(TARGET)..."
	@echo ""
	$(TARGET)

# Create test files and directories for testing
test-setup:
	@echo "📁 Creating test environment..."
	mkdir -p test_source/subdir1/subdir2
	echo "This is test file 1" > test_source/file1.txt
	echo "This is test file 2" > test_source/file2.txt
	echo "This is a file in subdir1" > test_source/subdir1/file3.txt
	echo "This is a file in subdir2" > test_source/subdir1/subdir2/file4.txt
	dd if=/dev/urandom of=test_source/binary_file.bin bs=1024 count=100 2>/dev/null
	@echo "✅ Test environment created in 'test_source/' directory"

# Clean test files
test-clean:
	@echo "🧹 Cleaning test environment..."
	rm -rf test_source test_destination
	@echo "✅ Test environment cleaned!"

# Run a quick test
test: $(TARGET) test-setup
	@echo ""
	@echo "🧪 Running test: Copy test_source to test_destination"
	@echo "────────────────────────────────────────────────────────"
	$(TARGET) test_source test_destination
	@echo ""
	@echo "📊 Verifying copy..."
	@if [ -d "test_destination" ]; then \
		echo "✅ Destination directory created"; \
		echo "📁 Contents of test_destination:"; \
		find test_destination -type f; \
	else \
		echo "❌ Test failed: Destination directory not created"; \
	fi

# Display help
help:
	@echo "╔════════════════════════════════════════════════════════╗"
	@echo "║     File/Folder Copy Utility - Makefile Help          ║"
	@echo "╚════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "📁 Project Structure:"
	@echo "  src/              - Source files (.c)"
	@echo "  include/          - Header files (.h)"
	@echo "  build/            - Object files (.o)"
	@echo "  bin/              - Executable files"
	@echo "  docs/             - Documentation"
	@echo "  scripts/          - Build and demo scripts"
	@echo "  tests/            - Test files"
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build the project (default)"
	@echo "  make all          - Build the project"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean    - Deep clean (remove all generated files)"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make run          - Build and run the program"
	@echo "  make install      - Install to /usr/local/bin (requires sudo)"
	@echo "  make uninstall    - Remove from /usr/local/bin (requires sudo)"
	@echo "  make test-setup   - Create test files and directories"
	@echo "  make test         - Run automated test"
	@echo "  make test-clean   - Remove test files and directories"
	@echo "  make help         - Display this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make && bin/filecopy"
	@echo "  make run"
	@echo "  bin/filecopy source.txt destination.txt"
	@echo "  bin/filecopy /path/to/dir /path/to/backup"
	@echo ""

# Phony targets (not actual files)
.PHONY: all clean distclean rebuild install uninstall run test-setup test-clean test help

