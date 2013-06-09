CFLAGS = `sdl-config --cflags` -I../include
CCFLAGS = $(CFLAGS)

CPPOBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
CCOBJS = $(patsubst %.cc,%.o,$(wildcard *.cc))
COBJS = $(patsubst %.c,%.o,$(wildcard *.c))
OBJS = $(CPPOBJS) $(CCOBJS) $(COBJS)

all: ../libmonocle.a

../libmonocle.a: $(OBJS)
	ar cr ../libmonocle.a $(OBJS)

clean:
	rm -rf ../libmonocle.a $(OBJS)

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
