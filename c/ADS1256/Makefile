DIR_OBJ = ./obj
DIR_BIN = ./bin

OBJ_C = $(wildcard ${DIR_OBJ}/*.c)
OBJ_O = $(patsubst %.c,${DIR_BIN}/%.o,$(notdir ${OBJ_C}))

TARGET = ads1256_test

CC = gcc
DEBUG = -g -O0 -Wall
CFLAGS += $(DEBUG) 
LIB = -lgpiod -lm

${TARGET}:${OBJ_O}
	$(CC) $(CFLAGS) $(OBJ_O) -o $@ $(LIB)

${DIR_BIN}/%.o : $(DIR_OBJ)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIB)
	
clean :
	rm -f $(DIR_BIN)/*.* 
	rm -f $(TARGET)