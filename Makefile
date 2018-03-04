#NAME: Kenny Luu
#EMAIL: kennyluuluu@gmail.com
#ID: 104823244

CC=gcc
CFLAGS=-Wall -Wextra

default: lab3a

lab4b: lab3a.c
	$(CC) $(CFLAGS) -o lab3a lab3a.c

.PHONY: clean
clean:
	rm -rf lab3a *.tar.gz

dist:
	tar -zcvf lab3a-104823244.tar.gz README lab3a.c Makefile
