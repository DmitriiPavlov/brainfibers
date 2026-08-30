#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"

#ifndef IMPORT_POINTS_H
#define IMPORT_POINTS_H



//updates the cached params in points
void update_bounds_from_point(Points* points, int index){
    Point* ptr = points->ptr;
    Point new_point = ptr[index];
    if (points->xmax < new_point.x) points->xmax = new_point.x;
    if (points->ymax < new_point.y) points->ymax = new_point.y;
    if (points->xmin > new_point.x) points->xmin = new_point.x;
    if (points->ymin > new_point.y) points->ymin = new_point.y;

}

void cache_line_bounds(VTKdata* data){
    for (int i = 0; i < data->lines.mlen; i++){
        for (int j = 0; j < data->lines.meta[i].len; j++){
            Line* line = &(data->lines.meta[i]);
            Point point = get_point(*data, i, j);
            if (line->xmax < point.x) line->xmax  = point.x;
            if (line->ymax < point.y) line->ymax  = point.y;
            if (line->xmin > point.x) line->xmin  = point.x;
            if (line->ymin > point.y) line->ymin  = point.y;
        }
    }
}

void cache_point_bounds(Points* points){
    points->xmax = points->ptr[0].x;
    points->xmin = points->ptr[0].x;
    points->ymax = points->ptr[0].y;
    points->ymin = points->ptr[0].y;

    for (int i = 0; i < points->len; i++) {
        update_bounds_from_point(points, i);
    }
}

VTKdata import_vtk(const char* filename){
    FILE  *file;
    Points points;
    Lines  lines  = {0};
    int    offset = 0;

    file = fopen(filename, "r");
    if (!file) {
        perror("tracks.vtk");
        return NullVTK;
    }

    char line[256];

    for (int i = 0; i < 4; i++)
        fgets(line, sizeof(line), file);

    fscanf(file, "POINTS %d %*s", &points.len);

    points.ptr = (Point*) malloc(points.len * sizeof(Point));
    if (!points.ptr) {
        perror("malloc");
        fclose(file);
        return NullVTK;
    }

    for (int i = 0; i < points.len; i++) {
        fscanf(file, "%f %f %f",
               &points.ptr[i].x,
               &points.ptr[i].y,
               &points.ptr[i].z);
    }

    cache_point_bounds(&points);

    //consume the newline
    fgets(line, sizeof(line), file);

    fscanf(file, "LINES %d %d", &lines.mlen, &lines.dlen);
    printf("Metadata: %d %d", lines.mlen, lines.dlen);

    lines.dlen = lines.dlen - lines.mlen;
    lines.meta = (Line*) malloc(lines.mlen * sizeof(Line));
    lines.data = (int*) malloc(lines.dlen * sizeof(int));
    

    for (int i = 0; i < lines.mlen; i++){
        fscanf(file, "%d", &(lines.meta[i].len));
        lines.meta[i].idx = offset;
        for (int j = 0; j < lines.meta[i].len; j++){
            fscanf(file, "%d", &lines.data[offset]);
            offset++;
        }
    }

    assert_valid_lines(lines);

    fclose(file);

    printf("%f %f %f %f", points.xmin, points.xmax, points.ymin, points.ymax);
    return (VTKdata){points, lines, NULL};
}


float ZFILTER_MIN = -10.0;
float ZFILTER_MAX = 10.0;
bool zfilter(Point p){
    return (p.z < ZFILTER_MAX && p.z > ZFILTER_MIN);
}

bool nofilter(Point p){
    return true;
}

//filter returns true for keeping

