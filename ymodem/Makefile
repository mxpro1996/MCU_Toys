OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
TARGET = ymodem_demo
CC = gcc
CXX = g++

$(TARGET): $(OBJS)
	@$(CC) -o $(TARGET) $(OBJS)

clean:
	@rm -f *.o $(TARGET)

test: $(TARGET)
	@./$(TARGET)