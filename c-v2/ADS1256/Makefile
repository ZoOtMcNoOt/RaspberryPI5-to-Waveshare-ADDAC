# Define Compiler and Linker
CC = gcc

# Define Compiler Flags
# -g: Adds debugging information
# -Wall: Enables all common compiler warnings
# -I.: Adds the current directory to the include path (if you have .h files in the same directory)
CFLAGS = -g -Wall -I.

# Define Linker Flags
# -lm: Links the math library
LDFLAGS = -lm -lgpiod # Assuming gpiod is still needed as per other Makefiles

# Define Source Files
SRCS = main.c ads1256.c

# Define Object Files from Source Files
# This replaces .c with .o for each source file
OBJS = $(SRCS:.c=.o)

# Define the Target Executable Name
TARGET = my_app

# --- Targets ---

# Default Target: Build the executable
all: $(TARGET)

# Rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Rule to compile .c source files into .o object files
# $< is the first prerequisite (the .c file)
# $@ is the target (the .o file)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean Target: Remove object files and the target executable
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete."

# Phony Targets: Targets that are not actual files
.PHONY: all clean
