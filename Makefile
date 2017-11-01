# Makefile

CC := gcc
CFLAGS := -O2 -Wall -Wextra $(CFLAGS)
LDFLAGS := -lrt

default: __phony all
all: __phony
clean: __phony

.PHONY: __phony
__phony: ;

%.o: %.c
	gcc $(CFLAGS) -m32 -c $< -o $@

usched_switch.o:%.o: %.s
	gcc $(CFLAGS) -m32 -c $< -o $@

clean: clean_objects
clean_objects: __phony
	rm -f *.o

all: test
test: usched.o usched_switch.o test.o
	gcc $(CFLAGS) -m32 $^ $(LDFLAGS) -o $@

clean: clean_test
clean_test: __phony
	rm -f test

# vim: set ts=4 sw=4 noet syn=make:
