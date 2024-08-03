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

#ifndef BRDF_H_
#define BRDF_H_

#include "Random.h"


float roughnessSquaredToSpecPower(in float alpha) {
    return max(0.01, 2.0f / (square(alpha) + 1e-4) - 2.0f);
}


// subsurfaceAlbedo -- 0 if all light is absorbed,
//                     1 if no light is absorbed
float evalBRDFLambertian(float subsurfaceAlbedo)
{
    return subsurfaceAlbedo / M_PI;
}

// u1, u2   -- uniform random numbers
vec3 sampleLambertian(const vec3 n, float u1, float u2, out float oneOverPdf)
{
    return sampleOrientedHemisphere(n, u1, u2, oneOverPdf);
}



#define BRDF_MIN_SPECULAR_COLOR 0.04

vec3 getSpecularColor(const vec3 albedo, float metallic)
{
    vec3 minSpec = vec3(BRDF_MIN_SPECULAR_COLOR);
    return mix(minSpec, albedo, metallic);
}

#define AO_ALBEDO_THRESHOLD 0.02

float getMaterialAmbient(const vec3 albedo)
{
    float l = getLuminance(albedo);

    return l > AO_ALBEDO_THRESHOLD ? 
        1.0 :
        1.0 - square((l - AO_ALBEDO_THRESHOLD) / AO_ALBEDO_THRESHOLD);
}

vec3 demodulateSpecular(const vec3 contrib, const vec3 surfSpecularColor)
{
    return contrib / max(vec3(0.01), surfSpecularColor);
}

// nl -- cos between surface normal and light direction
// specularColor -- reflectance color at zero angle
vec3 getFresnelSchlick(float nl, const vec3 specularColor)
{
    return specularColor + (vec3(1.0) - specularColor) * pow(1 - max(nl, 0), 5);
}

float getFresnelSchlick(float n1, float n2, const vec3 V, const vec3 N)
{
    float R0 = (n1 - n2) / (n1 + n2);
    R0 *= R0;

    return mix(R0, 1.0, pow(1.0 - abs(dot(N, V)), 5.0));
}

// GGX distribution - reciprocal
float D_GGX_rcp( float nm, float alpha )
{
#if SHIPPING_HACK
#ifdef FORCE_EVALBRDF_GGX_LOOSE
    // shipping hack: make ggx factor be non-zero, so we can reuse for reprojection,
    // let there be some lag, but at least less noise
    alpha = max( 0.2, alpha );
#endif
#endif

    const float alphaSq = square( alpha );

    nm = max( 0.0, nm );
    return  M_PI * square( nm * nm * ( alphaSq - 1 ) + 1 ) / alphaSq;
}

// GGX distribution
float D_GGX( float nm, float alpha )
{
    return 1.0 / D_GGX_rcp( nm, alpha );
}

// Smith G1 for GGX, Karis' approximation ("Real Shading in Unreal Engine 4")
// ns = dot( macrosurface normal, s ), where s is either v or l
float G1_GGX( float ns, float alpha )
{
    return 2 * ns * safePositiveRcp( ns * ( 2 - alpha ) + alpha );
}

#define MIN_GGX_ROUGHNESS 0.005

// n -- macrosurface normal
// v -- direction to viewer
// l -- direction to light
// alpha -- roughness
vec3 evalBRDFSmithGGX(const vec3 n, const vec3 v, const vec3 l, float alpha, const vec3 specularColor)
{
    alpha = max(alpha, MIN_GGX_ROUGHNESS);

    const float nl = max(dot(n, l), 0);

    if (nl <= 0)
    {
        return vec3(0.0);
    }

    const vec3 F = getFresnelSchlick(nl, specularColor);

    // here, microfacet normal is a half-vector,
    // since we know in which direction l should be reflected
    const vec3  h = normalize( v + l );
    const float D = D_GGX( dot( n, h ), alpha );

    float G2Modif;
    {
        const float nv = max(dot(n, v), 0);

        // approximation for SmithGGX, Hammon ("PBR Diffuse Lighting for GGX+Smith Microsurfaces")
        // inlcudes 1 / (4 * nl * nv)
        G2Modif = 0.5 / mix(2 * nl * nv, nl + nv, alpha);
    }

    return F * G2Modif * D;
}



