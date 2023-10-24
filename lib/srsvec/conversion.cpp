/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "srsran/srsvec/conversion.h"

#include "simd.h"

using namespace srsran;
using namespace srsvec;

namespace details {

#if SRSRAN_SIMD_F_SIZE && SRSRAN_SIMD_S_SIZE
using simd_conv_func_type = simd_s_t (*)(simd_f_t a, simd_f_t b);

template <bool R = false>
struct simd_conversion_helper {
  simd_conv_func_type convert = srsran_simd_convert_2f_s;
};

template <>
struct simd_conversion_helper<true> {
  simd_conv_func_type convert = srsran_simd_convert_2f_s_round;
};
#endif

using gen_conv_func_type = int16_t (*)(float);

template <bool R = false>
struct gen_conversion_helper {
  gen_conv_func_type convert = [](float a) { return (int16_t)a; };
};

template <>
struct gen_conversion_helper<true> {
  gen_conv_func_type convert = [](float a) { return static_cast<int16_t>(std::round(a)); };
};

} // namespace details

static inline void convert_fb_simd(const float* x, int8_t* z, float scale, unsigned len)
{
  unsigned i = 0;

  // Force the use of SSE here instead of AVX since the implementations requires too many permutes across 128-bit
  // boundaries

#ifdef HAVE_SSE
  __m128 s = _mm_set1_ps(scale);
  if (SIMD_IS_SSE_ALIGNED(x) && SIMD_IS_SSE_ALIGNED(z)) {
    for (; i + 16 <= len; i += 16) {
      __m128 a = _mm_load_ps(&x[i]);
      __m128 b = _mm_load_ps(&x[i + 1 * 4]);
      __m128 c = _mm_load_ps(&x[i + 2 * 4]);
      __m128 d = _mm_load_ps(&x[i + 3 * 4]);

      __m128 sa = _mm_mul_ps(a, s);
      __m128 sb = _mm_mul_ps(b, s);
      __m128 sc = _mm_mul_ps(c, s);
      __m128 sd = _mm_mul_ps(d, s);

      __m128i ai = _mm_cvttps_epi32(sa);
      __m128i bi = _mm_cvttps_epi32(sb);
      __m128i ci = _mm_cvttps_epi32(sc);
      __m128i di = _mm_cvttps_epi32(sd);
      __m128i ab = _mm_packs_epi32(ai, bi);
      __m128i cd = _mm_packs_epi32(ci, di);

      __m128i i8 = _mm_packs_epi16(ab, cd);

      _mm_store_si128((__m128i*)&z[i], i8);
    }
  } else {
    for (; i + 16 <= len; i += 16) {
      __m128 a = _mm_loadu_ps(&x[i]);
      __m128 b = _mm_loadu_ps(&x[i + 1 * 4]);
      __m128 c = _mm_loadu_ps(&x[i + 2 * 4]);
      __m128 d = _mm_loadu_ps(&x[i + 3 * 4]);

      __m128 sa = _mm_mul_ps(a, s);
      __m128 sb = _mm_mul_ps(b, s);
      __m128 sc = _mm_mul_ps(c, s);
      __m128 sd = _mm_mul_ps(d, s);

      __m128i ai = _mm_cvttps_epi32(sa);
      __m128i bi = _mm_cvttps_epi32(sb);
      __m128i ci = _mm_cvttps_epi32(sc);
      __m128i di = _mm_cvttps_epi32(sd);
      __m128i ab = _mm_packs_epi32(ai, bi);
      __m128i cd = _mm_packs_epi32(ci, di);

      __m128i i8 = _mm_packs_epi16(ab, cd);

      _mm_storeu_si128((__m128i*)&z[i], i8);
    }
  }
#endif /* HAVE_SSE */

  for (; i < len; i++) {
    z[i] = (int8_t)(x[i] * scale);
  }
}

