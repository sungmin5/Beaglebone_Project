default: build

build:
	gcc -lm -lmraa -Wall -Wextra -o lab_A lab_A.c
clean:
	rm -rf lab_A *.tar.gz 
dist: build
	tar -zcvf lab_A.tar.gz lab_A.c README Makefile lab_A_test.sh

check: build
	chmod +x lab_A_test.sh
	./lab_A_test.sh	
