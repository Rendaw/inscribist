INCLUDES = `pkg-config --cflags gtk+-2.0` -I/home/rendaw/created/library
LIBRARIES = `pkg-config --libs gtk+-2.0` -L/home/rendaw/created/library -lgeneral -lapplication -lscript -llua -lbz2

sketch: main.cxx settingsdialog.cxx settings.cxx cursorstate.cxx runsketcher.cxx simplesketcher.cxx gtkwrapper.cxx
	g++ -ggdb $(INCLUDES) $^ $(LIBRARIES) -o sketch
