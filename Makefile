CC=gcc
CCFLAGS=-Wall -O2
LDFLAGS=-lm -lpthread

pi_process: pi.c
	$(CC) $(CCFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) -f pi_process