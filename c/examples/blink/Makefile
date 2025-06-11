# Define source directory
DIR_SRC_MAIN = ./src

# Define output directories for object files and the final binary
DIR_OBJ_OUTPUT = ./obj
DIR_BIN_OUTPUT = ./bin

# Find all .c files in the source directory
SRC_FILES_MAIN = $(wildcard $(DIR_SRC_MAIN)/*.c)

# Create a list of object files, placing them in DIR_OBJ_OUTPUT
OBJ_FILES_MAIN = $(patsubst $(DIR_SRC_MAIN)/%.c,$(DIR_OBJ_OUTPUT)/%.o,$(SRC_FILES_MAIN))

TARGET_NAME = blink
TARGET = $(DIR_BIN_OUTPUT)/$(TARGET_NAME)

CC = gcc # Changed from cc to gcc for consistency, adjust if cc is specifically needed
DEBUG = -g -Wall
CFLAGS += $(DEBUG)
# CFLAGS += -D CONSUMER=\"$(TARGET_NAME)\" # Original CFLAGS, uncomment if needed
LIB = -lgpiod

# --- Targets ---

all: $(TARGET)

$(TARGET): $(OBJ_FILES_MAIN)
	@mkdir -p $(DIR_BIN_OUTPUT) # Ensure bin directory exists
	$(CC) $(CFLAGS) $(OBJ_FILES_MAIN) -o $@ $(LIB)
	@echo "Build complete: $@"

# Rule to compile main source files
$(DIR_OBJ_OUTPUT)/%.o : $(DIR_SRC_MAIN)/%.c
	@mkdir -p $(DIR_OBJ_OUTPUT) # Ensure obj directory exists
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f $(DIR_OBJ_OUTPUT)/*.o
	rm -f $(TARGET)
	@echo "Clean complete."

.PHONY: all clean
