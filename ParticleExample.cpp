#include "ParticleExample.h"
#include "Noise.h"
#include <vector>
#include <cmath>

// Global static texture variables
static SDL_Texture* s_snowTexture = NULL;
static SDL_Texture* s_rainTexture = NULL;
static SDL_Texture* s_leafTexture = NULL;
static SDL_Texture* s_dustTexture = NULL;
static SDL_Texture* s_fireTexture = NULL;
static SDL_Texture* s_smokeTexture   = NULL;
static SDL_Texture* s_windTexture   = NULL;

// Procedural texture generation structures
struct GlowStop { float d; float r, g, b, a; };

// Procedural texture: Radial glow with optional FBM distortion for Fire/Smoke
static SDL_Texture* makeRadialGlow(SDL_Renderer* ren, int size,
                                   const std::vector<GlowStop>& stops,
                                   float distortion) {
    if (!ren) return nullptr;
    std::vector<uint32_t> pixels((size_t)size * size);
    float cx = size * 0.5f, cy = size * 0.5f, invR = 1.0f / (size * 0.5f);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - cx, dy = y - cy;
            float d0 = sqrtf(dx*dx + dy*dy) * invR;
            if (d0 >= 1.0f) { pixels[y*size + x] = 0; continue; }
            float d = d0;
            if (distortion > 0.0f) {
                d += noise::fbm2D(x * 0.18f, y * 0.18f, 3) * distortion;
                if (d < 0.0f) d = 0.0f; if (d > 1.0f) d = 1.0f;
            }
            float r=0, g=0, b=0, a=0;
            if (stops.empty()) { /* keep 0 */ }
            else if (d <= stops.front().d) {
                r = stops.front().r; g = stops.front().g; b = stops.front().b; a = stops.front().a;
            } else if (d >= stops.back().d) {
                r = stops.back().r; g = stops.back().g; b = stops.back().b; a = stops.back().a;
            } else {
                for (size_t k = 0; k + 1 < stops.size(); ++k) {
                    if (d >= stops[k].d && d <= stops[k+1].d) {
                        float u = (d - stops[k].d) / (stops[k+1].d - stops[k].d);
                        r = stops[k].r + (stops[k+1].r - stops[k].r) * u;
                        g = stops[k].g + (stops[k+1].g - stops[k].g) * u;
                        b = stops[k].b + (stops[k+1].b - stops[k].b) * u;
                        a = stops[k].a + (stops[k+1].a - stops[k].a) * u;
                        break;
                    }
                }
            }
            uint8_t R = (uint8_t)(r * 255.0f);
            uint8_t G = (uint8_t)(g * 255.0f);
            uint8_t B = (uint8_t)(b * 255.0f);
            uint8_t A = (uint8_t)(a * 255.0f);
            pixels[y*size + x] = ((uint32_t)R << 24) | ((uint32_t)G << 16) |
                     ((uint32_t)B << 8)  |  (uint32_t)A;
        }
    }
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_STATIC, size, size);
    if (tex) {
        SDL_UpdateTexture(tex, nullptr, pixels.data(), size * (int)sizeof(uint32_t));
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    return tex;
}

static SDL_Texture* makeFireGlowTexture(SDL_Renderer* ren) {
    std::vector<GlowStop> stops = {
        {0.00f, 1.00f, 0.95f, 0.65f, 1.00f},
        {0.18f, 1.00f, 0.80f, 0.25f, 0.95f},
        {0.42f, 0.95f, 0.40f, 0.08f, 0.65f},
        {0.72f, 0.65f, 0.15f, 0.02f, 0.30f},
        {1.00f, 0.25f, 0.05f, 0.00f, 0.00f},
    };
    return makeRadialGlow(ren, 128, stops, 0.18f);
}

static SDL_Texture* makeSmokeCloudTexture(SDL_Renderer* ren) {
    std::vector<GlowStop> stops = {
        {0.00f, 0.95f, 0.95f, 0.95f, 0.55f},
        {0.35f, 0.85f, 0.85f, 0.85f, 0.40f},
        {0.70f, 0.70f, 0.70f, 0.70f, 0.15f},
        {1.00f, 0.60f, 0.60f, 0.60f, 0.00f},
    };
    return makeRadialGlow(ren, 128, stops, 0.40f);
}

