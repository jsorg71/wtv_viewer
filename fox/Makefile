
OBJS=wtv_viewer.o wtv_picker.o wtv_fullscreen.o wtv_mainwindow.o

FOX_CFLAGS=$(shell fox-config --cflags)
FOX_LIBS=$(shell fox-config --libs)

CXXFLAGS=-O2 -g -Wall -Wextra -I.. $(FOX_CFLAGS) -fvisibility=hidden -fPIC

LDFLAGS=

LIBS=$(FOX_LIBS) ../wtv.a -lpthread -lpulse -lX11 -lxcb -lX11-xcb -lxcb-dri3 -lxcb-render

all:
	$(MAKE) -C ..
	$(MAKE) wtv_viewer

wtv_viewer: $(OBJS) ../wtv.a
	$(CXX) -o wtv_viewer $(OBJS) $(LDFLAGS) $(LIBS)

clean:
	$(MAKE) -C .. clean
	rm -f wtv_viewer $(OBJS)

%.o: %.cpp *.h
	$(CXX) $(CXXFLAGS) -c $<

