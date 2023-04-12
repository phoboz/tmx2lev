CXX = g++
CFLAGS = -Wall -DVERTICAL
INCFLAGS =
LIBS = -lz
LDFLAGS =
OUTPUT = tmx2lev

.cpp.o:
	$(CXX) $(CFLAGS) $(INCFLAGS) -c $*.c

OBJS = tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp \
       base64.cpp TmxImage.cpp TmxLayer.cpp TmxMap.cpp TmxObject.cpp \
       TmxObjectGroup.cpp TmxPolygon.cpp TmxPolyline.cpp TmxPropertySet.cpp \
       TmxTile.cpp TmxTileset.cpp TmxUtil.cpp \
       main.cpp

all: tmx2bin

tmx2bin: $(OBJS)
	$(CXX) -o $(OUTPUT) $(CFLAGS) $(OBJS) $(LIBS) $(LDFLAGS)

clean:
	rm *.o; rm $(OUTPUT)


