#include "ParticleExample.h"
#include "SDL3/SDL.h"

int main(int, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    auto win = SDL_CreateWindow("SDL3 Particles", 1024, 768, SDL_WINDOW_OPENGL);
    auto ren = SDL_CreateRenderer(win, nullptr);

    auto p = new ParticleExample();        // create a new particle system pointer
    p->setRenderer(ren);                   // set the renderer
    p->setPosition(512, 384);              // set the position
    p->setStyle(ParticleExample::FIRE);    // set the example effects
    p->setStartSpin(0);
    p->setStartSpinVar(90);
    p->setEndSpin(90);
    p->setStartSpinVar(90);

    bool running = true;
    
    Uint64 lastTime = SDL_GetTicks();

    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_KEY_UP)
            {
                int s = (e.key.key - SDLK_A + 1); 
                p->setStyle(ParticleExample::PatticleStyle(s)); 
            }
            if (e.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
        }

        Uint64 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        if (dt > 0.1f) dt = 0.1f;
        if (dt <= 0.0f) dt = 0.001f;

        float speedMultiplier = 2.5f; // 播放倍速
        dt *= speedMultiplier;

        p->update(dt);

        SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
        SDL_RenderClear(ren);

        // 5. 渲染
        p->draw();    
        
        SDL_RenderPresent(ren);
        SDL_Delay(10);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    delete p;    // destroy it

    return 0;
}