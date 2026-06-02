CC = gcc
CFLAGS = -std=c11 -O3

TARGET = tp

SRCS = main.c table_actions.c cell_parsing.c
OBJS = $(SRCS:.c=.o)

HEADERS = CSV_proc.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@if exist *.o del *.o
	@if exist $(TARGET) del $(TARGET)
	@echo Cleaned

#clean:
#	rm -f *.o $(TARGET)