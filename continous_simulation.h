#include "types.h"
#include "grid.h"
#include "distributions.h"
#include "discrete_simulation.h"


typedef struct{
  //stores a, (b+c)/2, d in the [a b; c d] diffusivity matrix
  Array(Ntuple(float, 3)) coefficients;
  discreteSpace(float) concentration;
} continousFrame;

void reset_approximation_frame(VTKdata data, discreteFrame* frame){
  POP_ARRAY(frame->particles, LEN_ARRAY(frame->particles));
  for (int i = 0; i < LEN_ARRAY(frame->particles);){
    particle p;
    int delta = float_to_int_range(pdf_uniform(), 0, 1000);
    delta = amin(delta, LEN_ARRAY(frame->particles) - i);
    i += delta;
    
    int start = rand() % LEN_ARRAY(data.points.data);
    for (int j = 0; j < delta; j++){
      p.a = start % LEN_ARRAY(data.points.data);
      p.b = ADJ_ELEMENT(&data, p.a, rand() % ADJ_LENGTH(&data, p.a));
      p.t = 0;
      PUSH_ARRAY(frame->particles, p);
    }
  }
}

#define NEUMANN_GET(concentration, x, y) DISCRETE_DIRECT(concentration, INIT_TUPLE(int, CLAMP_INT(x, 0, concentration->xresolution - 1), CLAMP_INT(y, 0, concentration->yresolution - 1)))

inline float compute_xx_partial(discreteSpace(float) concentration, int x, int y){
  //need to enforce boundary conditions with NEUMANN_GET
  return NEUMANN_GET(concentration, x - 1, y) - 2*NEUMANN_GET(concentration, x, y) + NEUMANN_GET(concentration, x + 1, y);
}

inline float compute_yy_partial(discreteSpace(float) concentration, int x, int y){
  //need to enforce boundary conditions with NEUMANN_GET
  return NEUMANN_GET(concentration, x, y - 1) - 2*NEUMANN_GET(concentration, x, y) + NEUMANN_GET(concentration, x, y + 1);
}

inline float compute_xy_partial(discreteSpace(float) concentration, int x, int y){
  //need to enforce boundary conditions with NEUMANN_GET
  return NEUMANN_GET(concentration, x + 1, y + 1) - NEUMANN_GET(concentration, x + 1, y - 1) - NEUMANN_GET(concentration, x - 1, y + 1) + NEUMANN_GET(concentration, x - 1, y -1);
}

void gather_derivative_data(VTKdata data, discreteFrame frame, Array(Array(Ntuple(float, 3))) second_derivatives, Array(Array(float)) first_derivatives, float v){
  //we will use x + frame.xresolution*y for accessing the first and second derivative array
  //the convention for the coefficients is xx, xy, yy
  discreteSpace(float) u  = frame.concentration;
  for (int x = 0; x < u->xresolution; x++){
    for (int y = 0; y < u->yresolution; y++){
      PUSH_ARRAY(AT_ARRAY(first_derivatives,  x + u->xresolution*y), -1 * DISCRETE_DIRECT(u, INIT_TUPLE(int, x, y)));
      PUSH_ARRAY(AT_ARRAY(second_derivatives, x + u->xresolution*y),
                 INIT_TUPLE(float, compute_xx_partial(u, x, y), compute_xy_partial(u,x,y), compute_yy_partial(u,x,y)));
    }
  }
  step_dframe(data, frame, v);

  for (int x = 0; x < u->xresolution; x++){
    for (int y = 0; y < u->yresolution; y++){
      //we store -1 * the concentration at the previous time step, so this computes a rough derivative approximation (good enough)
      LAST_ARRAY(AT_ARRAY(first_derivatives,  x + u->xresolution*y)) += DISCRETE_DIRECT(u, INIT_TUPLE(int, x, y));
    }
  }

  return;
}

void gather_global_derivative_data(VTKdata data, Array(Array(Ntuple(float, 3))) second_derivatives, Array(Array(float)) first_derivatives, float v){
  discreteFrame frame = init_dframe_v2(data, pdf_uniform, 500, 500, 50000, 0, 1000);

}

void compute_coeffcients(VTKdata data, continousFrame* frame){
  return;
}







