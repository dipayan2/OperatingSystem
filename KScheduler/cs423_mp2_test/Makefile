SUBDIRS = ${shell find . -mindepth 1 -maxdepth 1 -type d -print}

.PHONY: all clean

all clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
