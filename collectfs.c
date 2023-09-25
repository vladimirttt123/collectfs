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

const char * collectfs_ver = "0.0.1";
const char * fs_info_name = ".fsinfo";

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
static int64_t *file_sizes;

static int get_fs_info_size(){
	int info_size = 1 + config.lines_count*2 + (strlen(collectfs_ver) + 2);
	for( int i = 0; i < config.lines_count; i++ )
		info_size += strlen( config.lines[i] ) + snprintf( NULL, 0, "%ld", file_sizes[i] );

	return info_size;
}

static int get_fs_info( char *buf, size_t size, off_t offset ){
	int info_size = get_fs_info_size();
	
	char * res = (char*)malloc( info_size + 1 );
	memset( res, 0, info_size );
	
	for( int i = 0, len = snprintf( res, info_size, "v%s\n", collectfs_ver ); 
					i < config.lines_count; i++ )
		len += snprintf( res + len, info_size-len, "%ld\t%s\n", file_sizes[i], config.lines[i] );

	if( offset > info_size ) return 0;
	if( (offset+size) > info_size ) 
		size = info_size - offset;
	memcpy( buf, res + offset, size );

	free( res );

	return size;
}

static int64_t get_file_size( int base_line ){

	if( base_line < 0 || base_line >= config.lines_count || config.lines[base_line][0] == '/' ) 
		return -1; // incorrect base line
	
	if( file_sizes[base_line] < 0 ) {
		// reinit
		struct stat path_stat;
		int64_t size = 0;
		for( int i = base_line+1; i < config.lines_count && config.lines[i][0] == '/'; i++ ){
			
			if( file_sizes[i] < 0 ) {
				if( stat( config.lines[i], &path_stat ) != 0 ){
					printf( "Error: cannot stat %s", config.lines[i] );
					return file_sizes[i] = -2;
				}
				if( !S_ISREG( path_stat.st_mode ) ){
					printf( "%s is not a regular file", config.lines[i] );
					return file_sizes[i] = -3;
				}

				file_sizes[i] = path_stat.st_size;
			} 
			size += file_sizes[i];
		}

		file_sizes[base_line] = size; // everithing OK
	}
	
	return file_sizes[base_line];
}

static void *collect_init(struct fuse_conn_info *conn,
												struct fuse_config *cfg)
{
		(void) conn;
		
		// seems we do not need system cach on our FS because it is just reads from other and there need cache
		cfg->kernel_cache = 0; // was 1

		// init files sizes 
		file_sizes = (int64_t*)malloc( sizeof(int64_t)*config.lines_count );
		for( int i = 0; i < config.lines_count; i++ )
			file_sizes[i] = -1; // not inited or has a problem

		return NULL;
}

static int collect_getattr(const char *path, struct stat *stbuf,
												 struct fuse_file_info *fi)
{
	(void) fi;

	memset( stbuf, 0, sizeof(struct stat) );
	if (strcmp(path, "/") == 0 || strcmp(path, ".") == 0 || strcmp(path,"..") == 0 ) {
	 				stbuf->st_mode = S_IFDIR | 0555;
	 				stbuf->st_nlink = 2;
	} else {
		if( strcmp( path + 1, fs_info_name ) == 0 ){
			stbuf->st_size = get_fs_info_size();
		}else {
			int line = find_line( &config, path + 1 /*skip first slash*/ );
			if( line < 0 || config.lines[line][0] == '/' )
				return -ENOENT;

			stbuf->st_size = get_file_size( line );
		}

		if( stbuf->st_size < 0 ) return -EIO;

		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
	}

	return 0;
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

	if( strcmp( path + 1, fs_info_name ) == 0 ) return 0;

	int line = find_line( &config, path+1 );
	if( path[1] == '/' || line < 0 )
		return -ENOENT;

	if( get_file_size(line) < 0 ) return -EIO;

	return 0;
}

static int collect_release(const char * path, struct fuse_file_info *fi ){
	return 0;
}


static int collect_read(const char *path, char *buf, size_t size, off_t offset,
											struct fuse_file_info *fi)
{
	if( strcmp( path + 1, fs_info_name) == 0 )
		return get_fs_info( buf, size, offset );

	int base_line = find_line( &config, path+1 );
	if( base_line < 0 || path[1] == '/' ) return -ENOENT;

	off_t fsize = get_file_size( base_line );
	if( fsize < 0 ) return -EIO; // file has problems
	if( offset >= fsize ) return 0; // read after end of file

	off_t cur_offset = 0;
	size_t bytes_read = 0;

	for( int line = base_line + 1; line < config.lines_count && config.lines[line][0] == '/'; line++ ){

		if( offset > cur_offset ){
			if( fsize <= offset - cur_offset ){
				cur_offset += fsize;
				continue;
			}
		}
		FILE *f = fopen( config.lines[line], "r" );
		if( f == NULL ){
			file_sizes[base_line] = file_sizes[line] = -1;
			return -EIO;
		}
		if( offset > cur_offset ){
			int seek_res = fseek( f, offset - cur_offset, SEEK_SET );
			if( seek_res != 0 ){
				file_sizes[base_line] = file_sizes[line] = -1;
				fclose( f );
				return -EIO;
			}
		
			cur_offset += ftell( f );
		}

		if( offset != cur_offset ) {
			file_sizes[base_line] = file_sizes[line] = -1;
			fclose( f );
			return -EIO; // impossible problem
		}

		size_t cur_read = fread( buf + bytes_read, 1, size - bytes_read, f );
		bytes_read += cur_read;

		fclose( f );

		if( bytes_read == size ) return bytes_read;
	}

	return bytes_read;
}

static const struct fuse_operations collect_oper = {
				.init           = collect_init,
//				.destroy				= collect_destroy,
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

	if( config.lines_count == 0 ){
		printf( "Config is empty" );
		return 5;
	}


	// This allow to show src directory in mount output
	// do we need to free previous value?
	//	args.argv[0] = strdup( options.srcdir );


	ret = fuse_main(args.argc, args.argv, &collect_oper, NULL);
	fuse_opt_free_args(&args);
	free(file_sizes); // TODO move to destroy
	free_config( &config );
	
	return ret;
}
