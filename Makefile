LIB := libhal.so
VER := 1
PREFIX ?= /usr

all: $(LIB)

$(LIB): libhal_stub.c 
	$(CC) -shared -o $@ $< -fPIC -Wall
clean:
	$(RM) $(LIB)

install: $(LIB)
	install -D $(LIB) $(PREFIX)/lib/$(LIB).$(VER)
	ln -s $(LIB).$(VER) $(PREFIX)/lib/$(LIB) 

uninstall:
	$(RM) $(PREFIX)/lib/$(LIB).$(VER)
	$(RM) $(PREFIX)/lib/$(LIB)
