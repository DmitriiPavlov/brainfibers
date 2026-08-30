#include "bg_renderer.h"
#include "raylib.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef DISCRETE_SIM_H
#define DISCRETE_SIM_H

typedef struct {
  // count[i] is the amount of particles at point i
  int *counts;
  int *current_counts;
  int *past_counts;
  int len; // should = number of points
} DiscreteFrame;

DiscreteFrame init_frame(VTKdata data) {
  DiscreteFrame frame = {0};
  assert(data.points.len > 0);
  frame.len = data.points.len;
  frame.current_counts = (int *)malloc(frame.len * sizeof(int));
  frame.past_counts = (int *)malloc(frame.len * sizeof(int));
  frame.counts = frame.current_counts;
  return frame;
}

void populate_frame(DiscreteFrame frame, float p, int n)
{
  for (int i = 0; i < frame.len; i++){
    if((rand() % (int)(1/p) == 0)){
      frame.counts[i] = n;
    }
    else{
      frame.counts[i] = 0;
    }
  }
}

void destroy_frame(DiscreteFrame frame) {
  free(frame.current_counts);
  free(frame.past_counts);
}

void assert_conservation(DiscreteFrame* frame){
  int curr_total = 0;
  int prev_total = 0;
  for (int i = 0; i < frame->len; i++){
    curr_total += frame->current_counts[i];
    prev_total += frame->past_counts[i];
  }
  assert(curr_total == prev_total);
}

void step_once(VTKdata data, DiscreteFrame* frame) {
  //overwrite previous counts
  frame->current_counts = frame->past_counts;
  frame->past_counts = frame->counts;

  fflush(stdout);
  memset(frame->current_counts, 0, frame->len * sizeof(int));
  // point i
  for (int i = 0; i < frame->len; i++) {
    int n = 0;
    for (int k = 0; k <= ADJ_LIST_ELEMENT_SIZE; k++) {
      if (data.adjacency_list[ADJ_OFFSET(i,k)] == -1) break;
      n++;
    }
    if(n == 0){
      frame->current_counts[i] = frame->past_counts[i];
      continue;
    }
    // particle j
    for (int j = 0; j < frame->past_counts[i]; j++) {
      //inner loop unnecessary

      int neighbor = data.adjacency_list[ADJ_OFFSET(i, rand()%n)];
      frame->current_counts[neighbor]++;
    }
  }
  frame->counts = frame->current_counts;
  assert_conservation(frame);
}

void render_discrete_frame(VTKdata data, DiscreteFrame frame) {
  Camera2D camera = {0};

  camera.target = (Vector2){(data.points.xmin + data.points.xmax) / 2.0f,
                            (data.points.ymin + data.points.ymax) / 2.0f};

  camera.offset = (Vector2){VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f};

  camera.zoom = amin(VIRTUAL_WIDTH / (data.points.xmax - data.points.xmin),
                     VIRTUAL_HEIGHT / (data.points.ymax - data.points.ymin));

  BeginMode2D(camera);
  for (int i = 0; i < frame.len; i++) {
    if (frame.counts[i] != 0){
      DrawCircle(data.points.ptr[i].x, data.points.ptr[i].y,
                 sqrtf((float)frame.counts[i])/camera.zoom, RED);
    }
  }

  EndMode2D();
}

#endif
