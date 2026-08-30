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
    float xmin;
    float xmax;
    float ymin;
    float ymax;
} Line;

typedef struct {
    int*  data;
    int   dlen;
    Line* meta;
    int   mlen;
} Lines;


typedef struct {
    //primitives
    
    //For a point id i, with id j in a line k
    
    //points->ptr[i] is a point id location
    Points points;

    //lines->data[lines->meta[k].idx + j] is a point's id
    Lines lines;

    //cached data
    
    //neighbor_list is size points.len*ADJACENCY_LIST_SIZE
    //we use -1 for the end of the list for a point
    //neighbor_list[i*(ADJACENCY_LIST_ELEMENT_SIZE+1) + 1] is a point's neighbors, 
    int* adjacency_list;

    int start_intersections;
    
} VTKdata;

#define ADJ_LIST_ELEMENT_SIZE 32
//gives the adjacency list index for element x at position n
#define ADJ_OFFSET(x, n) ((x) * ADJ_LIST_ELEMENT_SIZE + (n))

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
    if (data.adjacency_list != NULL){
        free(data.adjacency_list);
    }
}

int first_point(VTKdata data, int k){
    assert(k >= 0 && k < data.lines.mlen);
    return data.lines.meta[k].idx;
}

int last_point(VTKdata data, int k){
    assert(k >= 0 && k < data.lines.mlen);
    return data.lines.meta[k].idx + data.lines.meta[k].len - 1;
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
