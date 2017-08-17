#ifndef PONG_H
#define PONG_H

#include <SDL2/SDL.h>

#include <stdint.h>

void loop(SDL_Renderer* renderer);

enum:int{MaxTickDelay=1000/60};

enum:int
{
    ScreenW=640, ScreenH=480,//determine all others. detect better dimensions at runtime? also consider fullscreen

    BallSide=ScreenW/40, PaddleW=ScreenW/80, PaddleHeight=ScreenH/6,//ball and paddle dimensions

    NearPaddleX = ScreenW/80, FarPaddleX = ScreenW - NearPaddleX - PaddleW, PaddleInitY = ScreenH/2 - PaddleHeight/2,//paddle positions

    PaddleMaxY = ScreenH - PaddleHeight,//paddle y bound

    BallMaxY = ScreenH - BallSide,//ball y bound

    FarGoalX = FarPaddleX - BallSide, NearGoalX = NearPaddleX + PaddleW,//ball goal boundaries

    PaddleStep = ScreenH/60//paddle step distance
};

//determine good non-fullscreen dimensions for different systems?
//surfaces vs rendering textures?

struct Ball
{
    SDL_Rect rec;
    int xvel;
    int yvel;
    uint32_t prng32_state;

    Ball();
    void adjust(int closePadY, int farPadY);
    void reset();
};

#endif // PONG_H
