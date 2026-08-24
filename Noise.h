#pragma once
#include <cmath>
#include <cstdint>

// 2D value noise + FBM, used for particle turbulence and procedural
// texture distortion. Pure CPU, O(1) per sample, no deps.
// Output range ~ [-1, 1]. 4-octave FBM normalized.
namespace noise {
inline float hash21(float x, float y) {
    uint32_t h = (uint32_t)(x * 374761393.0f + y * 668265263.0f);
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return ((h & 0x00FFFFFFu) / 16777215.0f) * 2.0f - 1.0f; // [-1, 1]
}
inline float _smooth(float t) { return t * t * (3.0f - 2.0f * t); }

inline float valueNoise2D(float x, float y) {
    float xi = floorf(x), yi = floorf(y);
    float xf = x - xi, yf = y - yi;
    float a = hash21(xi,       yi);
    float b = hash21(xi + 1.0f, yi);
    float c = hash21(xi,       yi + 1.0f);
    float d = hash21(xi + 1.0f, yi + 1.0f);
    float u = _smooth(xf), v = _smooth(yf);
    return a*(1-u)*(1-v) + b*u*(1-v) + c*(1-u)*v + d*u*v;
}

// Fractal Brownian Motion. octaves=4 足够, 越大越细碎越慢.
inline float fbm2D(float x, float y, int octaves = 4) {
    float amp = 0.5f, freq = 1.0f, sum = 0.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum  += amp * valueNoise2D(x * freq, y * freq);
        norm += amp;
        amp  *= 0.5f;
        freq *= 2.0f;
    }
    return (norm > 0.0f) ? sum / norm : 0.0f;
}
} // namespace noise
