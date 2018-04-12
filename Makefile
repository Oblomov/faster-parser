CFLAGS=-std=c99 -Wall -Wextra -O3 -g -march=native
LDFLAGS=-flto

RM ?= rm -rf

TARGETS=check-float bench-float bench-int debug-float debug-simd

all: $(TARGETS)

check: check-float
	./check-float

bench: bench-float bench-int
	./bench-float && ./bench-int

debug: debug-float debug-simd

check-float: check-float.c parse-float.h
debug-float: debug-float.c parse-float.h
debug-simd: debug-simd.c simd-int.h parse-int.h
bench-float: bench-float.c parse-float.h
bench-int: bench-int.c parse-int.h

clean:
	$(RM) $(TARGETS)

