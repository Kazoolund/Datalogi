CFLAGS = -Wall -Wextra -std=c99 -g -D_POSIX_SOURCE -D_POSIX_C_SOURCE=199309L

all: worker master

master: master.c shared.c settings.c shared.h settings.h
	$(CC) $(CFLAGS) master.c shared.c settings.c -o master

worker: worker.c prime.c shared.c shared.h
	$(CC) $(CFLAGS) worker.c prime.c shared.c -o worker

.PHONY: clean tests
clean:
	rm -rf master worker

tests: master worker
	sh run_tests.sh
