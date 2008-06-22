# qrq Makefile -- Fabian Kurz, DJ1YFK -- http://fkurz.net/ham/qrq.html

VERSION=0.1.4
DESTDIR?=/usr

# set to YES if you want to use OpenAL instead of OSS
# note that you must use OpenAL for OSX
USE_OPENAL=NO

# set to YES if building on OSX
OSX_PLATFORM=NO

# set to YES if you want make install to build an OSX bundle instead of installing to DESTDIR
# also directs qrq to look in bundle location for shared resources
OSX_BUNDLE=YES

ifneq ($(OSX_PLATFORM), YES)
		OSX_BUNDLE=NO
endif

CFLAGS:=$(CFLAGS) -D DESTDIR=\"$(DESTDIR)\" -D VERSION=\"$(VERSION)\"

ifeq ($(USE_OPENAL), YES)
		OBJECTS=qrq.o OpenAlImp.o OpenAlStream.o
		CFLAGS:=$(CFLAGS) -D OPENAL
		ifeq ($(OSX_PLATFORM), YES)
			LDFLAGS:=$(LDFLAGS) -framework OpenAL
			ifeq ($(OSX_BUNDLE), YES)
				CFLAGS:=$(CFLAGS) -D OSX_BUNDLE
			endif
		else
			LDFLAGS:=$(LDFLAGS) -lopenal
		endif
else
		OBJECTS=qrq.o oss.o
endif

all: qrq

qrq: $(OBJECTS)
	g++ -pthread -Wall -lm -lncurses $(LDFLAGS) -o $@ $^
	
.c.o:
	gcc $(CFLAGS) -c $<

.cpp.o:
	g++ $(CFLAGS) -c $<

ifeq ($(OSX_BUNDLE), YES)

install: qrq
	install -d -v							qrq.app/Contents
	install -d -v							qrq.app/Contents/MacOS
	install -d -v							qrq.app/Contents/Resources
	install -d -v							qrq.app/Contents/Resources/share/qrq/
	install -d -v							qrq.app/Contents/Resources/share/man/man1/
	install    -m 0755 qrq					qrq.app/Contents/MacOS/
	install    -m 0755 qrqscore				qrq.app/Contents/MacOS/
	install    -m 0644 qrq.1				qrq.app/Contents/Resources/share/man/man1/
	install    -m 0644 callbase				qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 qrqrc				qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 toplist				qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 OSXExtras/qrq.icns	qrq.app/Contents/Resources/
	install    -m 0755 OSXExtras/qrqsh		qrq.app/Contents/MacOS/
	sed 's/VERSION/$(VERSION)/' OSXExtras/Info.plist > qrq.app/Contents/Info.plist

uninstall:
	rm -rf qrq.app

else

install: qrq
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

endif

clean:
	rm -f qrq toplist-old *~ *.o
	rm -rf qrq.app

dist:
	sed 's/Version [0-9].[0-9].[0-9]/Version $(VERSION)/g' README > README2
	rm -f README
	mv README2 README
	rm -f releases/qrq-$(VERSION).tar.gz
	rm -rf releases/qrq-$(VERSION)
	mkdir qrq-$(VERSION)
	cp qrq.png qrqscore qrq.c oss.c oss.h qrqrc callbase toplist CHANGELOG README \
			COPYING qrq.1 Makefile qrq-$(VERSION)
	cp OpenAlImp.h OpenAlImp.cpp OpenAlStream.cpp OpenAlStream.h qrq-$(VERSION)
	cp -r OSXExtras qrq-$(VERSION)
	rm -rf qrq-$(VERSION)/OSXExtras/.svn/
	tar -zcf qrq-$(VERSION).tar.gz qrq-$(VERSION)
	mv qrq-$(VERSION) releases/
	mv qrq-$(VERSION).tar.gz releases/
	md5sum releases/*.gz > releases/md5sums.txt

