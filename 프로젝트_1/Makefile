all : ssu_backup ssu_backup_help ssu_backup_add ssu_backup_remove ssu_backup_recover

OBJECTS = ssu_backup.o
TARGETS = ssu_backup ssu_backup_help ssu_backup_add ssu_backup_remove ssu_backup_recover
CC = gcc

ssu_backup : $(OBJECTS)
	$(CC) -o $@ $^

ssu_backup_help : help.o
	$(CC) -o $@ $^

ssu_backup_add : add.o
	$(CC) -o $@ $^ -lcrypto 

ssu_backup_remove : remove.o
	$(CC) -o $@ $^

ssu_backup_recover : recover.o
	$(CC) -o $@ $^ -lcrypto

ssu_backup.o : ssu_backup.c 
	$(CC) -c $^

help.o : help.c
	$(CC) -c $^

add.o : add.c
	$(CC) -c $^

remove.o : remove.c
	$(CC) -c $^

recover.o : recover.c
	$(CC) -c $^

clean :
	rm ssu_backup.o
	rm help.o
	rm add.o
	rm remove.o
	rm recover.o
	rm ssu_backup_help
	rm ssu_backup
	rm ssu_backup_add
	rm ssu_backup_remove
	rm ssu_backup_recover