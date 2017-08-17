# Pong

Uses SDL2. Compile with SDL2 folder in path so `#include <SDL2/SDL.h>` works.

Ball may appear to be teleporting and not smoothly moving, even at 60 fps. Ex: ball at (100,100) frame 2, yvel=8, xvel=8.  
Then frame 3 erases prev image, teleports ball to (108,108) and shows it. How to generate streak effect/image blur? Maybe show
multiple overlapping rects depending on speed, staggered on top of eachother by some amount of pixels(depend on speed?). Can mess
with the color/alpha values of these "shadow" rects.
