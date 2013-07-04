# Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

REVISION = 4
INCLUDES = `pkg-config --cflags gtk+-2.0`
SHAREDCODE = application/*.cxx general/*.cxx script/*
LIBRARIES = `pkg-config --libs gtk+-2.0` -llua -lbz2

FILES = main.cxx settingsdialog.cxx settings.cxx cursorstate.cxx image.cxx gtkwrapper.cxx

DATA = /usr/local/share/inscribist/
BINARY = /usr/local/bin/inscribist

debug: $(FILES)
	g++ -ggdb -pedantic -Wall -Wno-long-long -D REVISION=$(REVISION) \
		-D DATALOCATION=\"data\" -D SETTINGSLOCATION=\"inscribist.conf\" \
		$(INCLUDES) $^ $(LIBRARIES) $(SHAREDCODE) -o inscribist.debug

release: $(FILES)
	g++ -O3 -D NDEBUG -D REVISION=$(REVISION) \
		-D DATALOCATION=\"$(DATA)\" \
		$(INCLUDES) $^ $(LIBRARIES) $(SHAREDCODE) -o inscribist

release-samedir: $(FILES)
	g++ -O3 -D NDEBUG -D REVISION=$(REVISION) \
		$(INCLUDES) $^ $(LIBRARIES) $(SHAREDCODE) -o inscribist

install: inscribist
	-mkdir -p $(DATA)
	cp data/* $(DATA)
	cp inscribist $(BINARY)

package:
	tar --transform 's,^,inscribist-$(REVISION)/,' --show-transformed -jcvhf packages/inscribist-$(REVISION).tar.bz2 \
		Makefile \
		*.cxx *.h \
		application general script \
		data \
		README.txt LICENSE.txt