static inline void convert_fb_simd(const float* x0, const float* x1, int8_t* z, float scale, unsigned len)
{
  len /= 2;

  unsigned i = 0;

#ifdef HAVE_SSE
  __m128 s = _mm_set1_ps(scale);
  if (SIMD_IS_SSE_ALIGNED(x0) && SIMD_IS_SSE_ALIGNED(x1) && SIMD_IS_SSE_ALIGNED(z)) {
    for (; i + 8 <= len; i += 8) {
      __m128 a1 = _mm_load_ps(&x0[i]);
      __m128 b1 = _mm_load_ps(&x1[i]);
      __m128 a2 = _mm_load_ps(&x0[i + 4]);
      __m128 b2 = _mm_load_ps(&x1[i + 4]);

      a1 = _mm_mul_ps(a1, s);
      b1 = _mm_mul_ps(b1, s);
      a2 = _mm_mul_ps(a2, s);
      b2 = _mm_mul_ps(b2, s);

      __m128i a1i = _mm_cvttps_epi32(a1);
      __m128i b1i = _mm_cvttps_epi32(b1);
      __m128i a2i = _mm_cvttps_epi32(a2);
      __m128i b2i = _mm_cvttps_epi32(b2);

      __m128i ai16 = _mm_packs_epi32(a1i, a2i);
      __m128i bi16 = _mm_packs_epi32(b1i, b2i);

      __m128i ci16 = _mm_unpacklo_epi32(ai16, bi16);
      __m128i di16 = _mm_unpackhi_epi32(ai16, bi16);

      __m128i i8 = _mm_packs_epi16(ci16, di16);

      _mm_store_si128((__m128i*)&z[2 * i], i8);
    }
  } else {
    for (; i + 8 <= len; i += 8) {
      __m128 a1 = _mm_loadu_ps(&x0[i]);
      __m128 b1 = _mm_loadu_ps(&x1[i]);
      __m128 a2 = _mm_loadu_ps(&x0[i + 4]);
      __m128 b2 = _mm_loadu_ps(&x1[i + 4]);

      a1 = _mm_mul_ps(a1, s);
      b1 = _mm_mul_ps(b1, s);
      a2 = _mm_mul_ps(a2, s);
      b2 = _mm_mul_ps(b2, s);

      __m128i a1i = _mm_cvttps_epi32(a1);
      __m128i b1i = _mm_cvttps_epi32(b1);
      __m128i a2i = _mm_cvttps_epi32(a2);
      __m128i b2i = _mm_cvttps_epi32(b2);

      __m128i ai16 = _mm_packs_epi32(a1i, a2i);
      __m128i bi16 = _mm_packs_epi32(b1i, b2i);

      __m128i ci16 = _mm_unpacklo_epi32(ai16, bi16);
      __m128i di16 = _mm_unpackhi_epi32(ai16, bi16);

      __m128i i8 = _mm_packs_epi16(ci16, di16);

      _mm_storeu_si128((__m128i*)&z[2 * i], i8);
    }
  }
#endif /* HAVE_SSE */

  for (; i < len; i += 2) {
    z[2 * i + 0] = (int8_t)(x0[i + 0] * scale);
    z[2 * i + 1] = (int8_t)(x0[i + 1] * scale);
    z[2 * i + 2] = (int8_t)(x1[i + 0] * scale);
    z[2 * i + 3] = (int8_t)(x1[i + 1] * scale);
  }
}

