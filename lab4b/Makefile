#NAME: SungMin Ahn

default: build

build:
	gcc -lm -lmraa -Wall -Wextra -o lab4b lab4b.c
clean:
	rm -rf lab4b *.tar.gz 
dist: build
	tar -zcvf lab4b-604949854.tar.gz lab4b.c README Makefile lab4b_test.sh

check: build
	chmod +x lab4b_test.sh
	./lab4b_test.sh	
