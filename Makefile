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
	formulate_bridge.o \
	docview.o \
	skia/out/canvaskit_wasm/libskia.a

MATERIAL_FONTS_FILES=\
	MaterialIcons-Regular.woff2 \
	MaterialIcons-Regular.woff \
	MaterialIcons-Regular.ttf \
	material-icons.css

MATERIAL_GIT_BASE := https://raw.githubusercontent.com/google/material-design-icons/master/iconfont/

$(MATERIAL_FONTS_FILES) :
	curl -O $(MATERIAL_GIT_BASE)$@

Roboto/Roboto.css :
	./download_font.sh 'https://fonts.googleapis.com/css?family=Roboto'

testdoc.html: $(TESTOBJS) material-icons.css
	emcc -o $@ $(TESTOBJS) \
		-std=c++14 \
		-Iskia/include/core \
		-Iskia/include/config \
		skia/modules/canvaskit/fonts/NotoMono-Regular.ttf.cpp \
		-s "EXTRA_EXPORTED_RUNTIME_METHODS=['cwrap']" \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s USE_LIBPNG=1 \
		-s USE_FREETYPE=1
