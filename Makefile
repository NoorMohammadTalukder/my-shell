SRC := src
OBJ := obj
BIN := bin
EXEC_NAME := shell

SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
INCS := -Iinclude/
EXEC := $(BIN)/$(EXEC_NAME)

CC     := gcc
CFLAGS := -g -Wall -std=c99 $(INCS)

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)/*.o $(EXEC)

$(shell mkdir -p $(OBJ) $(BIN))

.PHONY: all clean
