# qrq Makefile -- Fabian Kurz, DJ1YFK -- http://fkurz.net/ham/qrq.html

VERSION=0.2.1
DESTDIR?=/usr

# set to YES if you want to use Core Audio
# note that you must use Core Audio for OSX
USE_CA=NO

# set to YES if you want to use PulseAudio instead of OSS
USE_PA=YES

# set to YES if building on OSX
OSX_PLATFORM=NO

# set to YES if you want make install to build an OSX bundle instead of
# installing to DESTDIR
# also directs qrq to look in bundle location for shared resources
OSX_BUNDLE=YES

ifneq ($(OSX_PLATFORM), YES)
		OSX_BUNDLE=NO
endif

CFLAGS:=$(CFLAGS) -D DESTDIR=\"$(DESTDIR)\" -D VERSION=\"$(VERSION)\"
CC=gcc

ifeq ($(USE_CA), YES)
		OBJECTS=qrq.o coreaudio.o
		CFLAGS:=$(CFLAGS) -D CA -std=c99
		ifeq ($(OSX_PLATFORM), YES)
			LDFLAGS:=$(LDFLAGS) -framework AudioUnit -framework CoreServices  -isysroot /Developer/SDKs/MacOSX10.6.sdk -mmacosx-version-min=10.5
			CFLAGS:=$(CFLAGS) -isysroot /Developer/SDKs/MacOSX10.6.sdk -mmacosx-version-min=10.5
			ifeq ($(OSX_BUNDLE), YES)
				CFLAGS:=$(CFLAGS) -D OSX_BUNDLE
			endif
		else  # build for iphone/ipad
			LDFLAGS:=$(LDFLAGS) -L iOSExtras/lib -framework AudioToolbox -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS4.3.sdk
			CFLAGS:=$(CFLAGS) -I iOSExtras/include -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS4.3.sdk
			CC:=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/gcc-4.2 -arch armv6
			IPHONE_HOST=root@localhost
			SCP=scp -P2222
			SSH=ssh -p2222
		endif
else ifeq ($(USE_PA), YES)
		CFLAGS:=$(CFLAGS) -D PA
		LDFLAGS:=$(LDFLAGS) -lpulse-simple -lpulse 
		OBJECTS=qrq.o pulseaudio.o
else
		OBJECTS=qrq.o oss.o
		CFLAGS:=$(CFLAGS) -D OSS
endif	

all: qrq

qrq: $(OBJECTS)
	$(CC) -pthread -Wall -o $@ $^ -lm -lncurses $(LDFLAGS)
	
.c.o:
	$(CC) -Wall $(CFLAGS) -c $<

#.cpp.o:
#	g++ $(CFLAGS) -c $<

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
	install    -m 0644 db/callbase.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words1.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words2.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words3.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words4.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words5.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words6.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words7.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words8.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words9.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words10.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words11.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words12.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words13.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words14.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 db/words15.qcb			qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 qrqrc				qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 toplist				qrq.app/Contents/Resources/share/qrq/
	install    -m 0644 OSXExtras/qrq.icns	qrq.app/Contents/Resources/
	install    -m 0755 OSXExtras/qrqsh		qrq.app/Contents/MacOS/
	sed 's/VERSION/$(VERSION)/' OSXExtras/Info.plist > qrq.app/Contents/Info.plist

uninstall:
	rm -rf qrq.app

else

install: qrq
	install -d -v                      $(DESTDIR)/share/qrq/
	install -d -v                      $(DESTDIR)/share/man/man1/
	install -d -v                      $(DESTDIR)/bin/
	install -s -m 0755 qrq             $(DESTDIR)/bin/
	install    -m 0755 qrqscore        $(DESTDIR)/bin/
	install    -m 0644 qrq.1           $(DESTDIR)/share/man/man1/
	install    -m 0644 db/callbase.qcb $(DESTDIR)/share/qrq/
	install    -m 0644 db/words1.qcb   $(DESTDIR)/share/qrq/
	install    -m 0644 db/words2.qcb   $(DESTDIR)/share/qrq/
	install    -m 0644 db/words3.qcb   $(DESTDIR)/share/qrq/
	install    -m 0644 db/words4.qcb   $(DESTDIR)/share/qrq/
	install    -m 0644 db/words5.qcb   $(DESTDIR)/share/qrq/
	install    -m 0644 db/words6.qcb   $(DESTDIR)/share/qrq/
	install    -m 0644 db/words7.qcb   $(DESTDIR)/share/qrq/
	install    -m 0644 db/words8.qcb   $(DESTDIR)/share/qrq/
	install    -m 0644 db/words9.qcb   $(DESTDIR)/share/qrq/
	install    -m 0644 db/words10.qcb  $(DESTDIR)/share/qrq/
	install    -m 0644 db/words11.qcb  $(DESTDIR)/share/qrq/
	install    -m 0644 db/words12.qcb  $(DESTDIR)/share/qrq/
	install    -m 0644 db/words13.qcb  $(DESTDIR)/share/qrq/
	install    -m 0644 db/words14.qcb  $(DESTDIR)/share/qrq/
	install    -m 0644 db/words15.qcb  $(DESTDIR)/share/qrq/
	install    -m 0644 qrqrc           $(DESTDIR)/share/qrq/
	install    -m 0644 toplist         $(DESTDIR)/share/qrq/

	
