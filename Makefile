all: formulate.html

ENGINE=../engine

CFLAGS=\
	-std=c++17 \
	-g -O0 -Wall -Werror -Wno-unused-private-field
#	-g -Os --profiling
# -fsanitize=address -fsanitize=undefined 

# -Iskia/skia \
# -Iskia/skia/include/core \
# -Iskia/skia/include/config \
# -Iskia/skia/include/docs \
# -Iskia/skia/include/utils \


INC=\
	-Ipdfium/pdfium \
	-I$(ENGINE)/src/flutter/third_party/txt/src/ \
	-I$(ENGINE)/src \
	-I$(ENGINE)/src/third_party/icu/source/common \
	-I$(ENGINE)/src/third_party/skia/include/third_party/skcms \
	-I$(ENGINE)/src/third_party/freetype2/include/ \
	-I$(ENGINE)/src/third_party/skia \
	-I$(ENGINE)/src/third_party/skia/include/core \
	-I$(ENGINE)/src/third_party/skia/include/config \
	-I$(ENGINE)/src/third_party/skia/include/docs \
	-I$(ENGINE)/src/third_party/skia/include/utils \
	-I$(ENGINE)/src/third_party/harfbuzz/src \
	-Inon-test-include

%.o : %.cc
	em++ $(CFLAGS) -MMD $(INC) -o $@ $<

%.o : %.cpp
	em++ $(CFLAGS) -MMD $(INC) -o $@ $<

CLEANOBJS=\
	formulate_bridge.o \
	annotation.o \
	docview.o \
	scrollview.o \
	pdfdoc.o \
	view.o \
	page_cache.o \
	rendercache.o \
	rich_format.o \
	rootview.o \
	svgpath.o \
	thumbnailview.o \
	undo_manager.o \
	flutter_shim.o

OBJS=\
	$(CLEANOBJS) \
	pdfium/pdfium/out/Debug/obj/libpdfium.a \
	$(ENGINE)/src/out/Debug/obj/flutter/third_party/txt/libtxt.a

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
	em++ -c -o $@ \
		-std=c++17 \
		-Iskia/skia \
		-Iskia/skia/include/core \
		-Iskia/skia/include/config \
		$<

formulate.html: $(OBJS) $(MATERIAL_FONTS_FILES) Roboto/Roboto.css icudtl.dat
	em++ $(CFLAGS) -o $@ $(OBJS) \
		-s "EXTRA_EXPORTED_RUNTIME_METHODS=['cwrap']" \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s DEMANGLE_SUPPORT=1 \
		--preload-file usr \
		--preload-file icudtl.dat \
		-s TOTAL_MEMORY=268435456

clean:
	rm -f $(CLEANOBJS)

favicon.png: favicon.svg
	rsvg-convert -w 192 -h 192 --format=png --output=$@ $<
	pngquant -f --speed 1 --output favicon-temp.png $@
	optipng -clobber -o7 -strip all -out $@ favicon-temp.png
	rm favicon-temp.png

signature.html: signature.o
	em++ $(CFLAGS) -o $@ signature.o \
		opencv/opencv/build_wasm/lib/libopencv_core.a \
		opencv/opencv/build_wasm/lib/libopencv_imgproc.a \
		-s "EXTRA_EXPORTED_RUNTIME_METHODS=['cwrap']" \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s USE_LIBPNG=1 \
		-s USE_FREETYPE=1 \
		-s DEMANGLE_SUPPORT=1

DISTFILES=\
	formulate.js \
	formulate.wasm \
	formulate.data \
	Roboto \
	app.html \
	app.js \
	favicon.png \
	style.css \
	menu.js \
	menu.css \
	toolbar.css \
	quill \
	rich_format.js \
	material-icons.css \
	MaterialIcons-Regular.* \
	sig.html \
	sig.js \
	potrace.js \
	sigstore.js \
	signature.js \
	signature.wasm \

dist: signature.html formulate.html
	mkdir -p dist
	cp -Rp $(DISTFILES) dist/

-include *.d
