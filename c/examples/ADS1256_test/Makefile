# Define source directories
DIR_SRC_MAIN = ./src
DIR_SRC_LIB_ADS1256 = ../../lib/ADS1256
DIR_SRC_COMMON = ../../common

# Define output directories for object files and the final binary
DIR_OBJ_OUTPUT = ./obj
DIR_BIN_OUTPUT = ./bin

# Find all .c files in the source directories
SRC_FILES_MAIN = $(wildcard $(DIR_SRC_MAIN)/*.c)
SRC_FILES_LIB_ADS1256 = $(wildcard $(DIR_SRC_LIB_ADS1256)/*.c)
SRC_FILES_COMMON = $(wildcard $(DIR_SRC_COMMON)/*.c)

# Create lists of object files, placing them in DIR_OBJ_OUTPUT
OBJ_FILES_MAIN = $(patsubst $(DIR_SRC_MAIN)/%.c,$(DIR_OBJ_OUTPUT)/%.o,$(SRC_FILES_MAIN))
OBJ_FILES_LIB_ADS1256 = $(patsubst $(DIR_SRC_LIB_ADS1256)/%.c,$(DIR_OBJ_OUTPUT)/lib_ads1256_%.o,$(SRC_FILES_LIB_ADS1256))
OBJ_FILES_COMMON = $(patsubst $(DIR_SRC_COMMON)/%.c,$(DIR_OBJ_OUTPUT)/common_%.o,$(SRC_FILES_COMMON))

ALL_OBJ_FILES = $(OBJ_FILES_MAIN) $(OBJ_FILES_LIB_ADS1256) $(OBJ_FILES_COMMON)

TARGET_NAME = ads1256_test
TARGET = $(DIR_BIN_OUTPUT)/$(TARGET_NAME)

CC = gcc
# DEBUG = -g -O0 -Wall
DEBUG = -g -Wall # Simplified debug flags, adjust as needed
CFLAGS += $(DEBUG) 
# Add include paths for common and library headers
CFLAGS += -I$(DIR_SRC_COMMON) -I$(DIR_SRC_LIB_ADS1256)
LIB = -lgpiod -lm

# --- Targets ---

all: $(TARGET)

$(TARGET): $(ALL_OBJ_FILES)
	@mkdir -p $(DIR_BIN_OUTPUT) # Ensure bin directory exists
	$(CC) $(CFLAGS) $(ALL_OBJ_FILES) -o $@ $(LIB)
	@echo "Build complete: $@"

# Rule to compile main source files
$(DIR_OBJ_OUTPUT)/%.o : $(DIR_SRC_MAIN)/%.c
	@mkdir -p $(DIR_OBJ_OUTPUT) # Ensure obj directory exists
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile ADS1256 library source files
$(DIR_OBJ_OUTPUT)/lib_ads1256_%.o : $(DIR_SRC_LIB_ADS1256)/%.c
	@mkdir -p $(DIR_OBJ_OUTPUT)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile common source files
$(DIR_OBJ_OUTPUT)/common_%.o : $(DIR_SRC_COMMON)/%.c
	@mkdir -p $(DIR_OBJ_OUTPUT)
	$(CC) $(CFLAGS) -c $< -o $@
	
clean :
	rm -f $(DIR_OBJ_OUTPUT)/*.o
	rm -f $(TARGET)
	@echo "Clean complete."

.PHONY: all clean