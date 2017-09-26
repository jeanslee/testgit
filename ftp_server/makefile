CC = gcc
CFLAGS = -O2 -lcrypt
all: ftp_server
ftp_server: ftp_server.o data_trans.o adapter.o
	$(CC) -o ftp_server ftp_server.o data_trans.o adapter.o $(CFLAGS)
ftp_server.o: ftp_server.c ftp_server.h data_trans.c data_trans.h adapter.c adapter.h
	$(CC) -c ftp_server.c
data_trans.o: data_trans.c ftp_server.c data_trans.h ftp_server.h adapter.c adapter.h
	$(CC) -c data_trans.c
adapter.o: adapter.c adapter.h ftp_server.c ftp_server.h
	$(CC) -c adapter.c

clean:
	rm -f *.o
	rm -f ftp_server

