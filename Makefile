CFLAGS = $(shell sdl-config --cflags) -Iinclude -O2
CCFLAGS = $(CFLAGS)
DEMOLDFLAGS = -Wl,-rpath,. -Lbin -lmonocle $(shell sdl-config --libs)

CPPOBJS = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))
CCOBJS = $(patsubst %.cc,%.o,$(wildcard src/*.cc))
COBJS = $(patsubst %.c,%.o,$(wildcard src/*.c))
OBJS = $(CPPOBJS) $(CCOBJS) $(COBJS)


all: dirs | lib/libmonocle.a bin/libmonocle.so bin/earthball bin/rawtest

lib/libmonocle.a: lib $(OBJS)
	ar cr lib/libmonocle.a $(OBJS)

bin/libmonocle.so: bin $(OBJS)
	g++ -o bin/libmonocle.so -shared $(CCFLAGS) $(OBJS) $(shell sdl-config --libs) -lSDL_mixer -lSDL_image -lz

bin/earthball: bin/libmonocle.so demo/earthball.c bin/earthball-res.zip
	gcc -o bin/earthball $(CFLAGS) demo/earthball.c $(DEMOLDFLAGS)

bin/rawtest: bin/libmonocle.so demo/rawtest.c
	cp demo/resources/rawtest.zip demo/resources/shadow.txt bin/ && gcc -o bin/rawtest $(CFLAGS) demo/rawtest.c $(DEMOLDFLAGS)

bin/earthball-res.zip: demo/resources/earth.png demo/resources/march.it demo/resources/torpedo.wav
	cd demo/resources && zip ../../bin/earthball-res.zip earth.png march.it torpedo.wav

dirs:
	mkdir -p bin && mkdir -p lib

clean:
	rm -rf bin lib $(OBJS)

tidy:
	rm -rf $(OBJS)

depend:
	makedepend -Y. -Iinclude src/*.c src/*.cc src/*.cpp 2> /dev/null

$(CPPOBJS): %.o: %.cpp
	g++ -o $@ -c $(CCFLAGS) -fPIC $<

$(CCOBJS): %.o: %.cc
	g++ -o $@ -c $(CCFLAGS) -fPIC $<

$(COBJS): %.o: %.c
	gcc -o $@ -c $(CFLAGS) -fPIC $<
# DO NOT DELETE

src/audio.o: include/monocle.h
src/framebuffer.o: include/monocle.h
src/meta.o: include/monocle.h
src/raw_data.o: src/exception.h src/raw_data_internal.h include/monocle.h
src/raw_from_zipfile.o: src/raw_data_internal.h include/monocle.h
src/raw_from_zipfile.o: src/exception.h
