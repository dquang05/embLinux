CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
LDFLAGS =

SRCS_DAEMON = daemon.c
OBJS_DAEMON = $(SRCS_DAEMON:.c=.o)
TARGET_DAEMON = sensor-daemon

SRCS_CLI = cli.c
OBJS_CLI = $(SRCS_CLI:.c=.o)
TARGET_CLI = sensor-cli

.PHONY: all clean

all: $(TARGET_DAEMON) $(TARGET_CLI)

$(TARGET_DAEMON): $(OBJS_DAEMON)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_CLI): $(OBJS_CLI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS_DAEMON) $(OBJS_CLI) $(TARGET_DAEMON) $(TARGET_CLI)