static SDL_Texture* makeSnowTexture(SDL_Renderer* ren) {
    std::vector<GlowStop> stops = {
        {0.00f, 1.00f, 1.00f, 1.00f, 1.00f}, 
        {0.50f, 1.00f, 1.00f, 1.00f, 0.70f}, 
        {1.00f, 1.00f, 1.00f, 1.00f, 0.00f}, 
    };
    return makeRadialGlow(ren, 32, stops, 0.0f);
}

static SDL_Texture* makeDustTexture(SDL_Renderer* ren) {
    std::vector<GlowStop> stops = {
        {0.00f, 0.85f, 0.75f, 0.60f, 0.80f}, 
        {0.40f, 0.80f, 0.70f, 0.50f, 0.50f}, 
        {1.00f, 0.75f, 0.65f, 0.45f, 0.00f}, 
    };
    return makeRadialGlow(ren, 32, stops, 0.1f);
}

static SDL_Texture* makeRainTexture(SDL_Renderer* ren) {
    int size = 32;
    std::vector<uint32_t> pixels(size * size, 0);
    
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float u = (float)x / (size - 1) * 2.0f - 1.0f; 
            float v = (float)y / (size - 1) * 2.0f - 1.0f;
            float d = sqrtf(u * u * 4.0f + v * v);
            
            if (d <= 1.0f) {
                float a = 1.0f - d; 
                uint8_t alpha = (uint8_t)(a * 255.0f); 
                pixels[y * size + x] = (255 << 24) | (255 << 16) | (255 << 8) | alpha;
            }
        }
    }
    
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, size, size);
    if (tex) {
        SDL_UpdateTexture(tex, nullptr, pixels.data(), size * (int)sizeof(uint32_t));
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    return tex;
}

static SDL_Texture* makeWindTexture(SDL_Renderer* ren) {
    int w = 64, h = 16; 
    std::vector<uint32_t> pixels(w * h, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float u = (float)x / (w - 1) * 2.0f - 1.0f;
            float v = (float)y / (h - 1) * 2.0f - 1.0f;
            float d = sqrtf(u * u + v * v * 16.0f); 
            if (d <= 1.0f) {
                float a = (1.0f - d) * (1.0f - d);
                uint8_t alpha = (uint8_t)(a * 255.0f);
                pixels[y * w + x] = (255 << 24) | (255 << 16) | (255 << 8) | alpha;
            }
        }
    }
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, w, h);
    if (tex) {
        SDL_UpdateTexture(tex, nullptr, pixels.data(), w * (int)sizeof(uint32_t));
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    return tex;
}

static SDL_Texture* makeLeafTexture(SDL_Renderer* ren) {
    int size = 32;
    std::vector<uint32_t> pixels(size * size, 0);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = (float)x / (size - 1) * 2.0f - 1.0f;
            float dy = (float)y / (size - 1) * 2.0f - 1.0f;
            float d = dx * dx + dy * dy + 2.0f * fabs(dx * dy);
            
            if (d < 1.0f) {
                float a = 1.0f - sqrtf(d); 
                if (a < 0.0f) a = 0.0f;
                uint8_t R = 80 + (uint8_t)(fabs(dy) * 40); 
                uint8_t G = 150 + (uint8_t)((1.0f - fabs(dy)) * 80);
                uint8_t B = 40;
                uint8_t A = (uint8_t)(a * 255.0f);
                pixels[y * size + x] = (R << 24) | (G << 16) | (B << 8) | A;
            }
        }
    }
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, size, size);
    if (tex) {
        SDL_UpdateTexture(tex, nullptr, pixels.data(), size * (int)sizeof(uint32_t));
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    return tex;
}