static inline void convert_bf_simd(const int8_t* x, float* z, const float scale, unsigned len)
{
  unsigned    i    = 0;
  const float gain = 1.0f / scale;

#ifdef HAVE_SSE
  __m128 s = _mm_set1_ps(gain);
  if (SIMD_IS_SSE_ALIGNED(z)) {
    for (; i + 8 <= len; i += 8) {
      __m64 a8   = *(__m64*)&x[i];
      __m64 sign = _mm_cmpgt_pi8(_mm_setzero_si64(), a8);

      __m64 v0i16 = _mm_unpacklo_pi8(a8, sign);
      __m64 v1i16 = _mm_unpackhi_pi8(a8, sign);

      __m128 v0 = _mm_cvtpi16_ps(v0i16);
      __m128 v1 = _mm_cvtpi16_ps(v1i16);

      _mm_store_ps(&z[i], _mm_mul_ps(v0, s));
      _mm_store_ps(&z[i + 4], _mm_mul_ps(v1, s));
    }
  } else {
    for (; i + 8 <= len; i += 8) {
      __m64 a8   = *(__m64*)&x[i];
      __m64 sign = _mm_cmpgt_pi8(_mm_setzero_si64(), a8);

      __m64 v0i16 = _mm_unpacklo_pi8(a8, sign);
      __m64 v1i16 = _mm_unpackhi_pi8(a8, sign);

      __m128 v0 = _mm_cvtpi16_ps(v0i16);
      __m128 v1 = _mm_cvtpi16_ps(v1i16);

      _mm_storeu_ps(&z[i], _mm_mul_ps(v0, s));
      _mm_storeu_ps(&z[i + 4], _mm_mul_ps(v1, s));
    }
  }
#endif /* HAVE_SSE */

  for (; i < len; i++) {
    z[i] = (float)x[i] * gain;
  }
}

static inline void convert_bf_simd(const int8_t* x, float* z0, float* z1, const float scale, unsigned len)
{
  len /= 2;

  unsigned    i    = 0;
  const float gain = 1.0f / scale;

#ifdef HAVE_SSE
  __m128 s = _mm_set1_ps(gain);
  if (SIMD_IS_SSE_ALIGNED(z0) && SIMD_IS_SSE_ALIGNED(z1)) {
    for (; i + 4 <= len; i += 4) {
      __m64 a8   = *(__m64*)&x[2 * i];
      __m64 sign = _mm_cmpgt_pi8(_mm_setzero_si64(), a8);

      __m64 x0i16 = _mm_unpacklo_pi8(a8, sign);
      __m64 x1i16 = _mm_unpackhi_pi8(a8, sign);

      __m128 x0 = _mm_cvtpi16_ps(x0i16);
      __m128 x1 = _mm_cvtpi16_ps(x1i16);

      __m128 v0 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(1, 0, 1, 0));
      __m128 v1 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(3, 2, 3, 2));

      _mm_store_ps(&z0[i], _mm_mul_ps(v0, s));
      _mm_store_ps(&z1[i], _mm_mul_ps(v1, s));
    }
  } else {
    for (; i + 4 <= len; i += 4) {
      __m64 a8   = *(__m64*)&x[2 * i];
      __m64 sign = _mm_cmpgt_pi8(_mm_setzero_si64(), a8);

      __m64 x0i16 = _mm_unpacklo_pi8(a8, sign);
      __m64 x1i16 = _mm_unpackhi_pi8(a8, sign);

      __m128 x0 = _mm_cvtpi16_ps(x0i16);
      __m128 x1 = _mm_cvtpi16_ps(x1i16);

      __m128 v0 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(1, 0, 1, 0));
      __m128 v1 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(3, 2, 3, 2));

      _mm_storeu_ps(&z0[i], _mm_mul_ps(v0, s));
      _mm_storeu_ps(&z1[i], _mm_mul_ps(v1, s));
    }
  }
#endif /* HAVE_SSE */

  for (; i < len; i += 2) {
    z0[i + 0] = (float)x[2 * i + 0] * gain;
    z0[i + 1] = (float)x[2 * i + 1] * gain;
    z1[i + 0] = (float)x[2 * i + 2] * gain;
    z1[i + 1] = (float)x[2 * i + 3] * gain;
  }
}

