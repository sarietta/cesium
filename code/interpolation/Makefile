CC 		= g++
CC_FLAGS 	= -O3 -Wall -I/usr/local/include/eigen3 -I/opt/intel/mkl/include \
	 	  -ggdb -pg

LD_FLAGS 	= -L/usr/X11R6/lib
LIBS 		= -lglog -lX11

DIR	= `pwd | xargs -I @ basename @`
LIBNAME	= lib$(DIR).a
SRCS	= $(wildcard *.cc)
OBJS	= $(filter-out test%, $(SRCS:.cc=.o))
TOBJS	= $(filter test%, $(SRCS:.cc=.o))
TESTS	= $(subst .cc,,$(filter test%, $(SRCS)))

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: $(OBJS) lib

tests: $(TESTS)

$(TESTS): $(OBJS) $(TOBJS)
	$(CC) $(LD_FLAGS) $(LIBS) $(OBJS) $(TOBJS) -o $@

lib: $(filter-out test%, $(OBJS))
	@ar rcs $(LIBNAME) $?

clean:
	@rm -rf *.a *.o *.so $(TESTS)