void ParticleExample::FreeDefaultTextures()
{
    if (s_snowTexture) { SDL_DestroyTexture(s_snowTexture); s_snowTexture = NULL; }
    if (s_rainTexture) { SDL_DestroyTexture(s_rainTexture); s_rainTexture = NULL; }
    if (s_leafTexture) { SDL_DestroyTexture(s_leafTexture); s_leafTexture = NULL; }
    if (s_dustTexture) { SDL_DestroyTexture(s_dustTexture); s_dustTexture = NULL; }
    if (s_fireTexture) { SDL_DestroyTexture(s_fireTexture); s_fireTexture = NULL; }
    if (s_smokeTexture) { SDL_DestroyTexture(s_smokeTexture); s_smokeTexture = NULL; }
    if (s_windTexture) { SDL_DestroyTexture(s_windTexture); s_windTexture = NULL; }
}

void ParticleExample::resetSystem()
{
    stopSystem();
    style_ = NONE;
    setTexture(NULL); 
}

SDL_Texture* ParticleExample::getDefaultTexture()
{
    SDL_Texture** targetCache = nullptr;
    const char* fileName = nullptr;

    switch (style_)
    {
        case SNOW: targetCache = &s_snowTexture; fileName = "snow.png"; break;
        case RAIN: targetCache = &s_rainTexture; fileName = "rain.png"; break;
        case FALLING_LEAVES: targetCache = &s_leafTexture; fileName = "leaf.png"; break;
        case DUST_STORM: targetCache = &s_dustTexture; fileName = "dust.png"; break;
        case SMOKE: targetCache = &s_smokeTexture; fileName = "smoke.png"; break;
        case WIND: targetCache = &s_windTexture; fileName = "wind.png"; break;
        case FIRE:
        case FIRE_WORK:
        case EXPLOSION:
        default:
            targetCache = &s_fireTexture; fileName = "fire.png"; break;
    }

    if (targetCache && *targetCache) {
        return *targetCache;
    }

    // Try loading texture from disk
    SDL_Texture* newTexture = IMG_LoadTexture(_renderer, fileName);

    // Fallback to procedural generation if not found
    if (!newTexture && targetCache) {
        if (targetCache == &s_fireTexture) newTexture = makeFireGlowTexture(_renderer);
        else if (targetCache == &s_smokeTexture) newTexture = makeSmokeCloudTexture(_renderer);
        else if (targetCache == &s_snowTexture) newTexture = makeSnowTexture(_renderer);
        else if (targetCache == &s_rainTexture) newTexture = makeRainTexture(_renderer);
        else if (targetCache == &s_leafTexture) newTexture = makeLeafTexture(_renderer);
        else if (targetCache == &s_dustTexture) newTexture = makeDustTexture(_renderer);
        else if (targetCache == &s_windTexture) newTexture = makeWindTexture(_renderer);
    }

    if (newTexture && targetCache) {
        *targetCache = newTexture;
    }

    return newTexture;
}

