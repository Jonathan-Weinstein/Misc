#include "pong.h"

void keyStateMovePaddles(int *nearPadY, int *farPadY)
{
    const uint8_t *const held = SDL_GetKeyboardState(nullptr);

    if (held[SDL_SCANCODE_W] && *nearPadY>0) *nearPadY-=PaddleStep;

    if (held[SDL_SCANCODE_S] && *nearPadY<PaddleMaxY) *nearPadY+=PaddleStep;

    if (held[SDL_SCANCODE_UP] && *farPadY>0) *farPadY-=PaddleStep;

    if (held[SDL_SCANCODE_DOWN] && *farPadY<PaddleMaxY) *farPadY+=PaddleStep;
}

void updateScreen(SDL_Renderer* renderer, const SDL_Rect* ball, const SDL_Rect* nearPaddle, const SDL_Rect* farPaddle)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    //SDL_RenderDrawRect(renderer, ball);
    SDL_RenderFillRect(renderer, ball);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    //SDL_RenderDrawRect(renderer, nearPaddle);
    SDL_RenderFillRect(renderer, nearPaddle);

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    //SDL_RenderDrawRect(renderer, farPaddle);
    SDL_RenderFillRect(renderer, farPaddle);

    SDL_RenderPresent(renderer);
}

void loop(SDL_Renderer* renderer)
{
    SDL_Event event={};
    Ball ball;
    SDL_Rect    nearPaddle = {NearPaddleX, PaddleInitY, PaddleW, PaddleHeight},
                farPaddle  = {FarPaddleX,  PaddleInitY, PaddleW, PaddleHeight}; // x, y, w, h

    while(event.type!=SDL_QUIT)
    {
        uint32_t const startTicks=SDL_GetTicks();

        SDL_PollEvent(&event);
        keyStateMovePaddles(&nearPaddle.y, &farPaddle.y);
        ball.adjust(nearPaddle.y, farPaddle.y);
        updateScreen(renderer, &ball.rec, &nearPaddle, &farPaddle);//const pointer

        int32_t const toDelay = int32_t(MaxTickDelay) - int32_t(SDL_GetTicks() - startTicks);
        if (toDelay >= 0)
            SDL_Delay(toDelay);
    }
}
