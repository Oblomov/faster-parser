CFLAGS=-std=c99 -Wall -Wextra -O3 -g
LDFLAGS=-flto

RM ?= rm -rf

check: check-float
	./check-float

bench: bench-float
	./bench-float

debug: debug-float

check-float: check-float.c parse-float.h
debug-float: debug-float.c parse-float.h
bench-float: bench-float.c parse-float.h

clean:
	$(RM) check-float

