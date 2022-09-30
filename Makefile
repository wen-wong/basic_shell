CC = gcc
RM = rm
CFLAGS = -g

TARGET = shell
OUTPUT = a.txt

all: $(TARGET)

redirect:
	touch $(OUTPUT)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)
	$(RM) $(OUTPUT)
