#ifndef FLEX_BASIC_H
#define FLEX_BASIC_H

#define TO_STRING(value) #value
#define MACRO_TO_STRING(macro) TO_STRING(macro)

void append_objs(char **argv, int start);

#endif /* FLEX_BASIC_H */
