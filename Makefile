CFLAGS = -Wall -Wextra -std=c99 -g

all: worker master

master: master.c shared.c
	$(CC) $(CFLAGS) master.c shared.c -o master

worker: worker.c shared.c
	$(CC) $(CFLAGS) worker.c shared.c -o worker

.PHONY: clean
clean:
	rm -rf master worker
