CC = gcc
CFLAGS = -I./

IMPL = ./impl
INTR = ./intr
OBJ = ./obj

IMPL_FILES = $(wildcard $(IMPL)/*.c)
OBJ_FILES = $(patsubst $(IMPL)/%.c, $(OBJ)/%.o, $(IMPL_FILES))

run: $(OBJ_FILES)
	@mkdir -p rec
	@$(CC) $^ -o run
	@rm obj/*
	@./run
	#@gdb ./run
	@rm run

$(OBJ)/%.o : $(IMPL)/%.c
	@mkdir -p $(@D)
	@$(CC) -c -o $@ $< $(CFLAGS)
