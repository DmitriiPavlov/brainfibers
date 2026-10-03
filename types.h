#include <stdlib.h>
#include <assert.h>
#include <raylib.h>

#ifndef TYPES_H
#define TYPES_H

/* ----------------------------------------------------------------------- */
/* Generic dynamic array                                                    */
/* ----------------------------------------------------------------------- */

#define CONCAT_(a, b) a##b
#define CONCAT(a, b)  CONCAT_(a, b)

#define Array(T) CONCAT(Array_, T)

#define DEFINE_ARRAY(T)                 \
    typedef struct {                    \
        T*     ptr;                     \
        size_t len;                     \
        size_t capacity;                \
    } CONCAT(Array_, T)

#define INIT_ARRAY(T)          ((Array(T)){ (T*)malloc(sizeof(T) * 16), 0, 16 })
#define RESERVE_ARRAY(T, len)  ((Array(T)){ (T*)malloc(sizeof(T) * len), 0, len })

#define PUSH_ARRAY(array, x)                                                    \
    do {                                                                        \
        typeof((x)) __b = (x);                                                  \
        assert((array).len <= array.capacity);                                  \
        if ((array).len == array.capacity) {                                    \
            (array).capacity *= 2;                                              \
            (array).ptr = (typeof(&__b)) realloc(array.ptr,                     \
                                                  sizeof(*array.ptr) * array.capacity); \
        }                                                                       \
        assert(array.ptr != NULL);                                              \
        array.ptr[array.len] = (x);                                             \
        array.len++;                                                            \
    } while (0)

#define POP_ARRAY(array, n)              \
    do {                                  \
        assert(array.len >= n);          \
        array.len -= n;                  \
    } while (0)

#define AT_ARRAY(array, i)  (*(assert((i) < (array).len), &(array).ptr[(i)]))
#define LEN_ARRAY(array)    ((array).len)
#define PTR_ARRAY(array)    ((array).ptr)
#define LAST_ARRAY(array)   ((array).ptr[LEN_ARRAY(array) - 1])

