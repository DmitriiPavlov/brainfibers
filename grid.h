#include "types.h"

#ifndef GRID_H
#define GRID_H

#define CONCAT_(a, b) a##b
#define CONCAT(a, b)  CONCAT_(a, b)

#define discreteSpace(T) CONCAT(discreteSpace_, T)*
#define CLAMP_INT(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))
#define CLAMP_FLOAT(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

#define DEFINE_DISCRETE_SPACE(T) \
typedef struct { \
    float xmin; \
    float xmax; \
    float ymin; \
    float ymax; \
    int   xresolution; \
    int   yresolution; \
    Array(T) array; \
} CONCAT(discreteSpace_, T); \
\
T CONCAT(discreteSpaceGet_, T)(discreteSpace(T) space, float x, float y){ \
    int discretex = (int)(((x - space->xmin)/(space->xmax - space->xmin)) * space->xresolution); \
    int discretey = (int)(((y - space->ymin)/(space->ymax - space->ymin)) * space->yresolution); \
    discretex = CLAMP_INT(discretex, 0, space->xresolution - 1); \
    discretey = CLAMP_INT(discretey, 0, space->yresolution - 1); \
    return AT_ARRAY(space->array, discretex + space->xresolution*discretey); \
} \
T CONCAT(discreteSpaceSet_, T)(discreteSpace(T) space, float x, float y, T a){ \
    int discretex = (int)(((x - space->xmin)/(space->xmax - space->xmin)) * space->xresolution); \
    int discretey = (int)(((y - space->ymin)/(space->ymax - space->ymin)) * space->yresolution); \
    discretex = CLAMP_INT(discretex, 0, space->xresolution - 1); \
    discretey = CLAMP_INT(discretey, 0, space->yresolution - 1); \
    AT_ARRAY(space->array, discretex + space->xresolution*discretey) = a; \
} \
Ntuple(int, 2) CONCAT(discreteSpaceConvert_, T)(discreteSpace(T) space, float x, float y){ \
    int a = (int)(((x - space->xmin)/(space->xmax - space->xmin)) * space->xresolution); \
    int b = (int)(((y - space->ymin)/(space->ymax - space->ymin)) * space->yresolution); \
    a = CLAMP_INT(a, 0, space->xresolution - 1); \
    b = CLAMP_INT(b, 0, space->yresolution - 1); \
    return INIT_TUPLE(int, a, b); \
} \

DEFINE_DISCRETE_SPACE(int);
DEFINE_DISCRETE_SPACE(float);
DEFINE_DISCRETE_SPACE(Array(int));
DEFINE_DISCRETE_SPACE(Array(Line));
DEFINE_DISCRETE_SPACE(Array(Ntuple(int, 2)));

#define DISCRETE_SPACE_GET(T) CONCAT(discreteSpaceGet_, T)
#define DISCRETE_SPACE_SET(T) CONCAT(discreteSpaceSet_, T)
#define DISCRETE_SPACE_CONVERT(T) CONCAT(discreteSpaceConvert_, T)
#define DISCRETE_DIRECT(space, pair) AT_ARRAY(space->array, pair.at[0] + pair.at[1]*space->xresolution)

#define discrete_get(space, x, y) _Generic((space), \
    discreteSpace(int):        DISCRETE_SPACE_GET(int), \
    discreteSpace(float):      DISCRETE_SPACE_GET(float), \
    discreteSpace(Array(int)): DISCRETE_SPACE_GET(Array(int)), \
    discreteSpace(Array(Line)): DISCRETE_SPACE_GET(Array(Line)), \
    discreteSpace(Array(Ntuple(int,2))): DISCRETE_SPACE_GET(Array(Ntuple(int, 2))) \
)(space, x, y)

#define discrete_set(space, x, y, a) _Generic((space), \
    discreteSpace(int):        DISCRETE_SPACE_SET(int), \
    discreteSpace(float):      DISCRETE_SPACE_SET(float), \
    discreteSpace(Array(int)): DISCRETE_SPACE_SET(Array(int)), \
    discreteSpace(Array(Line)): DISCRETE_SPACE_SET(Array(Line)), \
    discreteSpace(Array(Ntuple(int,2))): DISCRETE_SPACE_SET(Array(Ntuple(int, 2))) \
)(space, x, y, a)

#define discrete_convert(space, x, y) _Generic((space), \
    discreteSpace(int):        DISCRETE_SPACE_CONVERT(int), \
    discreteSpace(float):      DISCRETE_SPACE_CONVERT(float), \
    discreteSpace(Array(int)): DISCRETE_SPACE_CONVERT(Array(int)), \
    discreteSpace(Array(Line)): DISCRETE_SPACE_CONVERT(Array(Line)), \
    discreteSpace(Array(Ntuple(int,2))): DISCRETE_SPACE_CONVERT(Array(Ntuple(int, 2))) \
)(space, x, y)


#define DISCRETE_CREATE(space_type, xres, yres) \
    ({ \
        space_type _space = malloc(sizeof(*_space)); \
        assert(_space != NULL); \
        _space->xmin = 0; \
        _space->xmax = 0; \
        _space->ymin = 0; \
        _space->ymax = 0; \
        _space->xresolution = (xres); \
        _space->yresolution = (yres); \
        _space->array = (typeof(_space->array)){ \
            malloc(sizeof(*_space->array.ptr) * (xres) * (yres)), \
            (xres) * (yres), \
            (xres) * (yres) \
        }; \
        _space; \
    })

#define DISCRETE_DESTROY(space) \
    do { \
        FREE_ARRAY((space)->array); \
        free(space); \
    } while (0)



#endif


