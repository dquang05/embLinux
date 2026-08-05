CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
LDFLAGS = -lrt -lpthread

SRCS_CONTROLLER = controller.c
OBJS_CONTROLLER = $(SRCS_CONTROLLER:.c=.o)
TARGET_CONTROLLER = controller

SRCS_DEVICE = device.c
OBJS_DEVICE = $(SRCS_DEVICE:.c=.o)
TARGET_DEVICE = device

.PHONY: all clean

all: $(TARGET_CONTROLLER) $(TARGET_DEVICE)

$(TARGET_CONTROLLER): $(OBJS_CONTROLLER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_DEVICE): $(OBJS_DEVICE)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS_CONTROLLER) $(OBJS_DEVICE) $(TARGET_CONTROLLER) $(TARGET_DEVICE)
