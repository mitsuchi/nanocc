CFLAGS=-std=c11 -g -static

nanocc: nanocc.c

test: nanocc
			./test.sh

docker-build:
			docker build -t nanocc:1 .

docker-run:
			docker run -it -v $(CURDIR):/nanocc --name nanocc nanocc:1

docker-test:
			docker run -v $(CURDIR):/nanocc --name nanocc nanocc:1 make test
			docker rm nanocc

docker-rm:
			docker rm nanocc

clean:
			rm -f nanocc *.o *~ tmp*

.PHONY: test clean