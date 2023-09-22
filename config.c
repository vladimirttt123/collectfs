#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"

int read_config( config_struct * config, const char* path ){
	config->src = NULL;
	config->lines = NULL;

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
		if( nextLine ) *nextLine = '\0';  // terminate the current line
		if( strlen( curLine ) > 0 && curLine[0] != '#' ){
			config->lines_count ++;
		}
		
		curLine = nextLine ? ( nextLine + 1 ) : (config->src + size);
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


int find_line( config_struct * config, const char * line ){
	for( int i = 0; i < config->lines_count; i++ ){
		if( strcmp( line, config->lines[i] ) == 0 ) return i;
	}

	return -1;
}


void free_config( config_struct * config ){
	if( config->src != NULL ) {
		free( config->src );
		config->src = NULL;
	}
	if( config->lines != NULL ){
		free( config->lines	);	
		config->lines = NULL;
	} 
}