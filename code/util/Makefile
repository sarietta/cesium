CC 		= 	g++
OPT		= 	-O3
#OPT 		= 	-ggdb -pg
CC_FLAGS 	= 	$(OPT) -Wall -I/usr/local/include/eigen3 -I/usr/include/eigen3 -I/opt/intel/mkl/include \
			-I/usr/local/MATLAB/R2011b/extern/include -DSKIP_OPENCV

LD_FLAGS 	= 	-L/usr/X11R6/lib -L/usr/local/MATLAB/R2011b/bin/glnxa64
LIBS 		= 	-lgflags -lglog -lX11 -lmat -lmx

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

$(TESTS): $(OBJS) $(TOBJS) $(EXT_OBJS)
	$(CC) $(LD_FLAGS) $(OBJS) $(filter $@.o, $(TOBJS)) $(EXT_OBJS) $(LIBS) -o $@

lib: $(filter-out test%, $(OBJS))
	@ar rcs $(LIBNAME) $?

clean:
	@rm -rf *.a *.o *.so $(TESTS)