template <bool ROUND = false>
static inline void convert_fi_simd(const float* x, int16_t* z, float scale, unsigned len)
{
  unsigned i = 0;

#if SRSRAN_SIMD_F_SIZE && SRSRAN_SIMD_S_SIZE
  details::simd_conversion_helper<ROUND> conversion_helper;

  simd_f_t s = srsran_simd_f_set1(scale);
  if (SIMD_IS_ALIGNED(x) && SIMD_IS_ALIGNED(z)) {
    for (; i + SRSRAN_SIMD_S_SIZE < len + 1; i += SRSRAN_SIMD_S_SIZE) {
      simd_f_t a = srsran_simd_f_load(&x[i]);
      simd_f_t b = srsran_simd_f_load(&x[i + SRSRAN_SIMD_F_SIZE]);

      simd_f_t sa = srsran_simd_f_mul(a, s);
      simd_f_t sb = srsran_simd_f_mul(b, s);

      simd_s_t i16 = conversion_helper.convert(sa, sb);

      srsran_simd_s_store(&z[i], i16);
    }
  } else {
    for (; i + SRSRAN_SIMD_S_SIZE < len + 1; i += SRSRAN_SIMD_S_SIZE) {
      simd_f_t a = srsran_simd_f_loadu(&x[i]);
      simd_f_t b = srsran_simd_f_loadu(&x[i + SRSRAN_SIMD_F_SIZE]);

      simd_f_t sa = srsran_simd_f_mul(a, s);
      simd_f_t sb = srsran_simd_f_mul(b, s);

      simd_s_t i16 = conversion_helper.convert(sa, sb);

      srsran_simd_s_storeu(&z[i], i16);
    }
  }
#endif /* SRSRAN_SIMD_F_SIZE && SRSRAN_SIMD_S_SIZE */
  details::gen_conversion_helper<ROUND> gen_conversion_helper;
  for (; i < len; i++) {
    z[i] = gen_conversion_helper.convert(x[i] * scale);
  }
}

static inline void convert_fi_simd(const float* x0, const float* x1, int16_t* z, float scale, unsigned len)
{
  len /= 2;

  unsigned i = 0;

#ifdef HAVE_SSE
  __m128 s = _mm_set1_ps(scale);
  if (SIMD_IS_SSE_ALIGNED(x0) && SIMD_IS_SSE_ALIGNED(x1) && SIMD_IS_SSE_ALIGNED(z)) {
    for (; i + 8 <= len; i += 8) {
      __m128 a1 = _mm_load_ps(&x0[i]);
      __m128 b1 = _mm_load_ps(&x1[i]);
      __m128 a2 = _mm_load_ps(&x0[i + 4]);
      __m128 b2 = _mm_load_ps(&x1[i + 4]);

      a1 = _mm_mul_ps(a1, s);
      b1 = _mm_mul_ps(b1, s);
      a2 = _mm_mul_ps(a2, s);
      b2 = _mm_mul_ps(b2, s);

      __m128i a1i = _mm_cvttps_epi32(a1);
      __m128i b1i = _mm_cvttps_epi32(b1);
      __m128i a2i = _mm_cvttps_epi32(a2);
      __m128i b2i = _mm_cvttps_epi32(b2);

      __m128i ai16 = _mm_packs_epi32(a1i, a2i);
      __m128i bi16 = _mm_packs_epi32(b1i, b2i);

      __m128i ci16 = _mm_unpacklo_epi32(ai16, bi16);
      __m128i di16 = _mm_unpackhi_epi32(ai16, bi16);

      _mm_store_si128((__m128i*)&z[2 * i], ci16);
      _mm_store_si128((__m128i*)&z[2 * i + 8], di16);
    }
  } else {
    for (; i + 8 <= len; i += 8) {
      __m128 a1 = _mm_loadu_ps(&x0[i]);
      __m128 b1 = _mm_loadu_ps(&x1[i]);
      __m128 a2 = _mm_loadu_ps(&x0[i + 4]);
      __m128 b2 = _mm_loadu_ps(&x1[i + 4]);

      a1 = _mm_mul_ps(a1, s);
      b1 = _mm_mul_ps(b1, s);
      a2 = _mm_mul_ps(a2, s);
      b2 = _mm_mul_ps(b2, s);

      __m128i a1i = _mm_cvttps_epi32(a1);
      __m128i b1i = _mm_cvttps_epi32(b1);
      __m128i a2i = _mm_cvttps_epi32(a2);
      __m128i b2i = _mm_cvttps_epi32(b2);

      __m128i ai16 = _mm_packs_epi32(a1i, a2i);
      __m128i bi16 = _mm_packs_epi32(b1i, b2i);

      __m128i ci16 = _mm_unpacklo_epi32(ai16, bi16);
      __m128i di16 = _mm_unpackhi_epi32(ai16, bi16);

      _mm_storeu_si128((__m128i*)&z[2 * i], ci16);
      _mm_storeu_si128((__m128i*)&z[2 * i + 8], di16);
    }
  }
#endif /* HAVE_SSE */

  for (; i < len; i += 2) {
    z[2 * i + 0] = (int16_t)(x0[i + 0] * scale);
    z[2 * i + 1] = (int16_t)(x0[i + 1] * scale);
    z[2 * i + 2] = (int16_t)(x1[i + 0] * scale);
    z[2 * i + 3] = (int16_t)(x1[i + 1] * scale);
  }
}

