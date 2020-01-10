# qrq Makefile -- Fabian Kurz, DJ1YFK -- http://fkurz.net/ham/qrq.html

VERSION?=0.3.4
DESTDIR?=/usr

# set to YES if you want to use Core Audio
# note that you must use Core Audio for OSX
USE_CA=NO

# set to YES if you want to use PulseAudio instead of OSS
USE_PA=YES

# set to YES if you compile with MINGW32
USE_WIN32=NO

# set to YES if building on OSX
OSX_PLATFORM=NO

# set to YES if you want make install to build an OSX bundle instead of
# installing to DESTDIR
# also directs qrq to look in bundle location for shared resources
OSX_BUNDLE=YES

ifneq ($(OSX_PLATFORM), YES)
		OSX_BUNDLE=NO
endif

ifeq ($(USE_WIN32), YES)
	CC=i686-w64-mingw32-gcc-posix
else
	CC=gcc
endif

INSTALL ?= install

# build information may be added by CI scripts and will be
# displayed in the help text (qrq -h)

BUILD_INFO=" Date: $(shell date +%Y-%m-%dT%H:%M:%S%z)\n git branch/commit: $(shell if [ x${CI_COMMIT_REF_NAME} != x ]; then echo ${CI_COMMIT_REF_NAME} ; elif [ -e .git ] || [ -e ../.git ]; then git symbolic-ref --short HEAD ; else echo not built from git repository ; fi) / $(shell if [ x${CI_COMMIT_SHA} != x ]; then echo ${CI_COMMIT_SHA}; elif [ -e .git ] || [ -e ../.git ] ; then git rev-parse --verify HEAD ; else echo - ; fi )\n by $(USER)@$(shell hostname -f)\n OS: $(shell uname -s -r -p)\n Compiler: $(shell $(CC) --version | head -1)"

CFLAGS:=$(CFLAGS) -D DESTDIR=\"$(DESTDIR)\" -D BUILD_INFO=\"$(BUILD_INFO)\" -D VERSION=\"$(VERSION)\" -I.

ifeq ($(USE_CA), YES)
		OBJECTS=qrq.o coreaudio.o
		CFLAGS:=$(CFLAGS) -D CA -std=c99 -pthread
		ifeq ($(OSX_PLATFORM), YES)
			LDFLAGS:=$(LDFLAGS) -framework AudioUnit -framework CoreServices -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk 
			CFLAGS:=$(CFLAGS) -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
			ifeq ($(OSX_BUNDLE), YES)
				CFLAGS:=$(CFLAGS) -D OSX_BUNDLE
			endif
		else  # build for iphone/ipad
			LDFLAGS:=$(LDFLAGS) -L iOSExtras/lib -framework AudioToolbox -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk
			CFLAGS:=$(CFLAGS) -I iOSExtras/include -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk
			CC:=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/gcc-4.2 -arch armv6
			IPHONE_HOST=root@localhost
			SCP=scp -P2222
			SSH=ssh -p2222
		endif
else ifeq ($(USE_PA), YES)
		CFLAGS:=$(CFLAGS) -D PA -pthread
		LDFLAGS:=$(LDFLAGS) -lpthread -lpulse-simple -lpulse -lncurses
		OBJECTS=qrq.o pulseaudio.o
else ifeq ($(USE_WIN32), YES)
		CFLAGS:=$(CFLAGS) -D WIN32 -Iinclude -Iinclude/ncursesw
		LDFLAGS:=$(LDFLAGS) -lwinmm -lpthread
		OBJECTS=qrq.o qrq.res lib/libncursesw.a
else
		OBJECTS=qrq.o oss.o
		LDFLAGS:=$(LDFLAGS) -lpthread -lncurses
		CFLAGS:=$(CFLAGS) -D OSS
endif

all: qrq

qrq: $(OBJECTS)
	$(CC) -Wall -o $@ $^ -lm $(LDFLAGS)
	
.c.o:
	$(CC) -Wall $(CPPFLAGS) $(CFLAGS) -c $<

#.cpp.o:
#	g++ $(CFLAGS) -c $<

ifeq ($(OSX_BUNDLE), YES)

install: qrq
	${INSTALL} -d -v							qrq.app/Contents
	${INSTALL} -d -v							qrq.app/Contents/MacOS
	${INSTALL} -d -v							qrq.app/Contents/Resources
	${INSTALL} -d -v							qrq.app/Contents/Resources/share/qrq/
	${INSTALL} -d -v							qrq.app/Contents/Resources/share/man/man1/
	${INSTALL}    -m 0755 qrq					qrq.app/Contents/MacOS/
	${INSTALL}    -m 0755 qrqscore				qrq.app/Contents/MacOS/
	${INSTALL}    -m 0644 qrq.1				qrq.app/Contents/Resources/share/man/man1/
	${INSTALL}    -m 0644 callbase.qcb				qrq.app/Contents/Resources/share/qrq/
	${INSTALL}    -m 0644 english.qcb				qrq.app/Contents/Resources/share/qrq/
	${INSTALL}    -m 0644 qrqrc				qrq.app/Contents/Resources/share/qrq/
	${INSTALL}    -m 0644 toplist				qrq.app/Contents/Resources/share/qrq/
	${INSTALL}    -m 0644 OSXExtras/qrq.icns	qrq.app/Contents/Resources/
	${INSTALL}    -m 0755 OSXExtras/qrqsh		qrq.app/Contents/MacOS/
	sed 's/VERSION/$(VERSION)/' OSXExtras/Info.plist > qrq.app/Contents/Info.plist

