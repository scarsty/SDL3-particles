# SDL3-particles

A Particle System for SDL3.

Modified from that of Cocos2dx.

It is a part of kys-cpp (<https://www.github.com/scarsty/kys-cpp>).

## How to use:

An example has been supplied in main.cpp, please notice the comments:

```c++
#include "SDL3/SDL.h"
#include "ParticleExample.h"

int main(int, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    auto win = SDL_CreateWindow("SDL3 Particles", 1024, 768, SDL_WINDOW_OPENGL);
    auto ren = SDL_CreateRenderer(win, nullptr);

    auto p = new ParticleExample();        // create a new particle system pointer
    p->setRenderer(ren);                   // set the renderer
    p->setPosition(512, 384);              // set the position
    p->setStyle(ParticleExample::FIRE);    // set the example effects

    while (1)
    {
        SDL_Event e;
        SDL_PollEvent(&e);
        if (e.type == SDL_EVENT_KEY_UP)
        {
            int s = (e.key.key - SDLK_A + 1);
            p->setStyle(ParticleExample::PatticleStyle(s));    // switch the example effects
        }

        SDL_RenderClear(ren);
        p->draw();    // you have to draw it in each loop
        SDL_RenderPresent(ren);
        SDL_Delay(10);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    delete p;    // destroy it

    return 0;
}
```

You can press A ~ K to switch the 11 example effects.



## Effect Examples

Fire

![fire](https://raw.githubusercontent.com/scarsty/SDL3-particles/master/pic/fire.png)

Snow

![snow](https://raw.githubusercontent.com/scarsty/SDL3-particles/master/pic/snow.png)