static inline void convert_if_simd(const int16_t* x, float* z, float scale, unsigned len)
{
  unsigned    i    = 0;
  const float gain = 1.0f / scale;

#ifdef HAVE_SSE
  __m128 s = _mm_set1_ps(gain);
  if (SIMD_IS_ALIGNED(z)) {
    for (; i + 4 <= len; i += 4) {
      __m64* ptr = (__m64*)&x[i];
      __m128 fl  = _mm_cvtpi16_ps(*ptr);
      __m128 v   = _mm_mul_ps(fl, s);

      _mm_store_ps(&z[i], v);
    }
  } else {
    for (; i + 4 <= len; i += 4) {
      __m64* ptr = (__m64*)&x[i];
      __m128 fl  = _mm_cvtpi16_ps(*ptr);
      __m128 v   = _mm_mul_ps(fl, s);

      _mm_storeu_ps(&z[i], v);
    }
  }
#endif /* HAVE_SSE */

  for (; i < len; i++) {
    z[i] = ((float)x[i]) * gain;
  }
}

static inline void convert_if_simd(const int16_t* x, float* z0, float* z1, float scale, unsigned len)
{
  len /= 2;

  unsigned    i    = 0;
  const float gain = 1.0f / scale;

#ifdef HAVE_SSE
  __m128 s = _mm_set1_ps(gain);
  if (SIMD_IS_SSE_ALIGNED(z0) && SIMD_IS_SSE_ALIGNED(z1)) {
    for (; i + 4 <= len; i += 4) {
      __m64 a = *(__m64*)&x[2 * i];
      __m64 b = *(__m64*)&x[2 * i + 4];

      __m128 x0 = _mm_cvtpi16_ps(a);
      __m128 x1 = _mm_cvtpi16_ps(b);

      __m128 v0 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(1, 0, 1, 0));
      __m128 v1 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(3, 2, 3, 2));

      _mm_store_ps(&z0[i], _mm_mul_ps(v0, s));
      _mm_store_ps(&z1[i], _mm_mul_ps(v1, s));
    }
  } else {
    for (; i + 4 <= len; i += 4) {
      __m64 a = *(__m64*)&x[2 * i];
      __m64 b = *(__m64*)&x[2 * i + 1];

      __m128 x0 = _mm_cvtpi16_ps(a);
      __m128 x1 = _mm_cvtpi16_ps(b);

      __m128 v0 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(1, 0, 1, 0));
      __m128 v1 = _mm_shuffle_ps(x0, x1, _MM_SHUFFLE(3, 2, 3, 2));

      _mm_storeu_ps(&z0[i], _mm_mul_ps(v0, s));
      _mm_storeu_ps(&z1[i], _mm_mul_ps(v1, s));
    }
  }
#endif /* HAVE_SSE */

  for (; i < len; i += 2) {
    z0[i + 0] = (float)x[2 * i + 0] * gain;
    z0[i + 1] = (float)x[2 * i + 1] * gain;
    z1[i + 0] = (float)x[2 * i + 2] * gain;
    z1[i + 1] = (float)x[2 * i + 3] * gain;
  }
}