void ParticleExample::setStyle(PatticleStyle style)
{
    if (style_ == style)
    {
        return;
    }
    style_ = style;
    if (style == NONE)
    {
        stopSystem();
    }
    
    setTexture(getDefaultTexture());

    // Reset AE features to prevent crossover between style changes
    clearColorRamp();
    setBlendMode(BlendMode::ALPHA);
    setTurbulence(0.0f, 0.0f);
    resetTurbulenceTime();

    bool needPreWarm = false;
    float preWarmRatio = 1.0f;

    switch (style)
    {
    case ParticleExample::FIRE:
    {
        initWithTotalParticles(250);

        // duration
        _duration = DURATION_INFINITY;

        // Gravity Mode
        this->_emitterMode = Mode::GRAVITY;

        // Gravity Mode: gravity
        this->modeA.gravity = { 0, 0 };

        // Gravity Mode: radial acceleration
        this->modeA.radialAccel = 0;
        this->modeA.radialAccelVar = 0;

        // Gravity Mode: speed of particles
        this->modeA.speed = -60;
        this->modeA.speedVar = 20;

        // starting angle
        _angle = 90;
        _angleVar = 10;

        // life of particles
        _life = 3;
        _lifeVar = 0.25f;

        // size, in pixels
        _startSize = 54.0f;
        _startSizeVar = 10.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;

        // emits per frame
        _emissionRate = _totalParticles / _life;

        // original fallback colors
        _startColor.r = 0.76f; _startColor.g = 0.25f; _startColor.b = 0.12f; _startColor.a = 1.0f;
        _startColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _endColor = {0.0f, 0.0f, 0.0f, 0.0f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _posVar = { 40.0f, 20.0f };

        // New AE Additions
        setBlendMode(BlendMode::ADD);
        setTurbulence(120.0f, 0.009f);
        setColorRamp({
            {0.00f, 1.00f, 0.95f, 0.70f, 1.00f},
            {0.20f, 1.00f, 0.75f, 0.20f, 0.95f},
            {0.50f, 0.95f, 0.40f, 0.08f, 0.70f},
            {0.80f, 0.60f, 0.15f, 0.02f, 0.30f},
            {1.00f, 0.20f, 0.05f, 0.00f, 0.00f},
        });
        needPreWarm = true;
        preWarmRatio = 0.7f;
        break;
    }
    case ParticleExample::FIRE_WORK:
    {
        initWithTotalParticles(1500);

        _duration = DURATION_INFINITY;
        this->_emitterMode = Mode::GRAVITY;
        this->modeA.gravity = { 0.0f, 90.0f };
        this->modeA.radialAccel = 0.0f;
        this->modeA.radialAccelVar = 0.0f;
        this->modeA.speed = -180.0f;
        this->modeA.speedVar = 50.0f;
        this->_angle = 90.0f;
        this->_angleVar = 20.0f;
        this->_life = 3.5f;
        this->_lifeVar = 1.0f;
        this->_emissionRate = _totalParticles / _life;

        _startColor = { 0.5f, 0.5f, 0.5f, 1.0f };
        _startColorVar = { 0.5f, 0.5f, 0.5f, 0.1f };
        _endColor = { 0.1f, 0.1f, 0.1f, 0.2f };
        _endColorVar = { 0.1f, 0.1f, 0.1f, 0.2f };

        _startSize = 8.0f;
        _startSizeVar = 2.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _posVar = { 0, 0 };

        // New AE Additions
        setBlendMode(BlendMode::ADD);
        setTurbulence(95.0f, 0.010f);
        setColorRamp({
            {0.00f, 1.00f, 0.95f, 0.70f, 1.00f},
            {0.30f, 1.00f, 0.60f, 0.20f, 0.90f},
            {0.70f, 0.80f, 0.25f, 0.10f, 0.40f},
            {1.00f, 0.40f, 0.10f, 0.05f, 0.00f},
        });
        break;
    }
    case ParticleExample::SUN:
    {
        initWithTotalParticles(350);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(0, 0));
        setRadialAccel(0);
        setRadialAccelVar(0);
        setSpeed(-20);
        setSpeedVar(5);
        _angle = 90;
        _angleVar = 360;
        _life = 1;
        _lifeVar = 0.5f;
        _startSize = 30.0f;
        _startSizeVar = 10.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = _totalParticles / _life;

        _startColor.r = 0.76f; _startColor.g = 0.25f; _startColor.b = 0.12f; _startColor.a = 1.0f;
        _startColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _endColor = {0.0f, 0.0f, 0.0f, 1.0f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _posVar = { 0, 0 };

        // New AE Additions
        setBlendMode(BlendMode::ADD);
        setColorRamp({
            {0.00f, 1.00f, 0.95f, 0.80f, 1.00f},
            {0.30f, 1.00f, 0.70f, 0.20f, 0.80f},
            {1.00f, 0.80f, 0.20f, 0.00f, 0.00f},
        });
        break;
    }
    case ParticleExample::GALAXY:
    {
        initWithTotalParticles(200);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(0, 0));
        setSpeed(-60);
        setSpeedVar(10);
        setRadialAccel(-80);
        setRadialAccelVar(0);
        setTangentialAccel(80);
        setTangentialAccelVar(0);
        _angle = 90;
        _angleVar = 360;
        _life = 4;
        _lifeVar = 1;
        _startSize = 37.0f;
        _startSizeVar = 10.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = _totalParticles / _life;

        _startColor.r = 0.12f; _startColor.g = 0.25f; _startColor.b = 0.76f; _startColor.a = 1.0f;
        _startColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _endColor = {0.0f, 0.0f, 0.0f, 1.0f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _posVar = { 0, 0 };

        // New AE Additions
        setBlendMode(BlendMode::ADD);
        setTurbulence(45.0f, 0.012f);
        setColorRamp({
            {0.00f, 0.20f, 0.40f, 1.00f, 0.00f},
            {0.25f, 0.30f, 0.50f, 1.00f, 0.80f},
            {0.80f, 0.80f, 0.60f, 1.00f, 0.90f},
            {1.00f, 1.00f, 1.00f, 1.00f, 0.00f},
        });
        break;
    }
    case ParticleExample::FLOWER:
    {
        initWithTotalParticles(250);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(0, 0));
        setSpeed(-80);
        setSpeedVar(10);
        setRadialAccel(-60);
        setRadialAccelVar(0);
        setTangentialAccel(15);
        setTangentialAccelVar(0);
        _angle = 90;
        _angleVar = 360;
        _life = 4;
        _lifeVar = 1;
        _startSize = 30.0f;
        _startSizeVar = 10.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = _totalParticles / _life;

        _startColor = {0.50f, 0.50f, 0.50f, 1.0f};
        _startColorVar = {0.5f, 0.5f, 0.5f, 0.5f};
        _endColor = {0.0f, 0.0f, 0.0f, 1.0f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _posVar = { 0, 0 };
        break;
    }
    case ParticleExample::METEOR:
    {
        initWithTotalParticles(150);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(-200, -200));
        setSpeed(-15);
        setSpeedVar(5);
        setRadialAccel(0);
        setRadialAccelVar(0);
        setTangentialAccel(0);
        setTangentialAccelVar(0);
        _angle = 90;
        _angleVar = 360;
        _life = 2;
        _lifeVar = 1;
        _startSize = 60.0f;
        _startSizeVar = 10.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = _totalParticles / _life;

        _startColor = {0.2f, 0.4f, 0.7f, 1.0f};
        _startColorVar = {0.0f, 0.0f, 0.2f, 0.1f};
        _endColor = {0.0f, 0.0f, 0.0f, 1.0f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _posVar = { 0, 0 };

        // New AE Additions
        setBlendMode(BlendMode::ADD);
        setColorRamp({
            {0.00f, 1.00f, 1.00f, 1.00f, 1.00f},
            {0.20f, 0.20f, 0.80f, 1.00f, 0.80f},
            {1.00f, 0.00f, 0.20f, 0.80f, 0.00f},
        });
        break;
    }
    case ParticleExample::SPIRAL:
    {
        initWithTotalParticles(500);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(0, 0));
        setSpeed(-150);
        setSpeedVar(0);
        setRadialAccel(-380);
        setRadialAccelVar(0);
        setTangentialAccel(45);
        setTangentialAccelVar(0);
        _angle = 90;
        _angleVar = 0;
        _life = 12;
        _lifeVar = 0;
        _startSize = 20.0f;
        _startSizeVar = 0.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = _totalParticles / _life;

        _startColor = {0.5f, 0.5f, 0.5f, 1.0f};
        _startColorVar = {0.5f, 0.5f, 0.5f, 0.0f};
        _endColor = {0.5f, 0.5f, 0.5f, 1.0f};
        _endColorVar = {0.5f, 0.5f, 0.5f, 0.0f};
        _posVar = { 0, 0 };

        // New AE Additions
        setBlendMode(BlendMode::ADD);
        setColorRamp({
            {0.00f, 0.20f, 1.00f, 0.40f, 0.00f},
            {0.15f, 0.30f, 1.00f, 0.50f, 0.90f},
            {0.80f, 0.10f, 0.80f, 0.40f, 0.60f},
            {1.00f, 0.00f, 0.30f, 0.10f, 0.00f},
        });
        break;
    }
    case ParticleExample::EXPLOSION:
    {
        initWithTotalParticles(700);
        _duration = 0.1f;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(0, 0));
        setSpeed(-70);
        setSpeedVar(40);
        setRadialAccel(0);
        setRadialAccelVar(0);
        setTangentialAccel(0);
        setTangentialAccelVar(0);
        _angle = 90;
        _angleVar = 360;
        _life = 5.0f;
        _lifeVar = 2;
        _startSize = 15.0f;
        _startSizeVar = 10.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = _totalParticles / _duration;

        _startColor = {0.7f, 0.1f, 0.2f, 1.0f};
        _startColorVar = {0.5f, 0.5f, 0.5f, 0.0f};
        _endColor = {0.5f, 0.5f, 0.5f, 0.0f};
        _endColorVar = {0.5f, 0.5f, 0.5f, 0.0f};
        _posVar = { 0, 0 };

        // New AE Additions
        setBlendMode(BlendMode::ADD);
        setTurbulence(170.0f, 0.014f);
        setColorRamp({
            {0.00f, 1.00f, 0.95f, 0.60f, 1.00f},
            {0.25f, 1.00f, 0.70f, 0.15f, 0.90f},
            {0.60f, 0.85f, 0.30f, 0.05f, 0.50f},
            {1.00f, 0.30f, 0.05f, 0.00f, 0.00f},
        });
        break;
    }
    case ParticleExample::SMOKE:
    {
        initWithTotalParticles(200);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(0, 0));
        setRadialAccel(0);
        setRadialAccelVar(0);
        setSpeed(-25);
        setSpeedVar(10);
        _angle = 90;
        _angleVar = 5;
        _life = 4;
        _lifeVar = 1;
        _startSize = 60.0f;
        _startSizeVar = 10.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = _totalParticles / _life;

        _startColor = {0.8f, 0.8f, 0.8f, 1.0f};
        _startColorVar = {0.02f, 0.02f, 0.02f, 0.0f};
        _endColor = {0.0f, 0.0f, 0.0f, 1.0f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _posVar = { 20.0f, 0.0f };

        // New AE Additions
        setBlendMode(BlendMode::ALPHA);
        setTurbulence(75.0f, 0.006f);
        setColorRamp({
            {0.00f, 0.90f, 0.90f, 0.90f, 0.70f},
            {0.40f, 0.78f, 0.78f, 0.78f, 0.55f},
            {0.80f, 0.60f, 0.60f, 0.60f, 0.20f},
            {1.00f, 0.45f, 0.45f, 0.45f, 0.00f},
        });
        needPreWarm = true;
        preWarmRatio = 0.6f;
        break;
    }
    case ParticleExample::SNOW:
    {
        initWithTotalParticles(700);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(0, 1));
        setSpeed(-5);
        setSpeedVar(1);
        setRadialAccel(0);
        setRadialAccelVar(1);
        setTangentialAccel(0);
        setTangentialAccelVar(1);
        _angle = -90;
        _angleVar = 5;
        _life = 45;
        _lifeVar = 15;
        _startSize = 10.0f;
        _startSizeVar = 5.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = 10;

        _startColor = {1.0f, 1.0f, 1.0f, 1.0f};
        _startColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _endColor = {1.0f, 1.0f, 1.0f, 0.0f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _posVar = { 1.0f * x_, 0.0f };
        
        needPreWarm = true;
        preWarmRatio = 0.8f;
        break;
    }
    case ParticleExample::RAIN:
    {
        initWithTotalParticles(5000);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(10, 10));
        setRadialAccel(0);
        setRadialAccelVar(1);
        setTangentialAccel(0);
        setTangentialAccelVar(1);
        setSpeed(-130);
        setSpeedVar(30);
        _angle = -90;
        _angleVar = 5;
        _life = 4.5f;
        _lifeVar = 0;
        _startSize = 4.0f;
        _startSizeVar = 2.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = 200;

        _startColor = {0.7f, 0.8f, 1.0f, 1.0f};
        _startColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _endColor = {0.7f, 0.8f, 1.0f, 0.5f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _posVar = { 1.0f * x_, 0.0f };

        needPreWarm = true;
        preWarmRatio = 0.8f;
        break;
    }
    // New Weather Styles Adapted for old bounds
    case ParticleExample::FALLING_LEAVES: 
    {
        initWithTotalParticles(80);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(10, 8));
        setSpeed(-60);
        setSpeedVar(30);
        setRadialAccel(0);
        setRadialAccelVar(1);
        setTangentialAccel(20);
        setTangentialAccelVar(20);
        _angle = -90;
        _angleVar = 45;
        _life = 18;
        _lifeVar = 6;
        _startSpin = 0.0f;
        _startSpinVar = 360.0f;
        _endSpin = 720.0f;
        _endSpinVar = 360.0f;
        _startSize = 16.0f;
        _startSizeVar = 8.0f;
        _endSize = START_SIZE_EQUAL_TO_END_SIZE;
        _emissionRate = 3;

        _startColor = {1.0f, 1.0f, 1.0f, 1.0f};
        _startColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _endColor = {1.0f, 1.0f, 1.0f, 0.0f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        _posVar = { 1.0f * x_, 0.0f };
        
        needPreWarm = true;
        preWarmRatio = 0.8f;
        break;
    }
    case ParticleExample::DUST_STORM: 
    {
        initWithTotalParticles(12000); 
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(1200, -800)); 
        _startSize = 14.0f;   
        _startSizeVar = 10.0f; 
        _endSize = 6.0f;     
        _endSizeVar = 3.0f;

        _startColor = {0.82f, 0.66f, 0.40f, 0.8f};
        _startColorVar = {0.02f, 0.02f, 0.02f, 0.3f}; 
        _endColor = {0.35f, 0.25f, 0.10f, 0.4f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};

        _sourcePosition = Vec2(x_ * -0.2f, y_ * 1.2f);
        _posVar = Vec2(x_ * 1.5f, y_ * 1.0f);
        
        _angle = -25; 
        _angleVar = 15;
        setSpeed(1000);
        setSpeedVar(400); 
        setTangentialAccel(60); 
        setTangentialAccelVar(30);

        _startSpin = 0;
        _startSpinVar = 360;
        _endSpin = 360;
        _endSpinVar = 360;

        _life = 2.5f;
        _lifeVar = 0.5f;
        _emissionRate = _totalParticles / _life;
        break;
    }
    case ParticleExample::WIND:
    {
        initWithTotalParticles(250);
        _duration = DURATION_INFINITY;
        setEmitterMode(Mode::GRAVITY);
        setGravity(Vec2(2000, 50)); 
        setSpeed(1500); 
        setSpeedVar(400); 
        _angle = 5; 
        _angleVar = 8;
        
        setRotationIsDir(true);
        
        _life = 0.8f; 
        _lifeVar = 0.3f;
        
        _startSize = 120.0f; _startSizeVar = 50.0f;
        _endSize = 30.0f; _endSizeVar = 10.0f;
        _emissionRate = _totalParticles / _life;
        
        setBlendMode(BlendMode::ADD);
        
        _startColor = {0.8f, 0.9f, 1.0f, 0.25f};
        _startColorVar = {0.1f, 0.1f, 0.0f, 0.1f};
        _endColor = {0.7f, 0.8f, 0.9f, 0.0f};
        _endColorVar = {0.0f, 0.0f, 0.0f, 0.0f};
        
        _sourcePosition = Vec2(-100, y_ / 2); 
        _posVar = { 50.0f, (float)y_ }; 
        
        needPreWarm = true;
        preWarmRatio = 1.0f;
        break;
    }
    default:
        break;
    }

    // Pre-warm logic to fill screen instantly
    if (needPreWarm && _emissionRate > 0)
    {
        float warmTime = _life * preWarmRatio;
        float step = 0.05f;
        for (float t = 0; t < warmTime; t += step)
        {
            this->update(step);
        }
    }
}