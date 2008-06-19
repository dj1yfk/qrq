# qrq Makefile -- Fabian Kurz, DJ1YFK -- http://fkurz.net/ham/qrq.html

VERSION=0.1.4
DESTDIR ?= /usr

all: qrq

qrq: qrq.c
	gcc qrq.c -pthread $(CFLAGS) -Wall -O2 -g -lm -lncurses \
			-D DESTDIR=\"$(DESTDIR)\" -D VERSION=\"$(VERSION)\" -o qrq

install:
	install -d -v               $(DESTDIR)/share/qrq/
	install -d -v               $(DESTDIR)/share/man/man1/
	install -d -v               $(DESTDIR)/bin/
	install -s -m 0755 qrq      $(DESTDIR)/bin/
	install    -m 0755 qrqscore $(DESTDIR)/bin/
	install    -m 0644 qrq.1    $(DESTDIR)/share/man/man1/
	install    -m 0644 callbase $(DESTDIR)/share/qrq/
	install    -m 0644 qrqrc    $(DESTDIR)/share/qrq/
	install    -m 0644 toplist  $(DESTDIR)/share/qrq/
	
uninstall:
	rm -f $(DESTDIR)/bin/qrq
	rm -f $(DESTDIR)/bin/qrqscore
	rm -f $(DESTDIR)/share/man/man1/qrq.1
	rm -f $(DESTDIR)/share/qrq/callbase
	rm -f $(DESTDIR)/share/qrq/qrqrc
	rm -f $(DESTDIR)/share/qrq/toplist
	rmdir $(DESTDIR)/share/qrq/

clean:
	rm -f qrq toplist-old *~

dist:
	sed 's/Version [0-9].[0-9].[0-9]/Version $(VERSION)/g' README > README2
	rm -f README
	mv README2 README
	rm -f releases/qrq-$(VERSION).tar.gz
	rm -rf releases/qrq-$(VERSION)
	mkdir qrq-$(VERSION)
	cp qrq.png qrqscore qrq.c qrqrc callbase toplist CHANGELOG README \
			COPYING qrq.1 Makefile qrq-$(VERSION)
	tar -zcf qrq-$(VERSION).tar.gz qrq-$(VERSION)
	mv qrq-$(VERSION) releases/
	mv qrq-$(VERSION).tar.gz releases/
	md5sum releases/*.gz > releases/md5sums.txt

