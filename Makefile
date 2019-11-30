CFLAGS = -Wall -Wextra -std=c99 -g

all: worker master

master: master.c shared.c settings.c
	$(CC) $(CFLAGS) master.c shared.c settings.c -o master

worker: worker.c prime.c shared.c
	$(CC) $(CFLAGS) worker.c prime.c shared.c -o worker

.PHONY: clean
clean:
	rm -rf master worker
