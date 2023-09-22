COMPILER = gcc
CFLAGS = -g

HEADERS = Makefile config.h
FILESYSTEM_FILES = collectfs.c config.c
FILESYSTEM_HEADERS = $(HEADERS)

all: collectfs 

collectfs: $(FILESYSTEM_FILES) $(HEADERS)
	$(COMPILER) $(FILESYSTEM_FILES) -o collectfs -Wall `pkg-config fuse3 --cflags --libs` ${CFLAGS}
	echo 'To Mount: ./collectfs [config file] [mount point]'


clean:
	rm -f collectfs
