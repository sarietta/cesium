CC 		= icc
CC_FLAGS 	= -O3 -Wall -std=c++0x -wd2196 -wd2536 -wd780
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