// "Sampling the GGX Distribution of Visible Normals", Heitz
// v        -- direction to viewer, normal's direction is (0,0,1)
// alpha    -- roughness
// u1, u2   -- uniform random numbers
// output   -- normal sampled with PDF D_v(Ne) = G1(v) * max(0, dot(v, Ne)) * D(Ne) / v.z
vec3 sampleGGXVNDF_Heitz(const vec3 v, float alpha, float u1, float u2, out float oneOverPdf)
{
    alpha = max( alpha, MIN_GGX_ROUGHNESS );

    // fix: avoid grazing angles
    u1 *= 0.98;
    u2 *= 0.98;

    // Section 3.2: transforming the view direction to the hemisphere configuration
    vec3 Vh = normalize(vec3(alpha * v.x, alpha * v.y, v.z));
    
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    const vec3 T1 = lensq > 0 ? vec3(-Vh.y, Vh.x, 0) * inversesqrt(lensq) : vec3(1,0,0);
    const vec3 T2 = cross(Vh, T1);

    // Section 4.2: parameterization of the projected area
    float r = sqrt(u1);    
    float phi = 2.0 * M_PI * u2;    
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

    // Section 4.3: reprojection onto hemisphere
    const vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;
    
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    const vec3 Ne = normalize(vec3(alpha * Nh.x, alpha * Nh.y, max(0.02, Nh.z)));

    {
        // here, macro normal is (0,0,1), so nm=m.z
        const float nm = Ne.z;
        const float D = D_GGX( nm, alpha );

        // here, macro normal is (0,0,1), so nv=v.z
        const float nv = v.z;
        const float G1 = G1_GGX( nv, alpha );

        oneOverPdf = v.z * safePositiveRcp(G1 * max(0, dot(v, Ne)) * D);
    }

    return Ne;
}

// "Bounded VNDF Sampling for Smith–GGX Reflections", Kenta Eto, Yusuke Tokuyoshi
// i        -- direction to viewer, normal's direction is (0,0,1)
// alpha    -- roughness
// u1, u2   -- uniform random numbers
// output   -- normal sampled with PDF D_v(Ne) = G1(v) * max(0, dot(v, Ne)) * D(Ne) / v.z
vec3 sampleGGXVNDF_Bounded(const vec3 i, float alpha, float u1, float u2, out float oneOverPdf)
{
    alpha = clamp( alpha, MIN_GGX_ROUGHNESS, 1.0 );

    // fix: avoid grazing angles
    u1 *= 0.98;
    u2 *= 0.98;

    vec3 m;
    {
        vec3  i_std = normalize( vec3( i.xy * alpha, i.z ) );
        // Sample a spherical cap
        float phi = 2.0 * M_PI * u1;
        float s   = 1.0f + length( vec2( i.x, i.y ) ); // Omit sgn for alpha <=1
        float a2  = alpha * alpha;
        float s2  = s * s;
        float k   = ( 1.0 - a2 ) * s2 / ( s2 + a2 * i.z * i.z ); // Eq. 5
        float b   = i.z > 0 ? k * i_std.z : i_std.z;
        // float z     = mad( 1.0 - u2, 1.0 + b, -b );
        float z        = ( 1.0 - u2 ) * ( 1.0 + b ) - b;
        float sinTheta = sqrt( saturate( 1.0 - z * z ) );
        vec3  o_std    = vec3( sinTheta * cos( phi ), sinTheta * sin( phi ), z );
        // Compute the microfacet normal m
        vec3  m_std = i_std + o_std;
        m           = normalize( vec3( m_std.xy * alpha, m_std.z ) );
    }

    // here, macro normal is (0,0,1), so nm=m.z
    const float nm  = m.z;
    const float ndf_rcp = D_GGX_rcp( nm, alpha );

    {
        vec2  ai   = alpha * i.xy;
        float len2 = dot( ai, ai );
        float t    = sqrt( len2 + i.z * i.z );
        if( i.z >= 0.0 )
        {
            float a  = alpha; // Eq. 6
            float s  = 1.0f + length( vec2( i.x, i.y ) );   // Omit sgn for a <=1
            float a2 = a * a;
            float s2 = s * s;
            float k  = ( 1.0 - a2 ) * s2 / ( s2 + a2 * i.z * i.z ); // Eq. 5

            oneOverPdf = ( 2.0 * ( k * i.z + t ) ) * ndf_rcp; // Eq. 8 * || dm/do ||
        }
        else
        {
            // Numerically stable form of the previous PDF for i.z < 0
            oneOverPdf = ( 2.0 * len2 ) * ndf_rcp / ( t - i.z ); // = Eq. 7 * || dm/do ||
        }
    }

    return m;
}


#define sampleGGXVNDF sampleGGXVNDF_Bounded


// Sample microfacet normal
// n        -- macrosurface normal, world space
// v        -- direction to viewer, world space
// alpha    -- roughness
// u1, u2   -- uniform random numbers
// Check Heitz's paper for the special representation of rendering equation term 
vec3 sampleSmithGGX(const vec3 n, const vec3 v, float alpha, float u1, float u2, out float oneOverPdf)
{
    const mat3 basis = getONB(n);

    // get v in normal's space, basis is orthogonal
    const vec3 ve = transpose(basis) * v;

    // microfacet normal
    const vec3 m = sampleGGXVNDF(ve, alpha, u1, u2, oneOverPdf);

    // reflect viewer dir by a microfacet
    const vec3 l = reflect( -ve, m );
    // reflection jacobian
    oneOverPdf *= 4 * dot( ve, m );

    // back to world space
    return basis * l;
}

#endif // BRDF_H_