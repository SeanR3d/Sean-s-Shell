# Makefile for CIS3207-02 lab 2: Shell
# by Sean Reddington
# shell: shell2.c shell2.h
# 	gcc -g shell2.c -o shell

# Makefile for CIS3207-02 lab 2: Shell
# by Sean Reddington

TARGETS = shell

CC_C = $(CROSS_TOOL)gcc

CFLAGS = -g
all: clean $(TARGETS) 

$(TARGETS): shell.h
	$(CC_C) $(CFLAGS) $@.c -o $@

clean:
	rm -f $(TARGETS)