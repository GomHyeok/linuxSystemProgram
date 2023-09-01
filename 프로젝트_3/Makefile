OBJECTS = main.o ssu_monitor.o
TARGETS = ssu_monitor

CC = gcc

TARGETS : $(OBJECTS)
	$(CC) -o ssu_monitor $^

main.o : main.c ssu_monitor.h
	$(CC) -c main.c

ssu_monitor.o : ssu_monitor.c ssu_monitor.h
	$(CC) -c ssu_monitor.c

clean :
	rm -f *.o ssu_monitor