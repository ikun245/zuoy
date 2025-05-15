CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -lm
SOURCES = account.c visualization.c bank_transaction.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = bank_system

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

run: $(TARGET)
	./$(TARGET)

debug: $(TARGET)
	gdb ./$(TARGET)