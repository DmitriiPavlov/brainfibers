#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include <stdlib.h>
#include <math.h>

typedef float (*Float_pdf)(void);

/* ---------- helpers ---------- */

static inline float dist_u01(void) {
  float x = (float)(rand() / ((double)RAND_MAX + 1.0));
  return x < 1.0f ? x : 0.99999994f;
}

static inline float dist_gauss01(void) {
  float u1 = 1.0f - dist_u01(), u2 = dist_u01();
  return sqrtf(-2.0f * logf(u1)) * cosf(6.2831853f * u2);
}

static inline float dist_max_of(int k) {
  float m = dist_u01();
  for (int i = 1; i < k; i++) { float r = dist_u01(); if (r > m) m = r; }
  return m;
}

static inline float dist_min_of(int k) {
  float m = dist_u01();
  for (int i = 1; i < k; i++) { float r = dist_u01(); if (r < m) m = r; }
  return m;
}

static inline float dist_gauss_in_unit(float center, float sigma) {
  float x;
  do { x = center + sigma * dist_gauss01(); } while (x < 0.0f || x >= 1.0f);
  return x;
}

/* Map a float in [0,1) to an int in [lo, hi], inclusive on both ends. */
static inline int float_to_int_range(float x, int lo, int hi) {
  if (hi < lo) { int t = lo; lo = hi; hi = t; }
  int span = hi - lo + 1;
  int i = (int)(x * span);
  if (i >= span) i = span - 1;
  if (i < 0) i = 0;
  return lo + i;
}

/* Sample a pdf and map to an index in [0, n) */
static inline int pdf_index(Float_pdf pdf, int n) {
  return float_to_int_range(pdf(), 0, n - 1);
}

/* ---------- distributions ---------- */

static inline float pdf_uniform(void)     { return dist_u01(); }
static inline float pdf_low_biased(void)  { return dist_min_of(2); }
static inline float pdf_high_biased(void) { return dist_max_of(2); }
static inline float pdf_super_high(void)  { return dist_max_of(8); }
static inline float pdf_super_low(void)   { return dist_min_of(8); }
static inline float pdf_centered(void)    { return (dist_u01() + dist_u01()) * 0.5f; }
static inline float pdf_gaussian(void)    { return dist_gauss_in_unit(0.5f, 0.10f); }

static inline float pdf_bimodal(void) {
  return dist_gauss_in_unit(dist_u01() < 0.5f ? 0.25f : 0.75f, 0.06f);
}

/* ---------- random multi-peak (4-5 peaks) ---------- */

#define PEAKS_MAX   5
#define PEAK_SIGMA  0.01f

static float dist_peak_center[PEAKS_MAX];
static int   dist_peak_count = 0;

static inline void pdf_peaks_reseed(void) {
  dist_peak_count = 4 + (int)(dist_u01() * 2);
  for (int i = 0; i < dist_peak_count; i++)
    dist_peak_center[i] = 0.05f + 0.90f * dist_u01();
}

static inline float pdf_peaks(void) {
  if (dist_peak_count == 0) pdf_peaks_reseed();
  int k = (int)(dist_u01() * dist_peak_count);
  return dist_gauss_in_unit(dist_peak_center[k], PEAK_SIGMA);
}

#define PM_PROB 1.0f   /* chance a cell holds a mass; 500x500 grid -> ~250 masses */

/* Each cell is empty or holds a mass of exactly 1 */
static inline float pdf_point_mass(void) {
  return dist_u01() < PM_PROB ? 1.0f : 0.0f;
}

/* Same, but masses have random size in [0.2, 1.2) */
static inline float pdf_point_mass_weighted(void) {
  return dist_u01() < PM_PROB ? 0.2f + dist_u01() : 0.0f;
}

#endif /* DISTRIBUTION_H */
