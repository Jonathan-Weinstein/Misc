#include "pong.h"

static inline
uint32_t xorshift32(uint32_t x)
{
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

Ball::Ball()
: rec{0, 0, BallSide, BallSide}//x, y, w, h
{
    prng32_state = SDL_GetTicks() | 1u;
    reset();
}

void Ball::reset()
{
    rec.x = (ScreenW - BallSide)/2u;
    rec.y = (ScreenH - BallSide)/2u;

    uint32_t val = xorshift32(prng32_state);
    prng32_state=val;

    static const int bag[4]={-8, -4, 4, 8};//xxx
    //non multiples of 4 may tunnel into paddles slightely?

    xvel = bag[val&3u];//bottom 2 bits
    yvel = bag[val>>30u];//top two bits
}

void Ball::adjust(int nearPadY, int farPadY)
{
    rec.y += yvel;
    if (rec.y<=0 || rec.y>=BallMaxY)//hit ceiling or floor
        yvel= -yvel;

    rec.x+=xvel;
    if (rec.x<=NearGoalX)
    {
        if (rec.y<=nearPadY-BallSide || rec.y>=nearPadY+PaddleHeight)//score
            reset();
        else
            xvel= -xvel;
    }
    else if (rec.x>=FarGoalX)
    {
        if (rec.y<=farPadY-BallSide || rec.y>=farPadY+PaddleHeight)
            reset();
        else
            xvel= -xvel;
    }
}

