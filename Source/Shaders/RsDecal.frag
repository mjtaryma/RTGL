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

#version 460

#define FRAMEBUF_IGNORE_ATTACHMENTS
#define DESC_SET_GLOBAL_UNIFORM 0
#define DESC_SET_FRAMEBUFFERS   1
#define DESC_SET_TEXTURES       2
#define DESC_SET_DECALS         3
#include "ShaderCommonGLSLFunc.h"
#include "Random.h"

layout( location = 0 ) flat in uint instanceIndex;

layout( location = 0 ) out vec4 out_albedo;
layout( location = 1 ) out uint out_normal;
layout( location = 2 ) out vec3 out_screenEmission;

void main()
{
    const ivec2 pix = getCheckerboardPix( ivec2( gl_FragCoord.xy ) );

    const ShDecalInstance decal = decalInstances[ instanceIndex ];

    const vec3 worldPosition = texelFetch( framebufSurfacePosition_Sampler, pix, 0 ).xyz;
    const mat4 worldToLocal  = inverse( decal.transform ); // TODO: on CPU
    const vec4 localPosition = worldToLocal * vec4( worldPosition, 1.0 );

    // if not inside [-0.5, 0.5] box
    if( any( greaterThan( abs( localPosition.xyz ), vec3( 0.5 ) ) ) )
    {
        discard;
    }

    // Z points from surface to outside
    const vec2 texCoord = localPosition.xy + 0.5;

    {
        out_albedo = unpackUintColor( decal.packedColor ) *
                     getTextureSample( decal.textureAlbedoAlpha, texCoord );
    }

    if( decal.textureNormal != MATERIAL_NO_TEXTURE )
    {
        const vec3 underlyingNormal = texelFetchNormal( pix );

        mat3 basis = getONB( underlyingNormal );

        vec2 nmap = getTextureSample( decal.textureNormal, texCoord ).xy;
        nmap.xy   = nmap.xy * 2.0 - vec2( 1.0 );

        out_normal =
            encodeNormal( safeNormalize( basis[ 0 ] * nmap.x + basis[ 1 ] * nmap.y + basis[ 2 ] ) );
    }
    else
    {
        out_normal = texelFetchEncNormal( pix );
    }

    {
        const vec4  baseColor       = unpackUintColor( decal.packedColor );
        const uint  textureEmissive = decal.textureEmissive_emissiveMult >> 16;
        const float emissiveMult = float( decal.textureEmissive_emissiveMult & 0xFFFF ) / 65535.f;

        vec3 ldrEmis;
        if( textureEmissive != MATERIAL_NO_TEXTURE )
        {
            ldrEmis = baseColor.rgb *
                      getTextureSample( textureEmissive, texCoord ).rgb;
        }
        else
        {
            ldrEmis = out_albedo.rgb;
        }
        ldrEmis *= emissiveMult * baseColor.a;

        out_screenEmission = ldrEmis;
    }
}