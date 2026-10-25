PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
PKG_CONFIG ?= pkg-config
CFLAGS ?= -O2

LIB_CFLAGS = $(shell $(PKG_CONFIG) --cflags libcurl openssl)
LIB_LIBS = $(shell $(PKG_CONFIG) --libs libcurl openssl)

.PHONY: all clean install
all: bbslink_client

bbslink_client: bbslink_client.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -Wextra -Wpedantic $(LIB_CFLAGS) -o $@ $< $(LDFLAGS) $(LIB_LIBS)

install: bbslink_client
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 bbslink_client $(DESTDIR)$(BINDIR)/bbslink_client

clean:
	rm -f bbslink_client

.PHONY: test
test:
	python3 tests/test_mock.py