void srsran::srsvec::convert(span<const cf_t> x, float scale, span<int8_t> z)
{
  assert(2 * x.size() == z.size());

  convert_fb_simd((const float*)x.data(), z.data(), scale, z.size());
}

void srsran::srsvec::convert(span<const cf_t> x0, span<const cf_t> x1, float scale, span<int8_t> z)
{
  assert(x0.size() == x1.size());
  assert(2 * x0.size() + 2 * x1.size() == z.size());

  convert_fb_simd((const float*)x0.data(), (const float*)x1.data(), z.data(), scale, z.size());
}

void srsran::srsvec::convert(span<const int8_t> x, float scale, span<cf_t> z)
{
  assert(x.size() == 2 * z.size());

  convert_bf_simd(x.data(), (float*)z.data(), scale, x.size());
}

void srsran::srsvec::convert(span<const int8_t> x, float scale, span<cf_t> z0, span<cf_t> z1)
{
  assert(z0.size() == z1.size());
  assert(x.size() == 2 * z0.size() + 2 * z1.size());

  convert_bf_simd(x.data(), (float*)z0.data(), (float*)z1.data(), scale, x.size());
}

void srsran::srsvec::convert(span<const cf_t> x, float scale, span<int16_t> z)
{
  assert(2 * x.size() == z.size());

  convert_fi_simd((const float*)x.data(), z.data(), scale, z.size());
}

void srsran::srsvec::convert(span<const cf_t> x0, span<const cf_t> x1, float scale, span<int16_t> z)
{
  assert(x0.size() == x1.size());
  assert(2 * x0.size() + 2 * x1.size() == z.size());

  convert_fi_simd((const float*)x0.data(), (const float*)x1.data(), z.data(), scale, z.size());
}

void srsran::srsvec::convert_round(span<const cf_t> x, float scale, span<int16_t> z)
{
  assert(2 * x.size() == z.size());

  convert_fi_simd</*ROUND =*/true>((const float*)x.data(), z.data(), scale, z.size());
}

void srsran::srsvec::convert_swap(span<const cf_t> x, float scale, span<int16_t> z)
{
  assert(2 * x.size() == z.size());

  convert_fi_simd((const float*)x.data(), z.data(), scale, z.size());

  // Perform I/Q swap
  for (unsigned i = 0; i != z.size(); i += 2) {
    int16_t temp = z[i];
    z[i]         = z[i + 1];
    z[i + 1]     = temp;
  }
}

void srsran::srsvec::convert(span<const int16_t> x, float scale, span<cf_t> z)
{
  assert(x.size() == 2 * z.size());

  convert_if_simd(x.data(), (float*)z.data(), scale, x.size());
}

void srsran::srsvec::convert(span<const int16_t> x, float scale, span<cf_t> z0, span<cf_t> z1)
{
  assert(z0.size() == z1.size());
  assert(x.size() == 2 * z0.size() + 2 * z1.size());

  convert_if_simd(x.data(), (float*)z0.data(), (float*)z1.data(), scale, x.size());
}

void srsran::srsvec::convert_swap(span<const int16_t> x, float scale, span<cf_t> z)
{
  assert(x.size() == 2 * z.size());

  convert_if_simd(x.data(), (float*)z.data(), scale, x.size());

  // Perform I/Q swap
  for (unsigned i = 0; i != z.size(); i++) {
    cf_t temp = z[i];
    z[i]      = {temp.imag(), temp.real()};
  }
}

void srsran::srsvec::convert(span<const float> x, float scale, span<int16_t> z)
{
  assert(x.size() == z.size());

  convert_fi_simd(x.data(), z.data(), scale, z.size());
}

void srsran::srsvec::convert_round(span<const float> x, float scale, span<int16_t> z)
{
  assert(x.size() == z.size());

  convert_fi_simd</*ROUND =*/true>(x.data(), z.data(), scale, z.size());
}

void srsran::srsvec::convert(span<const int16_t> x, float scale, span<float> z)
{
  assert(x.size() == z.size());

  convert_if_simd(x.data(), z.data(), scale, z.size());
}
