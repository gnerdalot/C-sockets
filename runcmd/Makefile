C_OPTS = -fstack-protector -Wall
SHELL := /bin/bash

runcmd: PROG = ../bin/runcmd
runcmd: SRC  = runcmd.c

define clean 
	rm -f $(PROG)*.l $(PROG).o $(PROG)-c $(PROG)-s bin/$(PROG)
endef

define check
	gcc -fsyntax-only $(SRC)
endef

define build
	test -f $(SRC) && gcc $(C_OPTS) -o $(PROG) $(SRC)
endef

.PHONY : clean

runcmd:
	$(check)
	$(build)

clean: 
	$(clean)

test:
	echo dddddddddddddddddddddddddddddddddddddddddddddddddddddddd | $(PROG)



