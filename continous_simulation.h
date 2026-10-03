#include "types.h"
#include "grid.h"
#include "distributions.h"
#include "discrete_simulation.h"


typedef struct{
  //stores a, (b+c)/2, d in the [a b; c d] diffusivity matrix
  Array(Ntuple(double, 3)) coefficients;
  discreteSpace(float) concentration;
} continuousFrame;

void reset_approximation_frame(VTKdata data, discreteFrame* frame){
  int length = LEN_ARRAY(frame->particles);
  POP_ARRAY(frame->particles, LEN_ARRAY(frame->particles));
  for (int i = 0; i < length;){
    particle p;
    int delta = float_to_int_range(pdf_uniform(), 0, 1000);
    delta = amin(delta,length - i);
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

typedef Ntuple(Ntuple(double, 3), 3) Matrix33;
DEFINE_ARRAY(Matrix33);

static inline Matrix33 outer_product(Ntuple(double, 3) v){
  Matrix33 product = {0};
  product.at[0].at[0] = v.at[0]*v.at[0];
  product.at[0].at[1] = v.at[0]*v.at[1];
  product.at[0].at[2] = v.at[0]*v.at[2];
  product.at[1].at[0] = v.at[1]*v.at[0];
  product.at[2].at[0] = v.at[2]*v.at[0];
  product.at[1].at[1] = v.at[1]*v.at[1];
  product.at[1].at[2] = v.at[1]*v.at[2];
  product.at[2].at[1] = v.at[2]*v.at[1];
  product.at[2].at[2] = v.at[2]*v.at[2];
  return product;
}

static inline Ntuple(double, 3) add_v(Ntuple(double, 3) a, Ntuple(double, 3) b){
  Ntuple(double, 3) out = {0};
  out.at[0] = a.at[0] + b.at[0];
  out.at[1] = a.at[1] + b.at[1];
  out.at[2] = a.at[2] + b.at[2];
  return out;
}

static inline Ntuple(double, 3) scale_v(Ntuple(double, 3) v, double k){
  return INIT_TUPLE(double, k*v.at[0], k*v.at[1], k*v.at[2]);
}

static inline Matrix33 add_m3(Matrix33 a, Matrix33 b){
  Matrix33 out = {0};
  out.at[0] = add_v(a.at[0], b.at[0]);
  out.at[1] = add_v(a.at[1], b.at[1]);
  out.at[2] = add_v(a.at[2], b.at[2]);
  return out;
}

static inline double determinant_m3(Matrix33 A){
  //we interpret A[i] as the i'th column vector
  double cf1 = A.at[0].at[0]*(A.at[1].at[1]*A.at[2].at[2] - A.at[1].at[2]*A.at[2].at[1]);
  double cf2 = A.at[1].at[0]*(A.at[2].at[1]*A.at[0].at[2] - A.at[2].at[2]*A.at[0].at[1]);
  double cf3 = A.at[2].at[0]*(A.at[0].at[1]*A.at[1].at[2] - A.at[0].at[2]*A.at[1].at[1]);
  return cf1 + cf2 + cf3;
}




#define NEUMANN_GET(concentration, x, y) DISCRETE_DIRECT(concentration, INIT_TUPLE(int, CLAMP_INT(x, 0, concentration->xresolution - 1), CLAMP_INT(y, 0, concentration->yresolution - 1)))

static inline float compute_xx_partial(discreteSpace(float) concentration, int x, int y){
  //need to enforce boundary conditions with NEUMANN_GET
  return NEUMANN_GET(concentration, x - 1, y) - 2*NEUMANN_GET(concentration, x, y) + NEUMANN_GET(concentration, x + 1, y);
}

static inline float compute_yy_partial(discreteSpace(float) concentration, int x, int y){
  //need to enforce boundary conditions with NEUMANN_GET
  return NEUMANN_GET(concentration, x, y - 1) - 2*NEUMANN_GET(concentration, x, y) + NEUMANN_GET(concentration, x, y + 1);
}

static inline float compute_xy_partial(discreteSpace(float) concentration, int x, int y){
  //need to enforce boundary conditions with NEUMANN_GET
  return (NEUMANN_GET(concentration, x + 1, y + 1) - NEUMANN_GET(concentration, x + 1, y - 1) - NEUMANN_GET(concentration, x - 1, y + 1) + NEUMANN_GET(concentration, x - 1, y -1))/4;
}

void gather_derivative_data(VTKdata data, discreteFrame frame, Array(Matrix33)* ATA, Array(Ntuple(double, 3))* ATy, Array(Ntuple(double, 3))* spare_vectors, float v){
  //we will use x + frame.xresolution*y for accessing the first and second derivative array
  //the convention for the coefficients is xx, xy, yy
  POP_ARRAY((*spare_vectors), LEN_ARRAY(*spare_vectors));

  discreteSpace(float) u  = frame.concentration;
  for (int y = 0; y < u->yresolution; y++){
    for (int x = 0; x < u->xresolution; x++){
      Ntuple(double, 3) Axy = INIT_TUPLE(double, (double) compute_xx_partial(u,x,y), (double) compute_xy_partial(u,x,y), (double) compute_yy_partial(u,x,y));
      PUSH_ARRAY((*spare_vectors), Axy);
      AT_ARRAY(*ATA, x + u->xresolution*y) = add_m3(outer_product(Axy), AT_ARRAY(*ATA, x + u->xresolution*y));
      AT_ARRAY(*ATy, x + u->xresolution*y) = add_v(AT_ARRAY(*ATy, x + u->xresolution*y), scale_v(Axy, -1 * DISCRETE_DIRECT(u,INIT_TUPLE(int, x,y))));
    }
  }
  step_dframe(data, frame, v);
  frame.concentration = get_concentration_dframe(data, frame);

  for (int y = u->yresolution - 1; y >= 0; y--){
    for (int x = u->xresolution - 1; x >= 0; x--){
      //we store -1 * the concentration at the previous time step, so this computes a rough derivative approximation (good enough)
      AT_ARRAY(*ATy, x + u->xresolution*y) = add_v(AT_ARRAY(*ATy, x + u->xresolution*y), scale_v(LAST_ARRAY(*spare_vectors), DISCRETE_DIRECT(u,INIT_TUPLE(int, x,y))));
      POP_ARRAY((*spare_vectors), 1);
    }
  }

  return;
}


#define NUM_OF_SAMPLES 2000
#define NUM_SPREADING 30
#define SAMPLE_COUNTS 100
void gather_global_derivative_data(VTKdata data, int w, int h, Array(Matrix33)* ATA, Array(Ntuple(double, 3))* ATy, float v){
  //we create the frame
  discreteFrame frame = init_dframe_v2(data, pdf_uniform, w, h, w*h*10, 0, 1000);
  Array(Ntuple(double, 3)) spare_vectors = INIT_ARRAY(Ntuple(double, 3));
  //we are going to assume second_derivatives, first_derivatives are properly initialized
  for (int i = 0; i < NUM_OF_SAMPLES; i++){
    for (int j = 0; j < NUM_SPREADING; j++){
      step_dframe(data, frame, v);
    }

    frame.concentration = get_concentration_dframe(data, frame);
    for (int j = 0; j < SAMPLE_COUNTS; j++){
      gather_derivative_data(data, frame, ATA, ATy, &spare_vectors, v);
    }
    reset_approximation_frame(data, &frame);
  }

  destroy_dframe(frame);
  FREE_ARRAY(spare_vectors);

}



void compute_coefficients_locally(Matrix33 ATA, Ntuple(double, 3) ATy, Ntuple(double, 3)* coefficients){
  //note that x[i] = det(Ai)/det(A)
  // Ai is merely A where the i'th column is y
  //we can chcekc tolerance here
  double det = determinant_m3(ATA);

  if (det == 0) {
    coefficients->at[0] = 0;
    coefficients->at[1] = 0;
    coefficients->at[2] = 0;
    return;
  }

  Matrix33 temp = ATA;
  temp.at[0] = ATy;
  double det0 = determinant_m3(temp);

  temp = ATA;
  temp.at[1] = ATy;
  double det1 = determinant_m3(temp);

  temp = ATA;
  temp.at[2] = ATy;
  double det2 = determinant_m3(temp);

  coefficients->at[0] = det0/det;
  coefficients->at[1] = det1/det;
  coefficients->at[2] = det2/det;
  
  return;

}

void compute_coefficients(VTKdata data, continuousFrame* frame, float v){
  int w = frame->concentration->xresolution;
  int h = frame->concentration->yresolution;
  frame->coefficients = INIT_ARRAY(Ntuple(double, 3));
  Array(Matrix33) ATA = INIT_ARRAY(Matrix33);
  Array(Ntuple(double, 3)) ATy = INIT_ARRAY(Ntuple(double, 3));

  for (int i = 0; i < w*h; i++){
    PUSH_ARRAY(ATA, (Matrix33){0});
    PUSH_ARRAY(ATy,  (Ntuple(double, 3)){0});
  }

  gather_global_derivative_data(data, w, h, &ATA, &ATy, v); 

  for (int y = 0; y < h; y++){
    for (int x = 0; x < w; x++){
      compute_coefficients_locally(AT_ARRAY(ATA, x + y*w),
                                   AT_ARRAY(ATy, x + y*w),
                                   &AT_ARRAY(frame->coefficients, x + y*w));
    }
  }

  FREE_ARRAY(ATA);
  FREE_ARRAY(ATy);

  return;
}







