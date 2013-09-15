CFLAGS = $(shell sdl-config --cflags) -Iinclude -O2 -Wall
CCFLAGS = $(CFLAGS)
DEMOLDFLAGS = -Wl,-rpath,. -Lbin -lmonocle $(shell sdl-config --libs)

OBJS = $(patsubst %.c,%.o,$(wildcard src/*.c))

all: dirs | lib/libmonocle.a bin/libmonocle.so bin/earthball bin/rawtest bin/jsontest

lib/libmonocle.a: lib $(OBJS)
	ar cr lib/libmonocle.a $(OBJS)

bin/libmonocle.so: bin $(OBJS)
	gcc -o bin/libmonocle.so -shared $(CCFLAGS) -fvisibility=hidden -fvisibility-inlines-hidden $(OBJS) $(shell sdl-config --libs) -lSDL_mixer -lSDL_image -lz

bin/earthball: bin/libmonocle.so demo/earthball.c bin/earthball-res.zip
	gcc -o bin/earthball $(CFLAGS) demo/earthball.c $(DEMOLDFLAGS)

bin/rawtest: bin/libmonocle.so demo/rawtest.c
	cp demo/resources/rawtest.zip demo/resources/shadow.txt bin/ && gcc -o bin/rawtest $(CFLAGS) demo/rawtest.c $(DEMOLDFLAGS)

bin/jsontest: demo/json-test.c src/json.c src/tree.c src/tree.h
	gcc -o bin/jsontest $(CFLAGS) demo/json-test.c src/tree.c

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
	gcc -o $@ -c $(CFLAGS) -fvisibility=hidden -DMONOCLE_EXPORTS -fPIC $<
# DO NOT DELETE

src/audio.o: include/monocle.h
src/event.o: include/monocle.h
src/framebuffer.o: include/monocle.h
src/json.o: src/json.h src/tree.h
src/meta.o: include/monocle.h src/monocle_internal.h
src/raw_data.o: include/monocle.h src/tree.h
src/tree.o: src/tree.h
