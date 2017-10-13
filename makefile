TARGET = main.out
SOURCES = main.cpp chip8.cpp gpu.cpp
OBJECTS = $(SOURCES:.cpp=.o)
CXXFLAGS = -std=c++14 -Wall -Wextra
LDFLAGS = $(shell sdl2-config --cflags --libs)

all: ${TARGET}

clean:
	rm ${OBJECTS} ${TARGET}

${TARGET}: ${SOURCES}
	${LINK.cc} -o $@ $^
