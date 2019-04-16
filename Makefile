all: formulate.html

OBJS=\
	formulate.o \
	pdfium/out/Debug/obj/libpdfium.a

INC=\
	-Ipdfium \
	-Iskia/include/core \
	-Iskia/include/config

%.o : %.cc
	emcc -std=c++14 $(INC) -o $@ $<

formulate.html: $(OBJS)
	emcc -o $@ $(OBJS) -s "EXTRA_EXPORTED_RUNTIME_METHODS=['cwrap']" -s ALLOW_MEMORY_GROWTH=1

TESTOBJS=\
	testdoc.o \
	docview.o \
	skia/out/canvaskit_wasm/libskia.a

testdoc.html: $(TESTOBJS)
	emcc -o $@ $(TESTOBJS) \
		-std=c++14 \
		-Iskia/include/core \
		-Iskia/include/config \
		skia/modules/canvaskit/fonts/NotoMono-Regular.ttf.cpp \
		-s "EXTRA_EXPORTED_RUNTIME_METHODS=['cwrap']" \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s USE_LIBPNG=1 \
		-s USE_FREETYPE=1
