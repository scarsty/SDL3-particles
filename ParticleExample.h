#pragma once
#include "ParticleSystem.h"
#include "SDL3_image/SDL_image.h"

class ParticleExample : public ParticleSystem
{
public:
    ParticleExample() {}
    virtual ~ParticleExample() {}

    enum PatticleStyle
    {
        NONE,
        FIRE,
        FIRE_WORK,
        SUN,
        GALAXY,
        FLOWER,
        METEOR,
        SPIRAL,
        EXPLOSION,
        SMOKE,
        SNOW,
        RAIN,
        FALLING_LEAVES,
        DUST_STORM,
        WIND
    };

    PatticleStyle style_ = NONE;
    void setStyle(PatticleStyle style);
    
    SDL_Texture* getDefaultTexture();

    // Static cleanup function to clear generated textures
    static void FreeDefaultTextures();

    // Reset system state
    void resetSystem();
};