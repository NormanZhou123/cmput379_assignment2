all:wbc379.c
	gcc -o wbc379 wbc379.c -lcrypto 
	gcc -o wbs379 wbs379.c -lpthread

clean:
	rm -f wbs379 wbc379
