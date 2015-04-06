#
C_OPTS = -fstack-protector -Wall

# target specific

wiki : PROG     = wikiudp
wiki : CLIENT   = wikiudp/wikiudp-client.c
wiki : SERVER   = wikiudp/wikiudp-server.c
wiki : OPTS     = wikiudp/wikiudp-opts.c

unpudp : PROG     = unpudp
unpudp : CLIENT   = unpudp/unpudp-c.c
unpudp : SERVER   = unpudp/unpudp-s.c
unpudp : OPTS     = unpudp/unpudp-opts.c

unpudp-cmd : PROG     = unpudp-cmd
unpudp-cmd : CLIENT   = unpudp-c.c
unpudp-cmd : SERVER   = unpudp-cmd-s.c
unpudp-cmd : OPTS     = unpudp-opts.c

unpudp-cmd-v2 : PROG     = unpudp-cmd-v2
unpudp-cmd-v2 : CLIENT   = unpudp-cmd-v2/unpudp-cmd-v2-c.c
unpudp-cmd-v2 : SERVER   = unpudp-cmd-v2/unpudp-cmd-v2-s.c
unpudp-cmd-v2 : OPTS     = unpudp-cmd-v2/unpudp-cmd-v2-opts.c

define clean
	rm -f $(PROG)*.l $(PROG)*.o $(PROG)-c $(PROG)-s bin/$(PROG)
endef
	
define check
	gcc -fsyntax-only $(OPTS) $(CLIENT) $(SERVER)
endef

define build 
	test -f $(CLIENT) && gcc $(C_OPTS) -o bin/$(PROG)-c $(CLIENT)
	test -f $(SERVER) && gcc $(C_OPTS) -o bin/$(PROG)-s $(SERVER) 
endef

help :
	@echo "Targets: runcmd splitl unpudp unpudp-cmd wiki unpudp-cmd-v2"

.PHONY : clean
clean:
	$(clean)

runcmdclean :
	rm runcmd || true

splitl :
	test -f splitl/splitl.c && \
		gcc -fsyntax-only splitl/splitl.c && \
		gcc $(C_OPTS) -o bin/splitl splitl/splitl.c

runcmd : runcmdclean
	test -f runcmd/runcmd.c && \
		gcc -fsyntax-only runcmd/runcmd.c && \
		gcc $(C_OPTS) -o bin/runcmd runcmd/runcmd.c

wiki : clean
	$(check)
	$(build)

unpudp : clean
	$(check)
	$(build)

unpudp-cmd : clean
	$(check)
	$(build)

unpudp-cmd-v2 : clean
	$(check)
	$(build)
