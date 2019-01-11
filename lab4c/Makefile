#NAME: Sung Min Ahn
#EMAIL: sungmin.sam5@gmail.com
#UID: 604949854
.SILENT:


default: build

build:
	gcc -Wall -Wextra -lmraa -lm -g -o lab4c_tcp lab4c_tcp.c
	gcc -Wall -Wextra -lmraa -lm -g -lssl -lcrypto -o lab4c_tls lab4c_tls.c
dist:
	tar -czf lab4c-604949854.tar.gz Makefile README lab4c_tcp.c lab4c_tls.c

clean:
	rm -rf *.tar.gz lab4c_tls la4c_tcp
