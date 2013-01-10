CC 		= icc
CC_FLAGS 	= -O3 -Wall -std=c++0x -wd2196 -wd2536 -wd780 -I../ `pkg-config --cflags opencv` -DSKIP_OPENCV
LD_FLAGS 	= 
LIBS 		= `pkg-config --libs opencv`

SRCS	= $(wildcard *.cc)
OBJS	= $(SRCS:.cc=.o)

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: lib

lib: $(OBJS)
	@ar rcs libdrawing.a $(OBJS)

clean:
	@rm -rf *.a *.o *.so
