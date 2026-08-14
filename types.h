#include <stdlib.h>
#include <assert.h>
#include <raylib.h>

#ifndef TYPES_H
#define TYPES_H

typedef struct {
    float x, y, z;
} Point;

typedef struct {
    Point* ptr;
    int    len;
    float xmin;
    float xmax;
    float ymin;
    float ymax;
} Points;

typedef struct {
    int  idx;
    int  len;
} Line;

typedef struct {
    int*  data;
    int   dlen;
    Line* meta;
    int   mlen;
} Lines;


typedef struct {
    Points points;
    Lines lines;
} VTKdata;

#define NullPoints ((Points){0})
#define NullLines  ((Lines){0})
#define NullVTK    ((VTKdata){0})

int get_index(const VTKdata data, const int nline, const int n){
    return data.lines.data[data.lines.meta[nline].idx + n];
}

Point get_point(const VTKdata data, const int nline, const int n){
     return data.points.ptr[get_index(data, nline, n)];
}

Vector2 get_projected_point(const VTKdata data, const int nline, const int n){
     Point p = get_point(data, nline, n);
     return (Vector2){p.x, p.y};
}

void free_points(Points points){
    free(points.ptr);
}
 
void free_lines(Lines lines){
    free(lines.data);
    free(lines.meta);
}

void free_vtkdata(VTKdata data){
    free_points(data.points);
    free_lines(data.lines);
}

void assert_valid_lines(Lines lines){
    int total_length = 0;
    for (int i = 0; i < lines.mlen; i++){
        total_length += lines.meta[i].len;
    }
    assert(total_length == lines.dlen);
}



#define amin(a, b) ((a) < (b) ? (a) : (b))
#define amax(a, b) ((a) > (b) ? (a) : (b))

#endif
