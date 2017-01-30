#ifndef CGL_UTIL_H
#define CGL_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#define SQUARE(x)  ((x)*(x))
#define SQUARE_UL(X) SQUARE(((uint64_t) (X)))

#define MALLOC_ARRAY(var, size) ((var) = malloc((size)*sizeof(*(var))))
#define CALLOC_ARRAY(var, size) ((var) = calloc((size), sizeof(*(var))))
#define ASSERT_MSG(msg, cond) assert(  ((void)(msg), (cond)) )

#endif // CGL_UTIL_H
