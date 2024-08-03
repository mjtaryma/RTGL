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

#pragma once

#include "EffectSimple.h"

namespace RTGL1
{

struct EffectRadialBlur_PushConst
{
};

struct EffectRadialBlur final : EffectSimple< EffectRadialBlur_PushConst, "EffectRadialBlur" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgPostEffectRadialBlur* params )
    {
        if( params == nullptr )
        {
            return SetupNull();
        }

        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectChromaticAberration_PushConst
{
    float intensity;
};

struct EffectChromaticAberration final
    : EffectSimple< EffectChromaticAberration_PushConst, "EffectChromaticAberration" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments&    args,
                const RgPostEffectChromaticAberration* params )
    {
        if( params == nullptr || params->intensity <= 0.0f )
        {
            return SetupNull();
        }

        GetPush().intensity = params->intensity;
        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectInverseBW final : EffectSimple< EmptyPushConst, "EffectInverseBW" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments&     args,
                const RgPostEffectInverseBlackAndWhite* params )
    {
        if( params == nullptr )
        {
            return SetupNull();
        }

        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectDistortedSides final : EffectSimple< EmptyPushConst, "EffectDistortedSides" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgPostEffectDistortedSides* params )
    {
        if( params == nullptr )
        {
            return SetupNull();
        }

        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectColorTint_PushConst
{
    float intensity;
    float r, g, b;
};

struct EffectColorTint final : EffectSimple< EffectColorTint_PushConst, "EffectColorTint" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgPostEffectColorTint* params )
    {
        if( params == nullptr )
        {
            return SetupNull();
        }

        GetPush().intensity = params->intensity;
        GetPush().r         = params->color.data[ 0 ];
        GetPush().g         = params->color.data[ 1 ];
        GetPush().b         = params->color.data[ 2 ];
        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectTeleport final : EffectSimple< EmptyPushConst, "EffectTeleport" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgPostEffectTeleport* params )
    {
        if( params == nullptr )
        {
            return SetupNull();
        }

        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectHueShift final : EffectSimple< EmptyPushConst, "EffectHueShift" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgPostEffectHueShift* params )
    {
        if( params == nullptr )
        {
            return SetupNull();
        }

        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectNightVision final : EffectSimple< EmptyPushConst, "EffectNightVision" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgPostEffectNightVision* params )
    {
        if( params == nullptr )
        {
            return SetupNull();
        }

        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectCrtDemodulateEncode final : EffectSimple< EmptyPushConst, "EffectCrtDemodulateEncode" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args )
    {
        return EffectSimple::Setup( args, true, 0, 0 );
    }
};

struct EffectCrtDecode final : EffectSimple< EmptyPushConst, "EffectCrtDecode" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args )
    {
        return EffectSimple::Setup( args, true, 0, 0 );
    }
};


// ------------------ //


struct EffectWaves_PushConst
{
    float amplitude;
    float speed;
    float multX;
};

struct EffectWaves final : EffectSimple< EffectWaves_PushConst, "EffectWaves" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgPostEffectWaves* params )
    {
        if( params == nullptr || params->amplitude <= 0.0f )
        {
            return SetupNull();
        }

        GetPush() = EffectWaves_PushConst{
            .amplitude = params->amplitude,
            .speed     = params->speed,
            .multX     = params->xMultiplier,
        };

        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectVHS_PushConst
{
    float intensity;
};

struct EffectVHS final : EffectSimple< EffectVHS_PushConst, "EffectVHS" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgPostEffectVHS* params )
    {
        if( params == nullptr || params->intensity <= 0.0f )
        {
            return SetupNull();
        }

        GetPush().intensity = params->intensity;
        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectDither_PushConst
{
    float intensity;
};

struct EffectDither final : EffectSimple< EffectDither_PushConst, "EffectDither" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgPostEffectDither* params )
    {
        if( params == nullptr || params->intensity <= 0.0f )
        {
            return SetupNull();
        }

        GetPush().intensity = params->intensity;
        return EffectSimple::Setup(
            args, params->isActive, params->transitionDurationIn, params->transitionDurationOut );
    }
};


// ------------------ //


struct EffectHDRPrepare_PushConst
{
    float crosstalk_r;
    float crosstalk_g;
    float crosstalk_b;
    float hdrBrightnessMult;
};

struct EffectHDRPrepare final : EffectSimple< EffectHDRPrepare_PushConst, "EffectHDRPrepare" >
{
    using EffectSimple::EffectSimple;

    bool Setup( const CommonnlyUsedEffectArguments& args, const RgDrawFrameTonemappingParams& tnmp )
    {
        GetPush() = EffectHDRPrepare_PushConst{
            .crosstalk_r       = tnmp.crosstalk.data[ 0 ],
            .crosstalk_g       = tnmp.crosstalk.data[ 1 ],
            .crosstalk_b       = tnmp.crosstalk.data[ 2 ],
            .hdrBrightnessMult = tnmp.hdrBrightness,
        };
        return EffectSimple::Setup( args, true, 0, 0 );
    }
};

}