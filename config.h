#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
	char * src;
	int lines_count;
	char ** lines;
} config_struct;

int read_config( config_struct * config, const char* path );

#endif // CONFIG_H