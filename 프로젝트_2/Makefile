CC = gcc
TARGET = ssu_score
OBJS = main.o ssu_score.o blank.o

CFLAGS = -Wall
LDFLAGS = -lc

all : $(TARGET)
    
$(TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
    
.c.o :
	$(CC) $(CFLAGS) -c -o $@ $<
    
clean : 
	rm -f $(OBJS) $@