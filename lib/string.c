#include <string.h>

size_t strlen(const char *str) {
	size_t len = 0;
	for(len = 0; *str++ != '\0'; len++);
	return(len);
}


