# Compiler settings
CC = gcc
CFLAGS = -g -Wno-implicit-function-declaration -Wno-implicit-int
LDFLAGS = -Bstatic -lm

# Source files
TASK_SRCS = tarefas/tarefa-0.c tarefas/tarefa-1.c tarefas/tarefa-2.c tarefas/tarefa-3.c tarefas/tarefa-4.c
TASK_OBJS = $(TASK_SRCS:.c=.o)
TASK_EXECS = $(TASK_SRCS:.c=)

# Common object files
COMMON_OBJS = smpl.o rand.o

# Default target
all: $(TASK_EXECS)

# Compile task executables
tarefas/tarefa-%: tarefas/tarefa-%.o $(COMMON_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile task object files
tarefas/%.o: tarefas/%.c smpl.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile common object files
smpl.o: smpl.c smpl.h
	$(CC) $(CFLAGS) -c smpl.c

rand.o: rand.c
	$(CC) $(CFLAGS) -c rand.c

# Clean target
clean:
	rm -f *.o tarefas/*.o $(TASK_EXECS)

.PHONY: all clean

