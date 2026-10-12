CC = gcc
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`


TARGET = ninix
SRCS = src/main.c src/os_detect.c src/installer.c src/ui.c src/cJSON.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)




$(TARGET):$(OBJS)
	$(CC)$(OBJS) -o $(TARGET)$(LDFLAGS)

	

%.o: %.c
	$(CC)$(CFLAGS) -c $< -o$@

clean:
	rm -f src/*.o $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild