TARGET = wn
LIBS = -lm -lboost_system -lboost_iostreams -lboost_timer
CC = gcc
CPP = g++

FLAGS = -Wall 

ifeq ($(RELEASE), TRUE)
	FLAGS += -O3
else
	FLAGS += -g -DDEBUG
endif

CFLAGS := $(FLAGS) -Wno-format-security\
   					-Wno-unused-function\
				   	-Wno-unused-variable\
					-Wno-parentheses\
					-Wno-non-literal-null-conversion\
					-Wno-pointer-sign\
					-Wno-format\
					-Wno-missing-braces\
					-Wno-implicit-int

CPPFLAGS = $(FLAGS) -Wno-local-type-template-args

LDFLAGS = -L/usr/local/lib/

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard lib/*.c)) \
			$(patsubst %.c, %.o, $(wildcard src/*.c)) \
           $(patsubst %.cpp, %.o, $(wildcard src/*.cpp))

HEADERS = $(wildcard include/*.h) $(wildcard src/*.h)

%.o: %.c $(HEADERS)
	    $(CC) -I./include/ $(CFLAGS) -c $< -o $@

%.o: %.cpp $(HEADERS)
	    $(CPP) -I ./ -I./include/ -I/usr/local/include/ $(CPPFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	    $(CPP) $(OBJECTS) -Wall $(LDFLAGS) $(LIBS) -o $@

clean:
	   -rm -f src/*.o lib/*.o
	   -rm -f $(TARGET)

