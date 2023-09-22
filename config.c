#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"

int read_config( config_struct * config, const char* path ){

	// Part I fully read config file and close
	FILE *f = fopen( path, "r" ); // open config file 
	if( f == NULL ) return -1; // cannot open config file

	if( fseek( f, 0, SEEK_END ) != 0 ) return -2; // cannot seek to end
	int64_t size = ftell( f );
	if( fseek( f, 0, SEEK_SET ) ) return -3; // cannot seek to start

	config->src = (char*)malloc( size + 1);
	if( fread( config->src, 1, size, f ) != size ) {
		free( config->src );
		return -4; // cannot fully read
	}
	fclose( f );

	config->src[size] = 0; // end of config

	// count lines
	config->lines_count = 0;
	char * curLine = config->src;
	while( curLine[0] != 0 ){
		char * nextLine = strchr(curLine, '\n');
		if( nextLine ){
			*nextLine = '\0';  // terminate the current line
			if( strlen( curLine ) > 0 && curLine[0] != '#' ){
				config->lines_count ++;
			}
			curLine = nextLine + 1;
		} 
		else curLine = config->src + size; // end of loop
	}


	if( config->lines_count == 0 ) {
		free( config->src );
		return -4; // empty config
	}

	config->lines = (char**)malloc( sizeof(char*) * config->lines_count );
	curLine = config->src;
	for( int i = 0; i < config->lines_count; ){
		int len = strlen( curLine );
		if( len > 0 && curLine[0] != '#' )
			config->lines[i++] = curLine;
		
		curLine += len + 1;
	}

	return 0;
}
