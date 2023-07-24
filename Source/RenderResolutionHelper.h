// Copyright (c) 2020-2021 Sultim Tsyrendashiev
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

#include <cassert>
#include <cmath>

#include "DLSS.h"
#include "RgException.h"
#include "ResolutionState.h"

namespace RTGL1
{

class RenderResolutionHelper
{
public:
    RenderResolutionHelper()  = default;
    ~RenderResolutionHelper() = default;

    RenderResolutionHelper( const RenderResolutionHelper& other )                = delete;
    RenderResolutionHelper( RenderResolutionHelper&& other ) noexcept            = delete;
    RenderResolutionHelper& operator=( const RenderResolutionHelper& other )     = delete;
    RenderResolutionHelper& operator=( RenderResolutionHelper&& other ) noexcept = delete;

    void Setup( const RgDrawFrameRenderResolutionParams& params,
                uint32_t                                 windowWidth,
                uint32_t                                 windowHeight,
                const std::shared_ptr< DLSS >&           dlss )
    {
        renderWidth  = windowWidth;
        renderHeight = windowHeight;

        upscaledWidth  = windowWidth;
        upscaledHeight = windowHeight;

        upscaleTechnique = params.upscaleTechnique;
        sharpenTechnique = params.sharpenTechnique;
        resolutionMode   = params.resolutionMode;

        // check for correct values
        {
            switch( upscaleTechnique )
            {
                case RG_RENDER_UPSCALE_TECHNIQUE_NEAREST:
                case RG_RENDER_UPSCALE_TECHNIQUE_LINEAR:
                case RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2:
                case RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS: break;
                default:
                    throw RgException(
                        RG_RESULT_WRONG_FUNCTION_ARGUMENT,
                        "RgDrawFrameRenderResolutionParams::upscaleTechnique is incorrect" );
            }
            switch( sharpenTechnique )
            {
                case RG_RENDER_SHARPEN_TECHNIQUE_NONE:
                case RG_RENDER_SHARPEN_TECHNIQUE_NAIVE:
                case RG_RENDER_SHARPEN_TECHNIQUE_AMD_CAS: break;
                default:
                    throw RgException(
                        RG_RESULT_WRONG_FUNCTION_ARGUMENT,
                        "RgDrawFrameRenderResolutionParams::sharpenTechnique is incorrect" );
            }
            switch( resolutionMode )
            {
                case RG_RENDER_RESOLUTION_MODE_CUSTOM:
                case RG_RENDER_RESOLUTION_MODE_ULTRA_PERFORMANCE:
                case RG_RENDER_RESOLUTION_MODE_PERFORMANCE:
                case RG_RENDER_RESOLUTION_MODE_BALANCED:
                case RG_RENDER_RESOLUTION_MODE_QUALITY:
                case RG_RENDER_RESOLUTION_MODE_ULTRA_QUALITY: break;
                default:
                    throw RgException(
                        RG_RESULT_WRONG_FUNCTION_ARGUMENT,
                        "RgDrawFrameRenderResolutionParams::resolutionMode is incorrect" );
            }
        }


        if( upscaleTechnique == RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2 )
        {
            if( resolutionMode == RG_RENDER_RESOLUTION_MODE_ULTRA_QUALITY )
            {
                resolutionMode = RG_RENDER_RESOLUTION_MODE_QUALITY;
                assert( 0 && "Ultra quality should not be used with FSR2" );
            }

            if( resolutionMode == RG_RENDER_RESOLUTION_MODE_CUSTOM )
            {
                renderWidth  = params.customRenderSize.width;
                renderHeight = params.customRenderSize.height;
            }
            else
            {
                float div = 1.0f;

                switch( resolutionMode )
                {
                    case RG_RENDER_RESOLUTION_MODE_ULTRA_PERFORMANCE: div = 3.0f; break;
                    case RG_RENDER_RESOLUTION_MODE_PERFORMANCE: div = 2.0f; break;
                    case RG_RENDER_RESOLUTION_MODE_BALANCED: div = 1.7f; break;
                    case RG_RENDER_RESOLUTION_MODE_QUALITY: div = 1.5f; break;
                    default: assert( 0 ); break;
                }

                renderWidth = static_cast< uint32_t >( static_cast< float >( windowWidth ) / div );
                renderHeight =
                    static_cast< uint32_t >( static_cast< float >( windowHeight ) / div );
            }
        }
        else if( upscaleTechnique == RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS )
        {
            if( resolutionMode == RG_RENDER_RESOLUTION_MODE_CUSTOM )
            {
                renderWidth  = params.customRenderSize.width;
                renderHeight = params.customRenderSize.height;
            }
            else
            {
                dlss->GetOptimalSettings(
                    windowWidth, windowHeight, resolutionMode, &renderWidth, &renderHeight );

                // ultra quality returns (0,0)
                if( renderWidth == 0 || renderHeight == 0 )
                {
                    renderWidth  = windowWidth;
                    renderHeight = windowHeight;
                }
            }
        }
        else
        {
            if( resolutionMode == RG_RENDER_RESOLUTION_MODE_CUSTOM )
            {
                renderWidth  = params.customRenderSize.width;
                renderHeight = params.customRenderSize.height;
            }
        }
    }

