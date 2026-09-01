#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"

#ifndef IMPORT_POINTS_H
#define IMPORT_POINTS_H



//updates the cached params in points
void update_bounds_from_point(Points* points, int index){
    Point new_point = AT_ARRAY(points->data, index);
    if (points->xmax < new_point.x) points->xmax = new_point.x;
    if (points->ymax < new_point.y) points->ymax = new_point.y;
    if (points->xmin > new_point.x) points->xmin = new_point.x;
    if (points->ymin > new_point.y) points->ymin = new_point.y;

}

void cache_line_bounds(VTKdata* data){
    for (int i = 0; i < LEN_ARRAY(data->lines.meta_data); i++){
        for (int j = 0; j < AT_ARRAY(data->lines.meta_data, i).len; j++){
            Line* line = &(AT_ARRAY(data->lines.meta_data, i));
            Point point = get_point(*data, i, j);
            if (line->xmax < point.x) line->xmax  = point.x;
            if (line->ymax < point.y) line->ymax  = point.y;
            if (line->xmin > point.x) line->xmin  = point.x;
            if (line->ymin > point.y) line->ymin  = point.y;
        }
    }
}

void cache_point_bounds(Points* points){
    points->xmax = AT_ARRAY(points->data, 0).x;
    points->xmax = AT_ARRAY(points->data, 0).x;
    points->ymax = AT_ARRAY(points->data, 0).y;
    points->ymax = AT_ARRAY(points->data, 0).y;

    for (int i = 0; i < LEN_ARRAY(points->data); i++) {
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

    int len;
    fscanf(file, "POINTS %d %*s", &len);

    points.data = RESERVE_ARRAY(Point, (size_t) len);
    if (!PTR_ARRAY(points.data)) {
        perror("malloc");
        fclose(file);
        return NullVTK;
    }

    for (int i = 0; i < len; i++) {
        Point new_point;
        fscanf(file, "%f %f %f",
               &new_point.x,
               &new_point.y,
               &new_point.z);
        PUSH_ARRAY(points.data, new_point);
    }

    cache_point_bounds(&points);

    //consume the newline
    fgets(line, sizeof(line), file);

    int meta_len, points_len;
    fscanf(file, "LINES %d %d", &meta_len, &points_len);
    printf("Metadata: %d %d", meta_len, points_len);

    points_len = points_len - meta_len;
    lines.meta_data = RESERVE_ARRAY(Line, (size_t) meta_len);
    lines.point_data = RESERVE_ARRAY(int, (size_t) points_len);
    

    for (int i = 0; i < meta_len; i++){
        Line line;
        int  ref;
        fscanf(file, "%d", &(line.len));
        line.idx = LEN_ARRAY(lines.point_data);
        for (int j = 0; j < line.len; j++){
            fscanf(file, "%d", &ref);
            PUSH_ARRAY(lines.point_data, ref);
        }
        PUSH_ARRAY(lines.meta_data, line);
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
    int* translation_buffer = (int*) malloc(LEN_ARRAY(old_data.points.data) * sizeof(int));
    VTKdata new_data = {0};
    int n = 0;
    int cur_line_idx = 0;
    int cur_data_idx = 0;


    new_data.points.data = INIT_ARRAY(Point);

    //now, we run it over once again, and populate new points along with translation buffer
    for (int i = 0; i < LEN_ARRAY(old_data.points.data); i++){
        Point p = AT_ARRAY(old_data.points.data, i);
        if (filter(p)){
            translation_buffer[i] = LEN_ARRAY(new_data.points.data);
            PUSH_ARRAY(new_data.points.data, p);
        } else {
            translation_buffer[i] = -1;
        }
    }

    cache_point_bounds(&new_data.points);
    

    new_data.lines.meta_data  = INIT_ARRAY(Line); 
    new_data.lines.point_data = INIT_ARRAY(int); 


    FOREACH_IDX(old_data.lines.meta_data, old_line, i){
        bool keep_line = true;
        for (int j = 0; j < old_line->len; j++){
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

        Line new_line;
        new_line.idx = LEN_ARRAY(new_data.lines.point_data);
        new_line.len = old_line->len;
        PUSH_ARRAY(new_data.lines.meta_data, new_line);


        for (int j = 0; j < old_line->len; j++){
            int new_data_idx = translation_buffer[get_index(old_data, i, j)];
            assert(new_data_idx >= 0);
            PUSH_ARRAY(new_data.lines.point_data, new_data_idx);
        }

    }
    
    assert_valid_lines(new_data.lines);
    free(translation_buffer);
    
    return new_data;

}

//asumes no repeated points in lines
VTKdata run_filter_with_truncation(VTKdata old_data, bool (*filter)(Point), int min_length){
    VTKdata new_data = {0};
    new_data.lines.meta_data = INIT_ARRAY(Line);
    new_data.lines.point_data = INIT_ARRAY(int);
    new_data.points.data = INIT_ARRAY(Point);

    FOREACH_IDX(old_data.lines.meta_data, old_line, old_line_idx){
        bool ongoing_line = false;
        for (int old_line_offset = 0; old_line_offset < old_line->len; old_line_offset++){
            if (ongoing_line){
                if (filter(get_point(old_data, old_line_idx, old_line_offset))){
                    PUSH_ARRAY(new_data.points.data, get_point(old_data, old_line_idx, old_line_offset));
                    PUSH_ARRAY(new_data.lines.point_data, (int) LEN_ARRAY(new_data.points.data) - 1);
                    LAST_ARRAY(new_data.lines.meta_data).len++;
                } else{
                    ongoing_line = false;
                    if (min_length < LAST_ARRAY(new_data.lines.meta_data).len){
                        int chop_len = LAST_ARRAY(new_data.lines.meta_data).len;
                        POP_ARRAY(new_data.points.data, chop_len);
                        POP_ARRAY(new_data.lines.meta_data, 1);
                        POP_ARRAY(new_data.lines.point_data, chop_len);
                    }

                }
            } else{
                if (filter(get_point(old_data, old_line_idx, old_line_offset))){
                    ongoing_line = true;
                    PUSH_ARRAY(new_data.points.data, get_point(old_data, old_line_idx, old_line_offset));
                    PUSH_ARRAY(new_data.lines.point_data, (int) LEN_ARRAY(new_data.points.data) - 1);

                    Line new_line = (Line){.idx = (int) LEN_ARRAY(new_data.points.data) - 1, .len = 1};
                    PUSH_ARRAY(new_data.lines.meta_data, new_line);
                } 
            }
        }
    }

    cache_point_bounds(&new_data.points);
    cache_line_bounds(&new_data);
    assert_valid_lines(new_data.lines);
    new_data.start_intersections = LEN_ARRAY(new_data.points.data);
    return new_data;
}

#endif