#define FOREACH(array, item)                                          \
    for (size_t _i_##item = 0; _i_##item < (array).len; ++_i_##item)  \
        for (typeof((array).ptr) item = &(array).ptr[_i_##item];      \
             item != NULL;                                            \
             item = NULL)                                        \

#define FOREACH_IDX(array, item, idx)                        \
    for (size_t idx = 0; idx < (array).len; ++idx)            \
        for (typeof((array).ptr) item = &(array).ptr[idx];    \
             item != NULL;                                    \
             item = NULL)

#define FREE_ARRAY(array)         \
    do {                          \
        free(array.ptr);         \
        array.capacity = 0;      \
        array.len = 0;           \
    } while (0)

DEFINE_ARRAY(int);
DEFINE_ARRAY(float);
DEFINE_ARRAY(Array(int));
DEFINE_ARRAY(Array(float));
/* ----------------------------------------------------------------------- */
/* N-Tuple
/* ----------------------------------------------------------------------- */

#define Ntuple(T, n) CONCAT(CONCAT(CONCAT(INTERNAL_TUPLE_, n), _),T)
#define DEFINE_NTUPLE(T, n) \
typedef struct{ \
    T at[n]; \
} Ntuple(T, n)\

#define GET_NTH_ARG(_1,_2,_3,_4,_5,_6,_7,_8,N,...) N
#define COUNT_ARGS(...) GET_NTH_ARG(__VA_ARGS__, 8,7,6,5,4,3,2,1)

#define INIT_TUPLE(T, ...) \
    ((Ntuple(T, COUNT_ARGS(__VA_ARGS__))){ .at = { __VA_ARGS__ } })

DEFINE_NTUPLE(int , 2);
DEFINE_NTUPLE(float , 3);

DEFINE_ARRAY(Ntuple(int, 2));
DEFINE_ARRAY(Array(Ntuple(int, 2)));
DEFINE_ARRAY(Ntuple(float, 3));
DEFINE_ARRAY(Array(Ntuple(float, 3)));

    

/* ----------------------------------------------------------------------- */
/* points                                                                   */
/* ----------------------------------------------------------------------- */

typedef struct {
    float x, y, z;
} Point;

DEFINE_ARRAY(Point);

typedef struct {
    Array(Point) data;
    float xmin;
    float xmax;
    float ymin;
    float ymax;
} Points;

/* ----------------------------------------------------------------------- */
/* Lines                                                                    */
/* ----------------------------------------------------------------------- */

typedef struct {
    int   idx;
    int   len;
    float xmin;
    float xmax;
    float ymin;
    float ymax;
} Line;

DEFINE_ARRAY(Line);
DEFINE_ARRAY(Array(Line));

#define LINE_START(line)  ((line).idx)
#define LINE_END(line)    ((line).idx + (line).len - 1)

typedef struct {
    Array(int)  point_data;
    Array(Line) meta_data;
} Lines;

/* ----------------------------------------------------------------------- */
/* Adjacency list                                                           */
/* ----------------------------------------------------------------------- */

#define MAX_NEIGHBORS 63

#define ADJ_LENGTH(data, i)     AT_ARRAY((data)->adjacency_list, i).len
#define ADJ_ELEMENT(data, i, j) AT_ARRAY((data)->adjacency_list, i).neighbors[j]

typedef struct {
    int neighbors[MAX_NEIGHBORS];
    int len;
} adj_list_element;

DEFINE_ARRAY(adj_list_element);

/* ----------------------------------------------------------------------- */
/* VTKdata                                                                  */
/* ----------------------------------------------------------------------- */

typedef struct {
    /* primitives */

    /* For a point id i, with id j in a line k */

    /* points->ptr[i] is a point id location */
    Points points;

    /* lines->data[lines->meta[k].idx + j] is a point's id */
    Lines lines;

    /* cached data */

    /* neighbor_list is size points.len*ADJACENCY_LIST_SIZE
     * we use -1 for the end of the list for a point
     * neighbor_list[i*(ADJACENCY_LIST_ELEMENT_SIZE+1) + 1] is a point's neighbors */
    Array(adj_list_element) adjacency_list;

    int start_intersections;

} VTKdata;

#define NullPoints ((Points){0})
#define NullLines  ((Lines){0})
#define NullVTK    ((VTKdata){0})

/* ----------------------------------------------------------------------- */
/* Free functions                                                           */
/* ----------------------------------------------------------------------- */

void free_points(Points points) {
    FREE_ARRAY(points.data);
}

void free_lines(Lines lines) {
    FREE_ARRAY(lines.meta_data);
    FREE_ARRAY(lines.point_data);
}

void free_vtkdata(VTKdata data) {
    FREE_ARRAY(data.points.data);
    FREE_ARRAY(data.lines.point_data);
    FREE_ARRAY(data.lines.meta_data);
    if (PTR_ARRAY(data.adjacency_list) != NULL) {
        FREE_ARRAY(data.adjacency_list);
    }
}

/* ----------------------------------------------------------------------- */
/* API                                                                      */
/* ----------------------------------------------------------------------- */

/* every point is referenced by its index in points.ptr, i */

int get_index(const VTKdata data, const int nline, const int n) {
    return AT_ARRAY(data.lines.point_data, AT_ARRAY(data.lines.meta_data, nline).idx + n);
}

Point get_point(const VTKdata data, const int nline, const int n) {
    return AT_ARRAY(data.points.data, get_index(data, nline, n));
}

Vector2 project(Point a){
    return (Vector2){a.x, a.y};
}

Vector2 get_projected_point(const VTKdata data, const int nline, const int n) {
    Point p = get_point(data, nline, n);
    return (Vector2){ p.x, p.y };
}

int first_point(VTKdata data, int k) {
    return AT_ARRAY(data.lines.meta_data, k).idx;
}

int last_point(VTKdata data, int k) {
    Line line = AT_ARRAY(data.lines.meta_data, k);
    return line.idx + line.len - 1;
}

void assert_valid_lines(Lines lines) {
    int total_length = 0;
    for (int i = 0; i < LEN_ARRAY(lines.meta_data); i++) {
        total_length += AT_ARRAY(lines.meta_data, i).len;
    }
    assert(total_length <= LEN_ARRAY(lines.point_data));
}

/* ----------------------------------------------------------------------- */
/* Misc                                                                     */
/* ----------------------------------------------------------------------- */

#define amin(a, b) ((a) < (b) ? (a) : (b))
#define amax(a, b) ((a) > (b) ? (a) : (b))

#endif
