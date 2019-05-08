all: formulate.html

OBJS=\
	formulate.o \
	pdfium/out/Debug/obj/libpdfium.a

INC=\
	-Ipdfium/pdfium \
	-Iskia/skia \
	-Iskia/skia/include/core \
	-Iskia/skia/include/config

%.o : %.cc
	emcc -g -O0 -std=c++14 $(INC) -o $@ $<

formulate.html: $(OBJS)
	emcc -o $@ $(OBJS) -s "EXTRA_EXPORTED_RUNTIME_METHODS=['cwrap']" -s ALLOW_MEMORY_GROWTH=1

TESTOBJS=\
	formulate_bridge.o \
	docview.o \
	scrollview.o \
	pdfdoc.o \
	view.o \
	NotoMono-Regular.ttf.o \
	skia/skia/out/canvaskit_wasm/libskia.a \
	pdfium/pdfium/out/Debug/obj/libpdfium.a

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

NotoMono-Regular.ttf.o : skia/skia/modules/canvaskit/fonts/NotoMono-Regular.ttf.cpp
	emcc -c -o $@ \
		-std=c++14 \
		-Iskia/skia \
		-Iskia/skia/include/core \
		-Iskia/skia/include/config \
		$<

testdoc.html: $(TESTOBJS) $(MATERIAL_FONTS_FILES) Roboto/Roboto.css
	emcc -O0 -g -o $@ $(TESTOBJS) \
		-s "EXTRA_EXPORTED_RUNTIME_METHODS=['cwrap']" \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s USE_LIBPNG=1 \
		-s USE_FREETYPE=1 \
		-s DEMANGLE_SUPPORT=1

favicon.png: favicon.svg
	rsvg-convert -w 192 -h 192 --format=png --output=$@ $<
	pngquant -f --speed 1 --output favicon-temp.png $@
	optipng -clobber -o7 -strip all -out $@ favicon-temp.png
	rm favicon-temp.png

