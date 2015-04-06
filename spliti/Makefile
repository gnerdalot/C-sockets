C_OPTS = -fstack-protector -Wall
SHELL := /bin/bash
PROG = ../bin/spliti

spliti : PROG = ../bin/spliti
spliti : SRC  = spliti.c

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

spliti : clean
	$(check)
	$(build)

clean: 
	$(clean)

test : spliti
	echo dddddddddddddddddddddddddddddddddddddddddddddddddddddddd | $(PROG) 10



