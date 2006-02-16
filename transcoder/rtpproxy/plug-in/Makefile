include Makefile.defs

exclude_modules = recorder
modules = $(filter-out $(exclude_modules) \
		$(wildcard Makefile*) CVS, \
		$(wildcard *) )

.PHONY: all
all: modules

.PHONY: clean
clean:
	-@rm -f *.so
	-@for r in $(modules) "" ; do \
		if [ -n "$$r" ]; then \
			echo "" ; \
			echo "" ; \
			$(MAKE) -C $$r clean ; \
		fi ; \
	done

.PHONY: modules
modules:
	-@for r in $(modules) "" ; do \
		if [ -n "$$r" ]; then \
			echo  "" ; \
			echo  "" ; \
			$(MAKE) -C $$r all; \
		fi ; \
	done 

.PHONY: install
install:
	-@for r in $(modules) "" ; do \
		if [ -n "$$r" ]; then \
			echo "" ; \
			echo "" ; \
			$(MAKE) -C $$r install; \
		fi ; \
	done
