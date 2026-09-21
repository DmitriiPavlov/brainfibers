#include "types.h"
#include "grid.h"
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
    assert(ADJ_LENGTH(data, a) + 1 < MAX_NEIGHBORS);
    assert(ADJ_LENGTH(data, b) + 1 < MAX_NEIGHBORS);

    //verify we are not double linking (unless we later want to)
    /*for (int i = 0; i < MAX_NEIGHBORS; i++){*/
    /*    assert(AT_ARRAY(data->adjacency_list, b).neighbors[i] != a);*/
    /*    assert(AT_ARRAY(data->adjacency_list, a).neighbors[i] != b);*/
    /*}*/
    bool b_exists = false;
    for (int i = 0; i < ADJ_LENGTH(data, a); i++){
        b_exists = b_exists || (ADJ_ELEMENT(data, a, i) == b);

    }

    bool a_exists = false;
    for (int i = 0; i < ADJ_LENGTH(data, b); i++){
        a_exists = a_exists || (ADJ_ELEMENT(data, b, i) == a);

    }

    assert(a_exists == b_exists);

    if (!a_exists){
        ADJ_ELEMENT(data, a, ADJ_LENGTH(data, a)) = b;
        ADJ_ELEMENT(data, b, ADJ_LENGTH(data, b)) = a;
        ADJ_LENGTH(data, a)++;
        ADJ_LENGTH(data, b)++;
    }
}

void unlink(VTKdata* data, int a, int b){
    assert(a < LEN_ARRAY(data->points.data));
    assert(b < LEN_ARRAY(data->points.data));

    for (int i = 0; i < ADJ_LENGTH(data, a); i++){
        if (ADJ_ELEMENT(data, a, i) == b){
            ADJ_ELEMENT(data, a, i) = ADJ_ELEMENT(data, a, ADJ_LENGTH(data, a) - 1);
            ADJ_ELEMENT(data, a, ADJ_LENGTH(data, a) - 1) = -1;
            ADJ_LENGTH(data, a)--;
        }
    }

    for (int i = 0; i < ADJ_LENGTH(data, b); i++){
        if (ADJ_ELEMENT(data, b, i) == a){
            ADJ_ELEMENT(data, b, i) = ADJ_ELEMENT(data, b, ADJ_LENGTH(data, b) - 1);
            ADJ_ELEMENT(data, b, ADJ_LENGTH(data, b) - 1) = -1;
            ADJ_LENGTH(data, b)--;
        }
    }
}


