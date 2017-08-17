//inkball?
#include "pong.h"

int startup(SDL_Window** window, SDL_Renderer** renderer);
void shutdown(SDL_Window* window, SDL_Renderer* renderer);

int main(int, char **)
{
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    if (startup(&window, &renderer)==0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Pong", "Player 1: w & s\nPlayer 2: up & down", window);
        loop(renderer);
    }
    else
    {
        (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Setup failed, SDL error:", SDL_GetError(), NULL);
    }

    shutdown(window, renderer);
    return 0;
}

int startup(SDL_Window** winref, SDL_Renderer** rendref)
{
    if (SDL_Init(SDL_INIT_VIDEO)==0
    && (*winref=SDL_CreateWindow("Pong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ScreenW, ScreenH, 0))!=NULL
    && (*rendref=SDL_CreateRenderer(*winref, -1, SDL_RENDERER_ACCELERATED))!=NULL)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

void shutdown(SDL_Window* window, SDL_Renderer* renderer)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
