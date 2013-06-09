CFLAGS = `sdl-config --cflags` -I../include -O2
CCFLAGS = $(CFLAGS)

CPPOBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
CCOBJS = $(patsubst %.cc,%.o,$(wildcard *.cc))
COBJS = $(patsubst %.c,%.o,$(wildcard *.c))
OBJS = $(CPPOBJS) $(CCOBJS) $(COBJS)

all: ../libmonocle.a ../earthball ../earthball-res.zip

../libmonocle.a: $(OBJS)
	ar cr ../libmonocle.a $(OBJS)

../earthball: ../libmonocle.a ../demo/earthball.c
	g++ -o ../earthball $(CFLAGS) ../demo/earthball.c -L.. -lmonocle `sdl-config --libs` -lSDL_mixer -lSDL_image -lz

../earthball-res.zip: ../demo/resources/earth.png ../demo/resources/march.it
	cd ../demo/resources && zip ../../earthball-res.zip earth.png march.it

clean:
	rm -rf ../libmonocle.a ../earthball ../earthball-res.zip $(OBJS)

tidy:
	rm -rf $(OBJS)

depend:
	makedepend -Y. -I../include -p$(BUILDDIR)/ *.c *.cc *.cpp 2> /dev/null

$(CPPOBJS): %.o: %.cpp
	g++ -o $@ -c $(CCFLAGS) $<

$(CCOBJS): %.o: %.cc
	g++ -o $@ -c $(CCFLAGS) $<

$(COBJS): %.o: %.c
	gcc -o $@ -c $(CFLAGS) $<
