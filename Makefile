MAKEFILE 	= Makefile

MODULES 	= util drawing string image gl interpolation city registration svm mpi

all: $(MODULES)

$(MODULES):
	@if [ ! -d ./lib ];\
	then \
		mkdir -p ./lib; \
	fi
	@echo "Building "$@
	@cd code/$@ && make
	@mv code/$@/*.a ./lib/
clean:
	@echo "Cleaning"
	@for i in $(MODULES);\
	do \
		cd code/$$i && make clean && cd ../../; \
	done
	@rm -rf lib/*
