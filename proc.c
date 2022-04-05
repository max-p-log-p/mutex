#include <err.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
usage(char *str)
{
	fprintf(stderr, "usage: %s\n", str);
	exit(1);
}

void
getHosts(const char *pathname, char s[][HOST_NAME_MAX], size_t len)
{
	FILE *stream;
	uint64_t i;

	if ((stream = fopen(pathname, "r")) == NULL)
		err(1, "fopen");

	/* Get servers */
	for (i = 0; i < len; ++i) {
		if (fgets(s[i], HOST_NAME_MAX, stream) == NULL)
			err(1, "fgets");
		s[i][strcspn(s[i], "\n")] = '\0';
	}

	if (fclose(stream))
		warn("fclose");
}
