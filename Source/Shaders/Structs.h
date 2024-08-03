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

#ifndef STRUCTS_H_
#define STRUCTS_H_

#if !SUPPRESS_TEXLAYERS
#define TEXLAYER_MAX 4
#else
#define TEXLAYER_MAX 1
#endif

struct ShTriangle
{
    mat3    positions;
    mat3    prevPositions;
    
    mat3    normals;
    
    mat3x2  layerTexCoord[ TEXLAYER_MAX ];
    uint    layerColorTextures[ TEXLAYER_MAX ];
    uint    layerColors[ TEXLAYER_MAX ];
    
    uint    vertexColors[ 3 ];
    uint    geometryInstanceFlags;

    float   roughnessDefault;
    float   metallicDefault;
    uint    occlusionRougnessMetallicTexture;   // layerTexCoord[ 0 ]
    
    uint    normalTexture;                      // layerTexCoord[ 0 ]
    uint    heightTexture;                      // layerTexCoord[ 0 ]
    
    float   emissiveMult;
    uint    emissiveTexture;                    // layerTexCoord[ 0 ]
    
    uint    portalIndex;
};

struct ShPayload
{
    vec2    baryCoords;
    uint    instIdAndIndex;
    uint    geomAndPrimIndex;
};

struct ShPayloadShadow
{
    uint    isShadowed;
};

struct ShHitInfo
{
    vec3    albedo;
    float   metallic;
    vec3    normal;
    float   roughness;
    vec3    hitPosition;
    uint    instCustomIndex;
    uint    geometryInstanceFlags;
    uint    portalIndex;
};

#endif // STRUCTS_H_