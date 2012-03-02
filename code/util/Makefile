CC 		= g++
CC_FLAGS 	= -O3 -Wall
#CC_FLAGS 	= -ggdb -Wall
LD_FLAGS 	= 
LIBS 		= 

SRCS	= $(wildcard *.cc)
OBJS	= $(SRCS:.cc=.o)

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: lib

lib: $(OBJS)
	@ar rcs libutil.a $(OBJS)

clean:
	@rm -rf *.a *.o *.so
