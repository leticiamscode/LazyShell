all:
	gcc -g -Wall src/client.c src/lsh.c -o lsh

clean:
	rm -f lsh