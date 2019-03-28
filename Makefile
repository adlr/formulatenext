all: formulate.html

OBJS=\
	formulate.o \
	pdfium/out/Debug/obj/libpdfium.a

%.o : %.cc
	emcc -std=c++14 -Ipdfium -o $@ $<

formulate.html: $(OBJS)
	emcc -o $@ $(OBJS) -s "EXTRA_EXPORTED_RUNTIME_METHODS=['cwrap']" -s ALLOW_MEMORY_GROWTH=1
