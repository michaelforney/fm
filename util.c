#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include "fm.h"

void *
reallocarray(void *buf, size_t n, size_t m)
{
	if (n > 0 && SIZE_MAX / n < m) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(buf, n * m);
}
