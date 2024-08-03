// Copyright (c) 2022 Sultim Tsyrendashiev
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#if !defined(EFFECT_SOURCE_IS_PING) && !defined(EFFECT_SOURCE_IS_PONG) 
    #error Define EFFECT_SOURCE_IS_PING or EFFECT_SOURCE_IS_PONG to boolean value
#endif


ivec2 effect_getFramebufSize()
{
    return imageSize(framebufUpscaledPing); // framebufUpscaledPong has the same size
}


vec2 effect_getInverseFramebufSize()
{
    ivec2 sz = effect_getFramebufSize();
    return vec2(1.0 / float(sz.x), 1.0 / float(sz.y));
}


ivec2 effect_clampPix(ivec2 pix)
{
    return clamp(pix, ivec2(0), effect_getFramebufSize() - 1);
}


bool effect_isPixValid(ivec2 pix)
{
    return pix == effect_clampPix(pix);
}


// get UV coords in [0..1] range
vec2 effect_getFramebufUV(ivec2 pix)
{
    return (vec2(pix) + 0.5) * effect_getInverseFramebufSize();
}


// to [-1..1]
vec2 effect_getCenteredFromPix(ivec2 pix)
{
    return effect_getFramebufUV(pix) * 2.0 - 1.0;
}


// from [-1..1]
ivec2 effect_getPixFromCentered(vec2 centered)
{
    return ivec2((centered * 0.5 + 0.5) * effect_getFramebufSize());
}


vec3 effect_loadFromSource(ivec2 pix)
{
    pix = effect_clampPix(pix);

    if (EFFECT_SOURCE_IS_PING)
    {
        return imageLoad(framebufUpscaledPing, pix).rgb;
    }
    else
    {
        return imageLoad(framebufUpscaledPong, pix).rgb;
    }
}


void effect_storeToTarget(const vec3 value, ivec2 pix)
{
    pix = effect_clampPix(pix);

    if (EFFECT_SOURCE_IS_PING)
    {
        imageStore(framebufUpscaledPong, pix, vec4(value, 0.0));
    }
    else
    {
        imageStore(framebufUpscaledPing, pix, vec4(value, 0.0));
    }
}


void effect_storeUnmodifiedToTarget(ivec2 pix)
{
    effect_storeToTarget(effect_loadFromSource(pix), pix);
}


vec3 effect_loadFromSource_Centered(vec2 centered)
{
    return effect_loadFromSource(effect_getPixFromCentered(centered));
}


#ifdef DESC_SET_RANDOM
#include "Random.h"
float effect_getRandomSample(ivec2 pix, uint frameIndex)
{
    return rnd16(getRandomSeed(pix, frameIndex), RANDOM_SALT_POSTEFFECT);
}
#endif
