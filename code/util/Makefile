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
			-I./mcr/v716/extern/include -DSKIP_OPENCV \
			-Qoption,cpp,--rvalue_ctor_is_not_copy_ctor

LD_FLAGS 	= 	-L/usr/X11R6/lib -L./mcr/v716/runtime/glnxa64/ -L./mcr/v716/bin/glnxa64/
LIBS 		= 	-lgflags -lglog -lX11 -lmat -lmx -lmwmclmcrrt

DIR	= `pwd | xargs -I @ basename @`
LIBNAME	= lib$(DIR).a
SRCS	= $(wildcard *.cc)
OBJS	= $(filter-out test%, $(SRCS:.cc=.o))
TOBJS	= $(filter test%, $(SRCS:.cc=.o))  
EXT_OBJS= ../svm/detector.o ../image/feature_pyramid.o ../image/feature_computer.o
TESTS	= $(subst .cc,,$(filter test%, $(SRCS)))

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: $(OBJS) lib

tests: $(TESTS)

$(TESTS): $(OBJS) $(TOBJS)
	$(CC) $(LD_FLAGS) $(OBJS) $(filter $@.o, $(TOBJS)) $(EXT_OBJS) $(LIBS) ./matlabfunc/distrib/matlabfunc.so -o $@

lib: $(filter-out test%, $(OBJS))
	@ar rcs $(LIBNAME) $?

clean:
	@rm -rf *.a *.o *.so $(TESTS)
