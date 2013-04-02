CC 		= 	icc
OPT		= 	-O3 -msse2
#OPT 		= 	-ggdb -pg
CC_FLAGS 	= 	$(OPT) -Wall -std=c++0x -wd2196 -wd2536 -wd780 \
			-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX -DSKIP_OPENCV
LD_FLAGS 	= 	
LIBS 		= 	-lgflags -lglog 

DIR	= `pwd | xargs -I @ basename @`
LIBNAME	= lib$(DIR).a
SRCS	= $(wildcard *.cc)
OBJS	= $(filter-out test%, $(SRCS:.cc=.o))
TOBJS	= $(filter test%, $(SRCS:.cc=.o))
TESTS	= $(subst .cc,,$(filter test%, $(SRCS)))
EXTOBJS = 

%.o:%.cc
	$(CC) $(CC_FLAGS) -c $< 

all: $(OBJS) lib

tests: $(TESTS)

$(TESTS): $(OBJS) $(TOBJS) 
	$(CC) $(LD_FLAGS) $(OBJS) $(EXTOBJS) $(filter $@.o, $(TOBJS)) $(LIBS) -o $@

lib: $(filter-out test%, $(OBJS))
	@ar rcs $(LIBNAME) $?

clean:
	@rm -rf *.a *.o *.so $(TESTS)
