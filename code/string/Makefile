CC 		= g++
CC_FLAGS 	= -O3 -Wall
LD_FLAGS 	= 
LIBS 		= 

SRCS	= $(wildcard *.cc)
OBJS	= $(SRCS:.cc=.o)

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: lib

lib: $(OBJS)
	@ar rcs libstring.a $(OBJS)

clean:
	@rm -rf *.a *.o *.so
