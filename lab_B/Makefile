
.SILENT:


default: build

build:
	gcc -Wall -Wextra -lmraa -lm -g -o lab_B_tcp lab_B_tcp.c
	gcc -Wall -Wextra -lmraa -lm -g -lssl -lcrypto -o lab_B_tls lab_B_tls.c
dist:
	tar -czf lab_B.tar.gz Makefile README lab_B_tcp.c lab_B_tls.c

clean:
	rm -rf *.tar.gz lab_B_tls la_B_tcp
