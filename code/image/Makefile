CC 		= icc
OPT		= -O3
#OPT		= -ggdb
CC_FLAGS 	= ${OPT} -Wall -std=c++0x -wd2196 -wd2536 -wd780 -I../ `pkg-config --cflags opencv` -DSKIP_OPENCV \
		  -I/usr/local/include/eigen3
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
