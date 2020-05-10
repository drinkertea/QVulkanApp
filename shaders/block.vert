#version 450

#define FRONT  0
#define BACK   1
#define LEFT   2
#define RIGHT  3
#define TOP    4
#define BOTTOM 5

#define TOP_LEFT  0
#define BOT_LEFT  1
#define BOT_RIGHT 2
#define TOP_RIGHT 3

layout (location = 0) in int cornerIndex;

layout (location = 1) in vec3 instancePos;
layout (location = 2) in int instanceTexIndex;
layout (location = 3) in int faceIndex;

layout (std140, push_constant) uniform PushConsts 
{
	mat4 mvp;
} pushConsts;

layout (location = 0) out vec3 outUV;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    switch (cornerIndex)
    {
        case TOP_LEFT : outUV = vec3(vec2(1,0), instanceTexIndex); break;
        case BOT_LEFT : outUV = vec3(vec2(0,0), instanceTexIndex); break;
        case BOT_RIGHT: outUV = vec3(vec2(0,1), instanceTexIndex); break;
        case TOP_RIGHT: outUV = vec3(vec2(1,1), instanceTexIndex); break;
    }

    vec3 pos;
    switch (faceIndex)
    {
        case FRONT: switch (cornerIndex)
        {
            case TOP_LEFT : pos = vec3(1, 1, 1); break;
            case BOT_LEFT : pos = vec3(0, 1, 1); break;
            case BOT_RIGHT: pos = vec3(0, 0, 1); break;
            case TOP_RIGHT: pos = vec3(1, 0, 1); break;
        } break;
        case BACK: switch (cornerIndex)
        {
            case TOP_LEFT : pos = vec3(0, 1, 0); break;
            case BOT_LEFT : pos = vec3(1, 1, 0); break;
            case BOT_RIGHT: pos = vec3(1, 0, 0); break;
            case TOP_RIGHT: pos = vec3(0, 0, 0); break;
        } break;
        case LEFT: switch (cornerIndex)
        {
            case TOP_LEFT : pos = vec3(0, 1, 1); break;
            case BOT_LEFT : pos = vec3(0, 1, 0); break;
            case BOT_RIGHT: pos = vec3(0, 0, 0); break;
            case TOP_RIGHT: pos = vec3(0, 0, 1); break;
        } break;
        case RIGHT: switch (cornerIndex)
        {
            case TOP_LEFT : pos = vec3(1, 1, 0); break;
            case BOT_LEFT : pos = vec3(1, 1, 1); break;
            case BOT_RIGHT: pos = vec3(1, 0, 1); break;
            case TOP_RIGHT: pos = vec3(1, 0, 0); break;
        } break;
        case TOP: switch (cornerIndex)
        {
            case TOP_LEFT : pos = vec3(1, 1, 0); break;
            case BOT_LEFT : pos = vec3(0, 1, 0); break;
            case BOT_RIGHT: pos = vec3(0, 1, 1); break;
            case TOP_RIGHT: pos = vec3(1, 1, 1); break;
        } break;
        case BOTTOM: switch (cornerIndex)
        {
            case TOP_LEFT : pos = vec3(1, 0, 1); break;
            case BOT_LEFT : pos = vec3(0, 0, 1); break;
            case BOT_RIGHT: pos = vec3(0, 0, 0); break;
            case TOP_RIGHT: pos = vec3(1, 0, 0); break;
        } break;
    }

    gl_Position = pushConsts.mvp * vec4(pos + instancePos, 1.0);
}
