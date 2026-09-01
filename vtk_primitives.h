#include "types.h"
#include <stdio.h>


#ifndef VTK_PRIMITIVES_H
#define VTK_PRIMITIVES_H

void print_diagnostics(VTKdata* data){
    printf("\n DIAGNOSTICS INFO \n");
    printf("TOTAL LINES: [%d] \n", LEN_ARRAY(data->lines.meta_data));
    printf("TOTAL POINTS: [%d] \n", LEN_ARRAY(data->points.data));
    printf("TOTAL VIRTUAL POINTS: [%d] \n", LEN_ARRAY(data->lines.point_data));
    printf("XMAX: [%f] \n", data->points.xmax);
    printf("YMAX: [%f] \n", data->points.ymax);
    printf("XMIN: [%f] \n", data->points.xmin);
    printf("YMIN: [%f] \n", data->points.ymin);
    fflush(stdout);
}

bool get_line_intersection(
    Vector2 p1, Vector2 p2,
    Vector2 p3, Vector2 p4,
    Vector2 *intersection
) {
    float x1 = p1.x, y1 = p1.y;
    float x2 = p2.x, y2 = p2.y;
    float x3 = p3.x, y3 = p3.y;
    float x4 = p4.x, y4 = p4.y;

    float denom =
        (x1 - x2) * (y3 - y4) -
        (y1 - y2) * (x3 - x4);

    // Parallel (or coincident)
    if (denom == 0.0f)
        return false;

    float t =
        ((x1 - x3) * (y3 - y4) -
         (y1 - y3) * (x3 - x4)) / denom;

    float u =
        -((x1 - x2) * (y1 - y3) -
          (y1 - y2) * (x1 - x3)) / denom;

    // Intersection must lie within both segments
    if (t < 0.0f || t > 1.0f ||
        u < 0.0f || u > 1.0f)
        return false;

    *intersection = (Vector2){
        x1 + t * (x2 - x1),
        y1 + t * (y2 - y1)
    };

    return true;
}

int add_new_point(VTKdata* data, Point new_point){
    PUSH_ARRAY(data->points.data, new_point);
    PUSH_ARRAY(data->adjacency_list, (adj_list_element){0});
    return LEN_ARRAY(data->points.data) - 1;
}

void link(VTKdata* data, int a, int b){
    assert(a < LEN_ARRAY(data->points.data));
    assert(b < LEN_ARRAY(data->points.data));
    //still valid after insertion
    assert(ADJ_LENGTH(data, a) < ADJ_LIST_ELEMENT_SIZE);
    assert(ADJ_LENGTH(data, b) < ADJ_LIST_ELEMENT_SIZE);

    ADJ_ELEMENT(data, a, ADJ_LENGTH(data, a)) = b;
    ADJ_ELEMENT(data, b, ADJ_LENGTH(data, b)) = a;

    ADJ_LENGTH(data, a)++;
    ADJ_LENGTH(data, b)++;
}

//neighbor_list size assumed to at least be at most ADJ_LIST_ELEMENT_SIZE
//returns number of neighbors
void populate_neighbor_data(VTKdata* data){
    //clear the neighbors list first
    //if the neigbhor list is NULL, allocate it first
    /*data->adjacency_list = RESERVE_ARRAY(int, LEN_ARRAY(data->points.data) * ADJ_LIST_ELEMENT_SIZE);*/
    data->adjacency_list = INIT_ARRAY(adj_list_element);
    for (int i = 0; i < LEN_ARRAY(data->points.data); i++){
        PUSH_ARRAY(data->adjacency_list, (adj_list_element){0});
    }

    for (int k = 0; k < LEN_ARRAY(data->lines.meta_data); k++){
        for (int j = first_point(*data, k) + 1; j < last_point(*data, k); j++){
            link(data, AT_ARRAY(data->lines.point_data, j - 1), AT_ARRAY(data->lines.point_data, j));
        }
    }
}


bool is_potential_intersection(VTKdata* data, int lineA, int lineB){
    Line a = AT_ARRAY(data->lines.meta_data, lineA);
    Line b = AT_ARRAY(data->lines.meta_data, lineB);
    if (a.xmax < b.xmax && a.ymax < b.ymax && a.xmax > b.xmin && a.ymax > b.ymin){
        return true;
    }

    if (b.xmax < a.xmax && b.ymax < a.ymax && b.xmax > a.xmin && b.ymax > a.ymin){
        return true;
    }

    return false;

}

#define INTERSECTION_POINT_Z 1000
void populate_intersections(VTKdata* data){
    Vector2 new_point;
    int new_index;
    data->start_intersections = LEN_ARRAY(data->points.data);

    fflush(stdout);
    for (int i = 0; i < LEN_ARRAY(data->lines.meta_data); i++){
        for (int j = 0; j + 1 < AT_ARRAY(data->lines.meta_data, i).len; j++){
            Vector2 a = get_projected_point(*data, i, j);
            Vector2 b = get_projected_point(*data, i, j + 1);
            for (int i2 = i + 1; i2 < LEN_ARRAY(data->lines.meta_data); i2++){
                //we skip if bounding boxes don't overlap
                if (!is_potential_intersection(data, i, i2)) continue;
                if (i == i2) continue;

                for (int j2 = 0; j2 + 1 < AT_ARRAY(data->lines.meta_data, i2).len; j2++){
                    if (i == i2 && j == j2) continue;
                    if (i == i2 && j + 1 == j2) continue;
                    if (i == i2 && j2 + 1 == j) continue;
                    Vector2 c = get_projected_point(*data, i2, j2);
                    Vector2 d = get_projected_point(*data, i2, j2 + 1);
                    if (get_line_intersection(a,b,c,d, &new_point)){
                        new_index = add_new_point(data, (Point){new_point.x, new_point.y, INTERSECTION_POINT_Z});
                        link(data, new_index, get_index(*data, i, j));
                        link(data, new_index, get_index(*data, i, j+ 1));
                        link(data, new_index, get_index(*data, i2, j2));
                        link(data, new_index, get_index(*data, i2, j2+ 1));
                    }
                }
            }
        }
    }
    
    
}

#endif
