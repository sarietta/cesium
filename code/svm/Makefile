CC 		= g++
OPT		= -O3
#OPT 		= -ggdb -pg

ifeq ($(shell uname), Darwin)
	ARCH=maci64
else
	ARCH=glnxa64
endif

CC_FLAGS = 	$(OPT) -Wall -I/usr/include/eigen3 \
	 	-I${HOME}/Development/slib/code \
		-I/usr/local/MATLAB/R2011b/extern/include \
		-DSKIP_OPENCV

LD_FLAGS = 	-L/usr/local/MATLAB/R2011b/bin/${ARCH}
LIBS = 		${HOME}/Development/slib/lib/libimage.a -lglog -lgflags -lmat -lmx

DIR	= `pwd | xargs -I @ basename @`
LIBNAME	= lib$(DIR).a
SRCS	= $(wildcard *.cc)
OBJS	= $(filter-out test%, $(SRCS:.cc=.o)) ../image/feature_computer.o
TOBJS	= $(filter test%, $(SRCS:.cc=.o))
TESTS	= $(subst .cc,,$(filter test%, $(SRCS)))

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: $(OBJS) lib

tests: $(TESTS)

$(TESTS): $(OBJS) $(TOBJS)
	$(CC) $(LD_FLAGS) $(LIBS) $(OBJS) $(filter $@.o, $(TOBJS)) -o $@

lib: $(filter-out test%, $(OBJS))
	@ar rcs $(LIBNAME) $?

clean:
	@rm -rf *.a *.o *.so $(TESTS)
