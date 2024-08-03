// Copyright (c) 2021 Sultim Tsyrendashiev
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

layout( location = 0 ) in vec4 vertColor;
layout( location = 1 ) in vec2 vertTexCoord;

layout( location = 0 ) out vec4 outColor;
#if !RS_WORLD_INL_CLASSIC
layout( location = 1 ) out vec3 outScreenEmission;
layout( location = 2 ) out float outReactivity;
#endif

#define DESC_SET_TEXTURES       0
#define DESC_SET_GLOBAL_UNIFORM 1
#define DESC_SET_TONEMAPPING    2
#define DESC_SET_VOLUMETRIC     3
#include "ShaderCommonGLSLFunc.h"
#include "Exposure.h"
#include "Volumetric.h"

layout( push_constant ) uniform RasterizerFrag_BT
{
    layout( offset = 64 ) uint  packedColor;
    layout( offset = 68 ) uint  textureIndex;
    layout( offset = 72 ) uint  emissiveTextureIndex;
    layout( offset = 76 ) float emissiveMult;
}
rasterizerFragInfo;

layout( constant_id = 0 ) const uint alphaTest       = 0;
layout( constant_id = 1 ) const uint isSkyVisibility = 0;

#define ALPHA_THRESHOLD 0.5

vec4 baseColor()
{
    return vertColor * unpackUintColor( rasterizerFragInfo.packedColor );
}

void main()
{
#if RS_WORLD_INL_CLASSIC
    if( isSkyVisibility != 0 )
    {
        discard;
        return;
    }
#endif

    vec4 ldrColor = baseColor() * getTextureSample( rasterizerFragInfo.textureIndex, vertTexCoord );
    outColor      = ldrColor;

#if !RS_WORLD_INL_CLASSIC
    outReactivity = 0.9 * ldrColor.a;
#endif

#if !RS_WORLD_INL_CLASSIC
    {
#if ILLUMINATION_VOLUME
        if( globalUniform.illumVolumeEnable != 0 )
        {
            vec4 ndc = vec4( gl_FragCoord.xyz, 1.0 );
            ndc.xy /= vec2( globalUniform.renderWidth, globalUniform.renderHeight );
            ndc.xy = ndc.xy * 2.0 - 1.0;

            vec4 worldpos = globalUniform.invView * globalUniform.invProjection * ndc;
            worldpos.xyz /= worldpos.w;

            vec3 sp = volume_toSamplePosition_T(
                worldpos.xyz, globalUniform.volumeViewProj, globalUniform.cameraPosition.xyz );
            vec3 illum = textureLod( g_illuminationVolume_Sampler, sp, 0.0 ).rgb;

            outColor.rgb *= illum;
        }
        else
#endif
        {
            outColor.rgb *= max( vec3( 1 ), tonemapping.avgLuminance );
        }
    }
#endif

    {
        vec3 ldrEmis;
        if( rasterizerFragInfo.emissiveTextureIndex != MATERIAL_NO_TEXTURE )
        {
            ldrEmis = baseColor().rgb *
                      getTextureSample( rasterizerFragInfo.emissiveTextureIndex, vertTexCoord ).rgb;
        }
        else
        {
            ldrEmis = ldrColor.rgb;
        }
        ldrEmis *= rasterizerFragInfo.emissiveMult;

#if RS_WORLD_INL_CLASSIC
        outColor.rgb += ldrEmis * ldrColor.a * globalUniform.emissionMaxScreenColor;
#else
        outScreenEmission = ldrEmis;
#endif
    }

    if( alphaTest != 0 )
    {
        if( outColor.a < ALPHA_THRESHOLD )
        {
            discard;
        }
    }
}