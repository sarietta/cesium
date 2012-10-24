CC 		= g++
CC_FLAGS 	= -O3 -Wall -I../ `pkg-config --cflags opencv` -I/usr/local/include/eigen3
#CC_FLAGS 	= -ggdb -Wall -I../ `pkg-config --cflags opencv` -I/usr/local/include/eigen3
LD_FLAGS 	= 
LIBS 		= `pkg-config --libs opencv`

SRCS	= $(wildcard *.cc)
OBJS	= $(SRCS:.cc=.o)

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: lib

lib: $(OBJS)
	@ar rcs libimage.a $(OBJS)

clean:
	@rm -rf *.a *.o *.so
