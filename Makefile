COMPILER = gcc
CFLAGS = -g

HEADERS = Makefile config.h
FILESYSTEM_FILES = collectfs.c config.c
FILESYSTEM_HEADERS = $(HEADERS)

all: collectfs 

collectfs: $(FILESYSTEM_FILES) $(HEADERS)
	$(COMPILER) $(FILESYSTEM_FILES) -o collectfs -Wall `pkg-config fuse3 --cflags --libs` ${CFLAGS}
	echo 'To Mount: ./collectfs [config file] [mount point]'

install: collectfs
	cp ./collectfs /usr/bin/collectfs
	/bin/chown root:root /usr/bin/collectfs
	/bin/chmod 0755 /usr/bin/collectfs
	ln -s ../bin/collectfs /usr/sbin/mount.collectfs

uninstall:
	rm /usr/sbin/mount.collectfs /usr/bin/collectfs

clean:
	rm -f collectfs