VTKdata run_filter(VTKdata old_data, bool (*filter)(Point)){
    int* translation_buffer = (int*) malloc(old_data.points.len * sizeof(int));
    VTKdata new_data = {0};
    int n = 0;
    int cur_line_idx = 0;
    int cur_data_idx = 0;

    //we need to first run the filter on every point,
    //count the hits
    for (int i = 0; i < old_data.points.len; i++){
        if (filter(old_data.points.ptr[i])){
            new_data.points.len++;
        }
    }

    new_data.points.ptr = (Point*) malloc(new_data.points.len * sizeof(Point));

    //now, we run it over once again, and populate new points along with translation buffer
    for (int i = 0; i < old_data.points.len; i++){
        if (filter(old_data.points.ptr[i])){
            new_data.points.ptr[n] = old_data.points.ptr[i];
            translation_buffer[i] = n;
            n++;
        } else {
            translation_buffer[i] = -1;
        }
    }
    assert(n == new_data.points.len);

    cache_point_bounds(&new_data.points);
    
    //now the same thing, for the lines
    //we count all the lines we wish to keep
    for (int i = 0; i < old_data.lines.mlen; i++){
        bool keep_line = true;
        for (int j = 0; j < old_data.lines.meta[i].len; j++){
            keep_line &= filter(get_point(old_data, i, j));
        }

        if (keep_line) {
            new_data.lines.mlen++;
            new_data.lines.dlen += old_data.lines.meta[i].len;
        }
    }

    new_data.lines.meta = (Line*)malloc(new_data.lines.mlen * sizeof(Line)); 
    new_data.lines.data = (int* )malloc(new_data.lines.dlen * sizeof(int)); 


    for (int i = 0; i < old_data.lines.mlen; i++){
        bool keep_line = true;
        for (int j = 0; j < old_data.lines.meta[i].len; j++){
            keep_line &= filter(get_point(old_data, i, j));
            if (!filter(get_point(old_data, i, j))){
                assert(translation_buffer[get_index(old_data, i, j)] == -1);
            } else{
                assert(translation_buffer[get_index(old_data, i ,j)] >= 0);
            }
        }

        if (!keep_line) {
            continue;
        }

        new_data.lines.meta[cur_line_idx].len = old_data.lines.meta[i].len; 
        new_data.lines.meta[cur_line_idx].idx = cur_data_idx;

        for (int j = 0; j < old_data.lines.meta[i].len; j++){
            int new_data_idx = translation_buffer[get_index(old_data, i, j)];
            assert(new_data_idx >= 0);
            new_data.lines.data[cur_data_idx] = new_data_idx;
            cur_data_idx++;
        }
        cur_line_idx++;

    }

    assert(cur_line_idx == new_data.lines.mlen);
    assert(cur_data_idx == new_data.lines.dlen);
    
    assert_valid_lines(new_data.lines);
    free(translation_buffer);
    
    return new_data;

}

VTKdata run_filter_with_truncation(VTKdata old_data, bool (*filter)(Point), int min_length){
    VTKdata new_data = {0};
    for (int old_line_idx = 0; old_line_idx < old_data.lines.mlen; old_line_idx++){
        bool ongoing_line = false;
        for (int old_line_offset = 0; old_line_offset < old_data.lines.meta[old_line_idx].len; old_line_offset++){
            if (ongoing_line){
                if (filter(get_point(old_data, old_line_idx, old_line_offset))){
                    new_data.points.len++;
                    new_data.points.ptr = (Point*)realloc(new_data.points.ptr, new_data.points.len * sizeof(Point));
                    new_data.points.ptr[new_data.points.len - 1] = get_point(old_data, old_line_idx, old_line_offset);

                    new_data.lines.dlen++;
                    new_data.lines.data = (int*)realloc(new_data.lines.data, new_data.lines.dlen * sizeof(int));
                    new_data.lines.data[new_data.lines.dlen - 1] = new_data.points.len -1;
                    new_data.lines.meta[new_data.lines.mlen - 1].len++;
                } else{
                    ongoing_line = false;
                    if (min_length < new_data.lines.meta[new_data.lines.mlen - 1].len){
                        int chop_len = new_data.lines.meta[new_data.lines.mlen - 1].len;
                        new_data.points.len -= chop_len;
                        new_data.points.ptr = (Point*)realloc(new_data.points.ptr, new_data.points.len * sizeof(Point));
                        new_data.lines.mlen -= 1;
                        new_data.lines.dlen -= chop_len;
                        new_data.lines.data = (int*)realloc(new_data.lines.data, new_data.lines.dlen * sizeof(int));
                    }

                }
            } else{
                if (filter(get_point(old_data, old_line_idx, old_line_offset))){
                    ongoing_line = true;
                    new_data.points.len++;
                    new_data.points.ptr = (Point*)realloc(new_data.points.ptr, new_data.points.len * sizeof(Point));
                    new_data.points.ptr[new_data.points.len - 1] = get_point(old_data, old_line_idx, old_line_offset);

                    new_data.lines.dlen++;
                    new_data.lines.data = (int*)realloc(new_data.lines.data, new_data.lines.dlen * sizeof(int));
                    new_data.lines.data[new_data.lines.dlen - 1] = new_data.points.len - 1;
                    new_data.lines.mlen++;
                    new_data.lines.meta = (Line*)realloc(new_data.lines.meta, new_data.lines.mlen * sizeof(Line));
                    new_data.lines.meta[new_data.lines.mlen - 1].idx = new_data.points.len - 1;
                    new_data.lines.meta[new_data.lines.mlen - 1].len = 1;
                } 
            }
        }
    }

    cache_point_bounds(&new_data.points);
    cache_line_bounds(&new_data);
    assert_valid_lines(new_data.lines);
    new_data.start_intersections = new_data.points.len;
    return new_data;
}

#endif