    float GetMipLodBias( float nativeBias = 0.0f ) const
    {
        // softer if none
        if( !IsUpscaleEnabled() )
        {
            return nativeBias;
        }

        // DLSS Programming Guide, Section 3.5
        float ratio = float( Width() ) / float( UpscaledWidth() );
        float bias  = nativeBias + std::log2( std::max( 0.01f, ratio ) ) - 1.0f;

        return bias;
    }

    // Render width always must be even for checkerboarding!
    uint32_t Width() const { return renderWidth + renderWidth % 2; }
    uint32_t Height() const { return renderHeight; }

    uint32_t UpscaledWidth() const { return upscaledWidth; }
    uint32_t UpscaledHeight() const { return upscaledHeight; }

    bool IsAmdFsr2Enabled() const
    {
        return upscaleTechnique == RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2;
    }
    bool IsNvDlssEnabled() const
    {
        return upscaleTechnique == RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS;
    }
    bool IsUpscaleEnabled() const { return IsAmdFsr2Enabled() || IsNvDlssEnabled(); }

    float GetAmdFsrSharpness() const { return 1.0f; } // 0.0 - max, 1.0 - min

    bool IsCASInsideFSR2() const
    {
        return upscaleTechnique == RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2 &&
               sharpenTechnique == RG_RENDER_SHARPEN_TECHNIQUE_AMD_CAS;
    }

    // For the additional sharpening pass
    bool IsDedicatedSharpeningEnabled() const
    {
        return IsCASInsideFSR2() ? false : sharpenTechnique != RG_RENDER_SHARPEN_TECHNIQUE_NONE;
    }
    RgRenderSharpenTechnique GetSharpeningTechnique() const { return sharpenTechnique; }
    float                    GetSharpeningIntensity() const { return 1.0f; }

    VkFilter GetBlitFilter() const
    {
        return upscaleTechnique == RG_RENDER_UPSCALE_TECHNIQUE_NEAREST ? VK_FILTER_NEAREST
                                                                       : VK_FILTER_LINEAR;
    }

    // RgRenderResolutionMode   GetResolutionMode()      const { return resolutionMode; }

    ResolutionState GetResolutionState() const
    {
        assert( Width() % 2 == 0 );
        return ResolutionState{ Width(), Height(), UpscaledWidth(), UpscaledHeight() };
    }

private:
    uint32_t renderWidth  = 0;
    uint32_t renderHeight = 0;

    uint32_t upscaledWidth  = 0;
    uint32_t upscaledHeight = 0;

    RgRenderUpscaleTechnique upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_LINEAR;
    RgRenderSharpenTechnique sharpenTechnique = RG_RENDER_SHARPEN_TECHNIQUE_NONE;
    RgRenderResolutionMode   resolutionMode   = RG_RENDER_RESOLUTION_MODE_CUSTOM;
};

}