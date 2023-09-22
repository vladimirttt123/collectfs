#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "config.h"

const char * collectfs_ver = "0.0.0";

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct options {
				char *config;
				char *dstdir;
				int show_help;
} options;

#define OPTION(t, p)                           \
		{ t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
				OPTION("-h", show_help),
				OPTION("--help", show_help),
				FUSE_OPT_END
};

static config_struct config;

static void show_help(const char *progname)
{
	printf( "collectfs ver %s\n", collectfs_ver );
	printf( "usage: %s [options] <configfile> <mountpoint>\n\n", progname );
	printf( "File-system specific options:\n"
				  "\n");
}


static int argument_parser( void *data, const char *arg, int key, struct fuse_args *outargs ){
	if( key == FUSE_OPT_KEY_NONOPT ){
		if( options.config == NULL ){
			options.config = realpath( arg, NULL );
			if( options.config == NULL ){
				printf( "Source path cannot be resolved\n" );
				return -1;
			}
			return 0;
		}
		if( options.dstdir != NULL ) {
			printf( "incorrect parameter %s\n", arg );
			return -1;
		}
		options.dstdir = realpath( arg, NULL );
		if( options.dstdir == NULL ){
			printf( "Destination path cannot be resolved\n" );
			return -1;
		}
	}

	return 1;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* Set defaults -- we have to use strdup so that
		 fuse_opt_parse can free the defaults if other
		 values are specified */
	options.config = options.dstdir = NULL;

	/* Parse options */
	if( fuse_opt_parse(&args, &options, option_spec, argument_parser) == -1 )
		return 1;

	/* When --help is specified, first print our own file-system
		 specific help text, then signal fuse_main to show
		 additional help (by adding `--help` to the options again)
		 without usage: line (by setting argv[0] to the empty
		 string) */
	if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
	}
	else {
		// check source dir not inside mount point
		if( options.config == NULL ){
			printf( "no config file \n" );
			return 2;
		}
		if( options.dstdir == NULL ){
			printf( "no mount point\n" );
			return 3;
		}

	}


	ret = read_config( &config, options.config );
	if( ret != 0 ){
		printf( "cannot read config %i\n", ret );
		return 4;
	}

	for( int i = 0; i < config.lines_count; i++ )
		printf( "%s\n", config.lines[i] );
	// This allow to show src directory in mount output
	// do we need to free previous value?
	//	args.argv[0] = strdup( options.srcdir );


	// ret = fuse_main(args.argc, args.argv, &plotz_oper, NULL);
	fuse_opt_free_args(&args);
	return ret;
}
