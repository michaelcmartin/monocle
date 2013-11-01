ifeq ($(OS),Windows_NT)
    OSCFLAGS=
    OSLDFLAGS=
    MONOCLEBIN=libmonocle.dll
else
    OSCFLAGS=-fvisibility=hidden -fPIC
    OSLDFLAGS=-fvisibility=hidden
    MONOCLEBIN=libmonocle.so
endif

CFLAGSNOSDL = -Iinclude -O2 -Wall
CFLAGS = $(shell sdl-config --cflags) $(CFLAGSNOSDL)
DEMOLDFLAGS = -Wl,-rpath,. -Lbin -lmonocle $(shell sdl-config --libs)

OBJS = $(patsubst %.c,%.o,$(wildcard src/*.c))

all: dirs | lib/libmonocle.a bin/$(MONOCLEBIN) bin/earthball bin/rawtest bin/jsontest

lib/libmonocle.a: lib $(OBJS)
	ar cr lib/libmonocle.a $(OBJS)

bin/$(MONOCLEBIN): bin $(OBJS)
	gcc -o bin/$(MONOCLEBIN) -shared $(CFLAGS) $(OSLDFLAGS) -fvisibility-inlines-hidden $(OBJS) $(shell sdl-config --libs) -lSDL_mixer -lSDL_image -lz

bin/earthball: bin/$(MONOCLEBIN) demo/earthball.c bin/earthball-res.zip
	gcc -o bin/earthball $(CFLAGS) demo/earthball.c $(DEMOLDFLAGS)

bin/rawtest: bin/$(MONOCLEBIN) demo/rawtest.c
	cp demo/resources/rawtest.zip demo/resources/shadow.txt bin/ && gcc -o bin/rawtest $(CFLAGS) demo/rawtest.c $(DEMOLDFLAGS)

bin/jsontest: demo/json-test.c src/json.c src/tree.c src/tree.h
	gcc -o bin/jsontest $(CFLAGSNOSDL) demo/json-test.c src/tree.c

bin/earthball-res.zip: demo/resources/earth.png demo/resources/march.it demo/resources/torpedo.wav demo/resources/earthball.json
	cd demo/resources && zip ../../bin/earthball-res.zip earth.png march.it torpedo.wav earthball.json

dirs:
	mkdir -p bin && mkdir -p lib

clean:
	rm -rf bin lib $(OBJS)

tidy:
	rm -rf $(OBJS)

depend:
	makedepend -Y. -Iinclude src/*.c src/*.cc src/*.cpp 2> /dev/null

$(OBJS): %.o: %.c
	gcc -o $@ -c $(CFLAGS) $(OSCFLAGS) -DMONOCLE_EXPORTS $<
# DO NOT DELETE

src/audio.o: include/monocle.h
src/event.o: include/monocle.h
src/framebuffer.o: include/monocle.h src/monocle_internal.h
src/json.o: include/monocle.h
src/meta.o: include/monocle.h src/monocle_internal.h
src/raw_data.o: include/monocle.h src/tree.h
src/resource.o: include/monocle.h src/tree.h
src/tree.o: src/tree.h include/monocle.h
