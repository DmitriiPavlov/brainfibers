#include "bg_renderer.h"
#include "grid.h"
#include "distributions.h"
#include <float.h>
#include "raylib.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef DISCRETE_SIM_H
#define DISCRETE_SIM_H

typedef struct{
  int a;
  int b;
  float t;
} particle;

DEFINE_ARRAY(particle);

typedef struct {
    Array(particle) particles;
    discreteSpace(float) concentration;
} discreteFrame;

discreteFrame init_dframe(VTKdata data, int particle_count){
  discreteFrame frame;
  frame.particles = INIT_ARRAY(particle);
  frame.concentration = DISCRETE_CREATE(discreteSpace(float), 500, 500);

  int start_nodes[10] = {0};
  for (int i = 0; i < 10; i++){
    start_nodes[i] = rand()%data.points.data.len;
    assert(ADJ_LENGTH(&data, start_nodes[i]) > 0);
  }
  
  for (int i = 0; i < particle_count; i++){
    particle p;
    p.a = start_nodes[rand()%10];
    p.b = ADJ_ELEMENT(&data, p.a, rand() % ADJ_LENGTH(&data, p.a));
    p.t = 0;
    PUSH_ARRAY(frame.particles, p);
  }
  return frame;
}

discreteFrame init_dframe_v2(VTKdata data, Float_pdf distribution, int xres, int yres, int particle_count, int low, int hi){
  discreteFrame frame;
  frame.particles = INIT_ARRAY(particle);
  frame.concentration = DISCRETE_CREATE(discreteSpace(float), xres, yres);

  for (int i = 0; i < particle_count; i++){
    particle p;
    int delta = float_to_int_range(distribution(), low, hi);
    i += delta;
    int start = rand() % LEN_ARRAY(data.points.data);
    for (int j = 0; j < delta; j++){
      p.a = start % LEN_ARRAY(data.points.data);
      p.b = ADJ_ELEMENT(&data, p.a, rand() % ADJ_LENGTH(&data, p.a));
      p.t = 0;
      PUSH_ARRAY(frame.particles, p);
    }
  }

  return frame;
}

float distance(VTKdata data, int p1, int p2){
  Vector2 a = project(AT_ARRAY(data.points.data, p1));
  Vector2 b = project(AT_ARRAY(data.points.data, p2));
  return sqrtf((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}

void step_particle(VTKdata data, particle* p, float v){
  while(v > 0){
    float dx = distance(data, p->a, p->b)*(1-p->t);
    if (v-dx >= 0){
      v = v - dx;
      p->a = p->b;
      p->b = ADJ_ELEMENT(&data, p->a, rand() % ADJ_LENGTH(&data, p->a));
      p->t = 0;
    } else{
      p->t  = 1 - (dx-v)/distance(data, p->a, p->b);
      v = 0;
    }
  }
}

void step_dframe(VTKdata data, discreteFrame frame, float v){
  FOREACH(frame.particles, p){
    step_particle(data, p, v);
  }
}


void destroy_dframe(discreteFrame frame) {
  FREE_ARRAY(frame.particles);
  DISCRETE_DESTROY(frame.concentration);
}


discreteSpace(float) get_concentration_dframe(VTKdata data, discreteFrame frame){
  discreteSpace(float) space = frame.concentration;
  space->xmin = data.points.xmin;
  space->ymin = data.points.ymin;
  space->xmax = data.points.xmax;
  space->ymax = data.points.ymax;
  for (int i = 0; i < space->array.len; i++){
    AT_ARRAY(space->array, i) = 0;
  }

  FOREACH(frame.particles, p){
      float x1 = AT_ARRAY(data.points.data,p->a).x;
      float y1 = AT_ARRAY(data.points.data,p->a).y;
      float x2 = AT_ARRAY(data.points.data,p->b).x;
      float y2 = AT_ARRAY(data.points.data,p->b).y;
      float x = x1 + (x2-x1)*p->t;
      float y = y1 + (y2-y1)*p->t;
      discrete_set(space, x,y, discrete_get(space,x, y) + 1);
  }
  return space;
}

void render_concentration(VTKdata data, discreteSpace(float) concentration) {
    float gamma = 0.2;
    Camera2D camera = {0};
    camera.target = (Vector2){(data.points.xmin + data.points.xmax) / 2.0f,
                               (data.points.ymin + data.points.ymax) / 2.0f};
    camera.offset = (Vector2){VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f};
    camera.zoom = amin(VIRTUAL_WIDTH / (data.points.xmax - data.points.xmin),
                        VIRTUAL_HEIGHT / (data.points.ymax - data.points.ymin));

    int xres = concentration->xresolution;
    int yres = concentration->yresolution;

    float cellW = (concentration->xmax - concentration->xmin) / (float)xres;
    float cellH = (concentration->ymax - concentration->ymin) / (float)yres;

    float cmin =  FLT_MAX;
    float cmax = -FLT_MAX;
    for (int x = 0; x < xres; x++) {
      for (int y = 0; y < yres; y++) {
        float v = AT_ARRAY(concentration->array, x + xres * y);
        if (v < cmin) cmin = v;
        if (v > cmax) cmax = v;
      }
    }

    float logmin = log1pf(cmin > 0.0f ? cmin : 0.0f);
    float logmax = log1pf(cmax > 0.0f ? cmax : 0.0f);
    float logrange = (logmax - logmin) > 1e-6f ? (logmax - logmin) : 1.0f;

    BeginMode2D(camera);
    for (int x = 0; x < xres; x++) {
      for (int y = 0; y < yres; y++) {
        float v = AT_ARRAY(concentration->array, x + xres * y);
        float t = (log1pf(v) - logmin) / logrange;
        t = CLAMP_FLOAT(t, 0.0f, 1.0f);

        t = powf(t, gamma); // <-- the adjustable knob

        if (t <= 0.0f) continue;

        Color col = ColorFromNormalized((Vector4){ t, 0.0f, 1.0f - t, 1.0f });
        col = Fade(col, t);

        float worldX = concentration->xmin + x * cellW;
        float worldY = concentration->ymin + y * cellH;

        DrawRectangleV((Vector2){worldX, worldY}, (Vector2){cellW, cellH}, col);
      }
    }
    EndMode2D();
}



void render_dframe(VTKdata data, discreteFrame frame){
  discreteSpace(float) concentration = get_concentration_dframe(data, frame);
  render_concentration(data, concentration);
}

#endif
