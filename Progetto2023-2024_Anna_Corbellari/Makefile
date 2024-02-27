CFLAGS=-Wall -O3
INCLUDES=-I./inc
LL_LIBS=-lm

ERR=src/errExit.c
MASTER_SRC=src/master.c
WORKER_SRC=src/worker.c

OBJS_ERR=$(ERR:.c=.o)
OBJS_MASTER=$(MASTER_SRC:.c=.o)
OBJS_WORKER=$(WORKER_SRC:.c=.o)

all: master worker

master: $(OBJS_ERR) $(OBJS_MASTER)
	@echo "Making executable: "$@
	@$(CC) $^ $(LL_LIBS) -o $@

worker: $(OBJS_ERR) $(OBJS_WORKER)
	@echo "Making executable: "$@
	@$(CC) $^ $(LL_LIBS) -o $@

.c.o:
	@echo "Compiling: "$<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean

clean:
	@rm -f src/*.o master worker
	@echo "Removed object files and executable..."
