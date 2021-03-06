CC 		= 	icc
OPT		= 	-O3
#OPT 		= 	-ggdb -pg

ifeq ($(shell uname), Darwin)
	ARCH=maci64
else
	ARCH=glnxa64
endif

CC_FLAGS 	= 	$(OPT) -Wall -std=c++0x -wd2196 -wd2536 -wd780 \
			-I/usr/local/include/eigen3 -I/opt/intel/mkl/include \
			-I/usr/local/MATLAB/R2011b/extern/include -DSKIP_OPENCV \
			-Qoption,cpp,--rvalue_ctor_is_not_copy_ctor

LD_FLAGS 	= 	-L/usr/X11R6/lib -L/usr/local/MATLAB/R2011b/bin/${ARCH}
LIBS 		= 	-lgflags -lglog -lX11 -lmat -lmx -lgfortran /usr/local/lib/libarpack_LINUX.a

DIR	= `pwd | xargs -I @ basename @`
LIBNAME	= lib$(DIR).a
SRCS	= $(wildcard *.cc)
OBJS	= $(filter-out test%, $(SRCS:.cc=.o))
TOBJS	= $(filter test%, $(SRCS:.cc=.o))
EXT_OBJS= ../util/random.o
TESTS	= $(subst .cc,,$(filter test%, $(SRCS)))

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: $(OBJS) lib

tests: $(TESTS)

$(TESTS): $(OBJS) $(TOBJS) $(EXT_OBJS)
	$(CC) $(LD_FLAGS) $(OBJS) $(filter $@.o, $(TOBJS)) $(EXT_OBJS) $(LIBS) -openmp -mkl=parallel -o $@

lib: $(filter-out test%, $(OBJS))
	@ar rcs $(LIBNAME) $?

clean:
	@rm -rf *.a *.o *.so $(TESTS)
