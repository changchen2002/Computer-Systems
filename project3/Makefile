CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c99
TARGET  = scheduler
SRC     = project3.c

.PHONY: all clean test-sjf test-rr test-mlfq test-all

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

sjf-test: $(TARGET)
	./$(TARGET) sjf < testing.txt

rr-test: $(TARGET)
	./$(TARGET) rr < testing.txt

mlfq-test: $(TARGET)
	./$(TARGET) mlfq < testing.txt

full-test: $(TARGET)
	@echo "PSJF"
	./$(TARGET) sjf < testing.txt
	@echo ""
	@echo "Round Robin"
	./$(TARGET) rr < testing.txt
	@echo ""
	@echo "MLFQ"
	./$(TARGET) mlfq < testing.txt

clean:
	rm -f $(TARGET)