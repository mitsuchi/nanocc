CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

nanocc: $(OBJS)
			$(CC) -o nanocc $(OBJS) $(LDFLAGS)

$(OBJS): nanocc.h

test: nanocc
			./test.sh

docker-build:
			docker build -t nanocc:1 .

docker-run:
			docker run -it -v $(CURDIR):/nanocc --name nanocc nanocc:1

docker-test:
			docker run --rm -v $(CURDIR):/nanocc --name nanocc nanocc:1 make test

docker-rm:
			docker rm nanocc

clean:
			rm -f nanocc *.o *~ tmp*

.PHONY: test clean