CC = gcc
CFLAGS = -I./

IMPL = ./impl
INTR = ./intr
OBJ = ./obj

IMPL_FILES = $(wildcard $(IMPL)/*.c)
OBJ_FILES = $(patsubst $(IMPL)/%.c, $(OBJ)/%.o, $(IMPL_FILES))

x1: $(OBJ_FILES)
	@mkdir -p recv
	@$(CC) $^ -o run1
	@./run1 6543 6542 
	#@gdb -args ./run1 6543 6543 
	@rm run1

x2: $(OBJ_FILES)
	@mkdir -p recv
	@$(CC) $^ -o run2
	@./run2 6542 6543
	#@gdb -args ./run2 6542 6543
	@rm run2

$(OBJ)/%.o : $(IMPL)/%.c
	@mkdir -p $(@D)
	@$(CC) -c -o $@ $< $(CFLAGS)
