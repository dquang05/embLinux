CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
LDFLAGS =

SRCS_WRITER = writer.c
OBJS_WRITER = $(SRCS_WRITER:.c=.o)
TARGET_WRITER = config-writer

SRCS_READER = reader.c
OBJS_READER = $(SRCS_READER:.c=.o)
TARGET_READER = config-reader

.PHONY: all clean

all: $(TARGET_WRITER) $(TARGET_READER)

$(TARGET_WRITER): $(OBJS_WRITER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_READER): $(OBJS_READER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS_WRITER) $(OBJS_READER) $(TARGET_WRITER) $(TARGET_READER)
