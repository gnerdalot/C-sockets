#
C_OPTS = -fstack-protector -Wall

# target specific

unpudp-cmd : PROG     = udp-cmd
unpudp-cmd : CLIENT   = udp-cmd-c.c
unpudp-cmd : SERVER   = udp-cmd-s.c
unpudp-cmd : OPTS     = udp-cmd-opts.c

define clean
	rm -f $(PROG)*.l $(PROG)*.o $(PROG)-c $(PROG)-s bin/$(PROG)
endef
	
define check
	gcc -fsyntax-only $(OPTS) $(CLIENT) $(SERVER)
endef

define build 
	test -f $(CLIENT) && gcc $(C_OPTS) -o ../bin/$(PROG)-c $(CLIENT)
	test -f $(SERVER) && gcc $(C_OPTS) -o ../bin/$(PROG)-s $(SERVER) 
endef

.PHONY : clean

unpudp-cmd : clean
	$(check)
	$(build)

clean:
	$(clean)

