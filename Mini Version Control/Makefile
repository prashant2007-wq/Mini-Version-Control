CC = gcc
CFLAGS = -Wall -Wextra -std=c99

OBJS = main.o init.o hash.o objects.o index.o commit.o log.o status.o diff.o

mgit: $(OBJS)
	$(CC) $(CFLAGS) -o mgit $(OBJS)

%.o: %.c mgit.h
	$(CC) $(CFLAGS) -c $< -o $@

all: mgit

clean:
	rm -f $(OBJS) mgit

test: mgit
	@echo "No automated tests yet; run mgit commands manually."

