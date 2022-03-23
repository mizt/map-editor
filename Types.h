enum Type {
    RGB = 0,
    MAP = 1
};

#define MAP_SCALE 8.0
#define MAP_OFFSET ((float)(0x5555))

#define STAGE_WIDTH 1600
#define STAGE_HEIGHT 900

#define FOOTER_HEIGHT 32
#define WHITE 0xFFFFFFFF
#define BLACK 0xFF000000

#define CLAMP(x,min,max) (((x)<(min))?(min):(((x)>(max))?(max):(x)))
