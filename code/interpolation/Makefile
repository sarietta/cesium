CC 		= g++
CC_FLAGS 	= -O3 -Wall -I/usr/local/include/eigen3 -I/opt/intel/mkl/include \
	 	  -ggdb -pg

LD_FLAGS 	= -L/opt/intel/mkl/lib/intel64
LIBS 		= -lmkl_core

SRCS	= $(wildcard *.cc)
OBJS	= $(SRCS:.cc=.o)

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: lib

lib: $(OBJS)
	@ar rcs libinterpolation.a $(OBJS)

clean:
	@rm -rf *.a *.o *.so
