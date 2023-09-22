#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
	char * src;
	int lines_count;
	char ** lines;
} config_struct;

int read_config( config_struct * config, const char* path );
int find_line( config_struct * config, const char * line );
void free_config( config_struct * config );

#endif // CONFIG_H