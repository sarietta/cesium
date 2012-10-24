CC 		= g++
CC_FLAGS 	= -O3 -Wall -I../ `pkg-config --cflags opencv`
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