bool exists_shared_neighbor(VTKdata* data, int a, int b, int c, int d){
    for(int i = 0; i < ADJ_LENGTH(data, a); i++){
        for(int j = 0; j < ADJ_LENGTH(data, b); j++){
            for(int k = 0; k < ADJ_LENGTH(data, c); k++){
                for(int l = 0; l < ADJ_LENGTH(data, d); l++){
                    int w = ADJ_ELEMENT(data, a, i);
                    int x = ADJ_ELEMENT(data, b, j);
                    int y = ADJ_ELEMENT(data, c, k);
                    int z = ADJ_ELEMENT(data, d, l);
                    if (w == x && x == y && y == z){
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


void add_intersection_point(VTKdata* data, Ntuple(int, 2) l1, Ntuple(int,2) l2){
    Vector2 a = project(AT_ARRAY(data->points.data, l1.at[0]));
    Vector2 b = project(AT_ARRAY(data->points.data, l1.at[1]));
    Vector2 c = project(AT_ARRAY(data->points.data, l2.at[0]));
    Vector2 d = project(AT_ARRAY(data->points.data, l2.at[1]));
    Vector2 new_point;
    if (get_line_intersection(a,b,c,d,&new_point)){
        //before we check anything, we want to check if there already exists a point
        //that satisfies these properties i.e. a devious nested for loop
        if (exists_shared_neighbor(data, l1.at[0], l1.at[1], l2.at[0], l2.at[1])){
            return;
        }

        int new_index = add_new_point(data, (Point){new_point.x, new_point.y, 100});

        unlink(data, l1.at[0], l1.at[1]);
        unlink(data, l2.at[0], l2.at[1]);

        link(data, new_index, l1.at[0]);
        link(data, new_index, l1.at[1]);
        link(data, new_index, l2.at[0]);
        link(data, new_index, l2.at[1]);
    }
}

//neighbor_list size assumed to at least be at most MAX_NEIGHBORS
//returns number of neighbors
void populate_neighbor_data(VTKdata* data){
    //clear the neighbors list first
    //if the neigbhor list is NULL, allocate it first
    /*data->adjacency_list = RESERVE_ARRAY(int, LEN_ARRAY(data->points.data) * MAX_NEIGHBORS);*/
    data->adjacency_list = INIT_ARRAY(adj_list_element);
    for (int i = 0; i < LEN_ARRAY(data->points.data); i++){
        PUSH_ARRAY(data->adjacency_list, (adj_list_element){0});
        for (int j = 0; j < MAX_NEIGHBORS; j++){
            AT_ARRAY(data->adjacency_list, i).neighbors[j] = -1;
        }

    }


    
    FOREACH_IDX(data->lines.meta_data, line, i){
        printf("Line number %d", i);
        for (int j = LINE_START(*line) + 1; j <= LINE_END(*line); j++){
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


//this doesn't unlink at the intersections (bad)
void populate_intersections_optimized(VTKdata* data){
    typedef discreteSpace(Array(Ntuple(int,2))) collision_grid;
    collision_grid space = DISCRETE_CREATE(collision_grid, 50, 50);
    space->xmin = data->points.xmin - 0.5;
    space->ymin = data->points.ymin - 0.5;
    space->xmax = data->points.xmax + 0.5;
    space->ymax = data->points.ymax + 0.5;

    data->start_intersections = LEN_ARRAY(data->points.data);

    for (int i = 0; i < space->array.len; i++){
        AT_ARRAY(space->array, i) = INIT_ARRAY(Ntuple(int, 2));
    }

    for (int i = 0; i < LEN_ARRAY(data->lines.meta_data); i++){
        for (int j = 0; j + 1 < AT_ARRAY(data->lines.meta_data, i).len; j++){
            //pair defining which 2 points there's a line 
            Ntuple(int, 2) pair = INIT_TUPLE(int, get_index(*data, i,j),get_index(*data,i,j+1));
            //point 1 at discrete coordinates
            Ntuple(int, 2) p1 = discrete_convert(space, get_point(*data, i,j).x, get_point(*data, i,j).y);
            Ntuple(int, 2) p2 = discrete_convert(space, get_point(*data, i,j+1).x, get_point(*data, i,j+1).y);
            for (int discrete_x = amin(p1.at[0], p2.at[0]); discrete_x <= amax(p1.at[0], p2.at[0]); discrete_x++){
                for (int discrete_y = amin(p1.at[1], p2.at[1]); discrete_y <= amax(p1.at[1], p2.at[1]); discrete_y++){
                    PUSH_ARRAY(AT_ARRAY(space->array,discrete_x + discrete_y*space->xresolution), pair);
                }
            }
        }
    }

    FOREACH(space->array, collision_list){
        FOREACH_IDX(*collision_list, line1, i){
            FOREACH_IDX(*collision_list, line2, j){
                if (i < j) {
                    if (line1->at[0] == line2->at[0] || line1->at[0] == line2->at[1] ||
                        line1->at[1] == line2->at[0] || line1->at[1] == line2->at[1]) {
                        // shares an endpoint — same/adjacent segment, skip
                    } else {
                        add_intersection_point(data, *line1, *line2);
                    }
                }
            }
        }
    }

    for (int i = 0; i < space->array.len; i++){
        FREE_ARRAY(AT_ARRAY(space->array, i));
    }


    DISCRETE_DESTROY(space);
    return;
}

#endif
