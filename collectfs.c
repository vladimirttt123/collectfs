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

static void *collect_init(struct fuse_conn_info *conn,
												struct fuse_config *cfg)
{
		(void) conn;
		
		// seems we do not need system cach on our FS because it is just reads from other and there need cache
		cfg->kernel_cache = 0; // was 1

		return NULL;
}

static int collect_getattr(const char *path, struct stat *stbuf,
												 struct fuse_file_info *fi)
{
	(void) fi;
	int res = 0;

	// printf( "getting attributes of %s\n", path );

	memset( stbuf, 0, sizeof(struct stat) );
	if (strcmp(path, "/") == 0 || strcmp(path, ".") == 0 || strcmp(path,"..") == 0 ) {
	 				stbuf->st_mode = S_IFDIR | 0755;
	 				stbuf->st_nlink = 2;
	} else {
		int64_t fsize = -1;
		int line = find_line( &config, path + 1 /*skip first slash*/ );
		if( line >= 0 ){
			fsize = 0;
			struct stat path_stat;
			for( line++; line < config.lines_count && config.lines[line][0] == '/'; line++ ){
				if( stat( config.lines[line], &path_stat ) == 0
						&& S_ISREG( path_stat.st_mode ) ){
					fsize += path_stat.st_size;
				}
			}
		}
		if( fsize >= 0 ) {
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
			stbuf->st_size = fsize;
		} else 
			res = -ENOENT;
	}

	return res;
}


static int collect_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
												 off_t offset, struct fuse_file_info *fi,
												 enum fuse_readdir_flags flags)
{
		(void) offset;
		(void) fi;
		(void) flags;

		if( strcmp(path, "/") != 0 )
			return -ENOENT; // exists only root folder

		filler(buf, ".", NULL, 0, 0);
		filler(buf, "..", NULL, 0, 0);

		for( int i = 0; i < config.lines_count; i ++ )
			if( config.lines[i][0] != '/' )
				filler( buf, config.lines[i], NULL, 0, 0 );	 
		
		return 0;
}

static int collect_open( const char *path, struct fuse_file_info *fi )
{
	if ((fi->flags & O_ACCMODE) != O_RDONLY)
		return -EACCES;

	return -EACCES;
}

static int collect_release(const char * path, struct fuse_file_info *fi ){
	return 0;
}


static int collect_read(const char *path, char *buf, size_t size, off_t offset,
											struct fuse_file_info *fi)
{
	return 0;
}

static const struct fuse_operations collect_oper = {
				.init           = collect_init,
				.getattr        = collect_getattr,
				.readdir        = collect_readdir,
				.open           = collect_open,
				.read           = collect_read,
				.release				= collect_release
};

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

	// for( int i = 0; i < config.lines_count; i++ )
	// 	printf( "%s\n", config.lines[i] );
	// This allow to show src directory in mount output
	// do we need to free previous value?
	//	args.argv[0] = strdup( options.srcdir );


	ret = fuse_main(args.argc, args.argv, &collect_oper, NULL);
	fuse_opt_free_args(&args);

	return ret;
}