uninstall:
	rm -rf qrq.app

else

install: qrq
	${INSTALL} -d -v                      $(DESTDIR)/share/qrq/
	${INSTALL} -d -v                      $(DESTDIR)/share/man/man1/
	${INSTALL} -d -v                      $(DESTDIR)/bin/
	${INSTALL} -s -m 0755 qrq             $(DESTDIR)/bin/
	${INSTALL}    -m 0755 qrqscore        $(DESTDIR)/bin/
	${INSTALL}    -m 0644 qrq.1           $(DESTDIR)/share/man/man1/
	${INSTALL}    -m 0644 callbase.qcb    $(DESTDIR)/share/qrq/
	${INSTALL}    -m 0644 english.qcb     $(DESTDIR)/share/qrq/
	${INSTALL}    -m 0644 qrqrc           $(DESTDIR)/share/qrq/
	${INSTALL}    -m 0644 toplist         $(DESTDIR)/share/qrq/

	
uninstall:
	rm -f $(DESTDIR)/bin/qrq
	rm -f $(DESTDIR)/bin/qrqscore
	rm -f $(DESTDIR)/share/man/man1/qrq.1
	rm -f $(DESTDIR)/share/qrq/callbase.qcb
	rm -f $(DESTDIR)/share/qrq/english.qcb
	rm -f $(DESTDIR)/share/qrq/qrqrc
	rm -f $(DESTDIR)/share/qrq/toplist
	rmdir $(DESTDIR)/share/qrq/

endif

package: qrq
	export CODESIGN_ALLOCATE=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/codesign_allocate; ldid -s qrq
	rm -rf qrq-pkg
	${INSTALL} -d -v                      qrq-pkg/$(DESTDIR)/share/qrq/
	${INSTALL} -d -v                      qrq-pkg/$(DESTDIR)/share/man/man1/
	${INSTALL} -d -v                      qrq-pkg/$(DESTDIR)/bin/
	${INSTALL} -d -v                      qrq-pkg/DEBIAN/
	${INSTALL} -s -m 0755 qrq             qrq-pkg/$(DESTDIR)/bin/
	${INSTALL}    -m 0755 qrqscore        qrq-pkg/$(DESTDIR)/bin/
	${INSTALL}    -m 0644 qrq.1           qrq-pkg/$(DESTDIR)/share/man/man1/
	${INSTALL}    -m 0644 callbase.qcb    qrq-pkg/$(DESTDIR)/share/qrq/
	${INSTALL}    -m 0644 english.qcb     qrq-pkg/$(DESTDIR)/share/qrq/
	${INSTALL}    -m 0644 qrqrc           qrq-pkg/$(DESTDIR)/share/qrq/
	${INSTALL}    -m 0644 toplist         qrq-pkg/$(DESTDIR)/share/qrq/
	${INSTALL}    -m 0644 control         qrq-pkg/DEBIAN/
	export COPYFILE_DISABLE=1; export COPY_EXTENDED_ATTRIBUTES_DISABLE=1; dpkg-deb -b qrq-pkg cydiastore_com.kb1ooo.qrq_v$(shell grep ^Version: control | cut -d ' ' -f 2).deb
	$(SCP) cydiastore_com.kb1ooo.qrq_v$(shell grep ^Version: control | cut -d ' ' -f 2).deb $(IPHONE_HOST):/tmp
	$(SSH) $(IPHONE_HOST) "dpkg -i /tmp/cydiastore_com.kb1ooo.qrq_v$(shell grep ^Version: control | cut -d ' ' -f 2).deb"

clean:
	rm -f qrq.exe qrq toplist-old *~ *.o
	rm -rf Summary
	rm -rf qrq.app

dist:
	sed 's/Version [0-9].[0-9].[0-9]/Version $(VERSION)/g' ../README > README
	mkdir -p ../releases
	rm -f ../releases/qrq-$(VERSION).tar.gz
	rm -rf ../releases/qrq-$(VERSION)
	mkdir qrq-$(VERSION)
	cp qrq.png qrqscore qrq.c qrqrc callbase.qcb toplist \
		../AUTHORS ../ChangeLog README ../COPYING qrq.1 Makefile qrqinstaller.nsi \
		english.qcb qrq.ico qrq.rc \
		qrq-$(VERSION)
	cp coreaudio.c coreaudio.h oss.c oss.h \
		qrq-$(VERSION)
	cp pulseaudio.h pulseaudio.c qrq-$(VERSION)
	cp -r OSXExtras qrq-$(VERSION)
	rm -rf qrq-$(VERSION)/OSXExtras/.svn/
	tar -zcf qrq-$(VERSION).tar.gz qrq-$(VERSION)
	mv qrq-$(VERSION) ../releases/
	mv qrq-$(VERSION).tar.gz ../releases/
	md5sum ../releases/*.gz > ../releases/md5sums.txt