uninstall:
	rm -f $(DESTDIR)/bin/qrq
	rm -f $(DESTDIR)/bin/qrqscore
	rm -f $(DESTDIR)/share/man/man1/qrq.1
	rm -f $(DESTDIR)/share/qrq/callbase.qcb
	rm -f $(DESTDIR)/share/qrq/words1.qcb
	rm -f $(DESTDIR)/share/qrq/words2.qcb
	rm -f $(DESTDIR)/share/qrq/words3.qcb
	rm -f $(DESTDIR)/share/qrq/words4.qcb
	rm -f $(DESTDIR)/share/qrq/words5.qcb
	rm -f $(DESTDIR)/share/qrq/words6.qcb
	rm -f $(DESTDIR)/share/qrq/words7.qcb
	rm -f $(DESTDIR)/share/qrq/words8.qcb
	rm -f $(DESTDIR)/share/qrq/words9.qcb
	rm -f $(DESTDIR)/share/qrq/words10.qcb
	rm -f $(DESTDIR)/share/qrq/words11.qcb
	rm -f $(DESTDIR)/share/qrq/words12.qcb
	rm -f $(DESTDIR)/share/qrq/words13.qcb
	rm -f $(DESTDIR)/share/qrq/words14.qcb
	rm -f $(DESTDIR)/share/qrq/words15.qcb
	rm -f $(DESTDIR)/share/qrq/qrqrc
	rm -f $(DESTDIR)/share/qrq/toplist
	rmdir $(DESTDIR)/share/qrq/

endif

package: qrq
	export CODESIGN_ALLOCATE=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/codesign_allocate; ldid -s qrq
	rm -rf qrq-pkg
	install -d -v                      qrq-pkg/$(DESTDIR)/share/qrq/
	install -d -v                      qrq-pkg/$(DESTDIR)/share/man/man1/
	install -d -v                      qrq-pkg/$(DESTDIR)/bin/
	install -d -v                      qrq-pkg/DEBIAN/
	install -s -m 0755 qrq             qrq-pkg/$(DESTDIR)/bin/
	install    -m 0755 qrqscore        qrq-pkg/$(DESTDIR)/bin/
	install    -m 0644 qrq.1           qrq-pkg/$(DESTDIR)/share/man/man1/
	install    -m 0644 db/callbase.qcb qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words1.qcb   qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words2.qcb   qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words3.qcb   qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words4.qcb   qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words5.qcb   qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words6.qcb   qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words7.qcb   qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words8.qcb   qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words9.qcb   qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words10.qcb  qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words11.qcb  qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words12.qcb  qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words13.qcb  qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words14.qcb  qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 db/words15.qcb  qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 qrqrc           qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 toplist         qrq-pkg/$(DESTDIR)/share/qrq/
	install    -m 0644 control         qrq-pkg/DEBIAN/
	export COPYFILE_DISABLE=1; export COPY_EXTENDED_ATTRIBUTES_DISABLE=1; dpkg-deb -b qrq-pkg cydiastore_com.kb1ooo.qrq_v$(shell grep ^Version: control | cut -d ' ' -f 2).deb
	$(SCP) cydiastore_com.kb1ooo.qrq_v$(shell grep ^Version: control | cut -d ' ' -f 2).deb $(IPHONE_HOST):/tmp
	$(SSH) $(IPHONE_HOST) "dpkg -i /tmp/cydiastore_com.kb1ooo.qrq_v$(shell grep ^Version: control | cut -d ' ' -f 2).deb"

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
	cp qrq.png qrqscore qrq.c qrqrc db/callbase.qcb toplist \
		AUTHORS ChangeLog README COPYING qrq.1 Makefile \
		db/words1.qcb db/words2.qcb db/words3.qcb db/words4.qcb \
		db/words5.qcb db/words6.qcb db/words7.qcb db/words8.qcb \
	       	db/words9.qcb db/words10.qcb db/words11.qcb db/words12.qcb \
		db/words13.qcb db/words14.qcb db/words15.qcb \
		qrq-$(VERSION)
	cp coreaudio.c coreaudio.h oss.c oss.h \
		qrq-$(VERSION)
	cp pulseaudio.h pulseaudio.c qrq-$(VERSION)
	cp -r OSXExtras qrq-$(VERSION)
	rm -rf qrq-$(VERSION)/OSXExtras/.svn/
	rm -rf qrq-$(VERSION)/db/.svn/
	tar -zcf qrq-$(VERSION).tar.gz qrq-$(VERSION)
	mv qrq-$(VERSION) releases/
	mv qrq-$(VERSION).tar.gz releases/
	md5sum releases/*.gz > releases/md5sums.txt

