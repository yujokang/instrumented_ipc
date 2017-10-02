#include "flex_basic.h"

#include <stdlib.h>

static char *objs[] = {MACRO_TO_STRING(RT_OBJ),
				MACRO_TO_STRING(CHILD_OBJ)};

void append_objs(char **argv, int start)
{
	int obj_i;

	for (obj_i = 0; obj_i < sizeof(objs) / sizeof(*objs); obj_i++) {
		argv[start + obj_i] = objs[obj_i];
	}

	argv[start + obj_i] = NULL;
}
