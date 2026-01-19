all:
	gcc -g -Wall src/*.c -o lsh

clean:
	rm -f lsh