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

#pragma once

#include "Common.h"
#include "ShaderManager.h"
#include "Framebuffers.h"
#include "GlobalUniform.h"
#include "Tonemapping.h"

namespace RTGL1
{


class RenderResolutionHelper;
class TextureManager;


class Bloom final : public IShaderDependency
{
public:
    Bloom( VkDevice                        device,
           std::shared_ptr< Framebuffers > framebuffers,
           const ShaderManager&            shaderManager,
           const GlobalUniform&            uniform,
           const TextureManager&           textureManager,
           const Tonemapping&              tonemapping );
    ~Bloom() override;

    Bloom( const Bloom& other )                = delete;
    Bloom( Bloom&& other ) noexcept            = delete;
    Bloom& operator=( const Bloom& other )     = delete;
    Bloom& operator=( Bloom&& other ) noexcept = delete;
    
    FramebufferImageIndex Apply( VkCommandBuffer       cmd,
                                 uint32_t              frameIndex,
                                 const GlobalUniform&  uniform,
                                 const Tonemapping&    tonemapping,
                                 const TextureManager& textureManager,
                                 uint32_t              upscaledWidth,
                                 uint32_t              upscaledHeight,
                                 FramebufferImageIndex inputFramebuf );

    void OnShaderReload( const ShaderManager* shaderManager ) override;

    static VkExtent2D MakeSize( uint32_t              upscaledWidth,
                                uint32_t              upscaledHeight,
                                FramebufferImageIndex index );

private:
    void CreatePipelines( const ShaderManager* shaderManager );
    void CreateStepPipelines( const ShaderManager* shaderManager );
    void CreateApplyPipelines( const ShaderManager* shaderManager );
    void DestroyPipelines();

private:
    static constexpr uint32_t StepCount = 7;

    VkDevice device;

    std::shared_ptr< Framebuffers > framebuffers;

    VkPipelineLayout pipelineLayout;

    VkPipeline downsamplePipelines[ StepCount ];
    VkPipeline upsamplePipelines[ StepCount ];

    VkPipeline preloadPipelines[ 2 ];
    VkPipeline applyPipelines[ 2 ];
};

}