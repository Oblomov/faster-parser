CFLAGS=-std=c99 -Wall -Wextra -O3 -g
LDFLAGS=-flto

RM ?= rm -rf

check-float: check-float.c parse-float.h

check: check-float
	./check-float

clean:
	$(RM) check-float

