MAKEFILE 	= Makefile

MODULES 	= util drawing string image gl matrix interpolation city

green = '\e[0;32m' $(1) '\e[1;32m'

all: $(MODULES)

$(MODULES):
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
