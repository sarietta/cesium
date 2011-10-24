MAKEFILE 	= Makefile
CC_FLAGS  	= -O3 -Wall
LD_FLAGS	=

MODULES 	= util drawing

green = '\e[0;32m' $(1) '\e[1;32m'

all: $(MODULES)

$(MODULES):
	@echo "Building "$@
	@cd code/$@ && make CC_FLAGS="$(CC_FLAGS)" LD_FLAGS="$(LD_FLAGS)"
	@mv code/$@/*.a ./lib/
clean:
	@echo "Cleaning"
	cd code/util && make clean
	@rm -rf lib/*
