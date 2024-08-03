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

#include "SwapchainPass.h"

RTGL1::SwapchainPass::SwapchainPass( VkDevice                    _device,
                                     VkPipelineLayout            _pipelineLayout,
                                     const ShaderManager&        _shaderManager,
                                     const RgInstanceCreateInfo& _instanceInfo )
    : device{ _device }
    , swapchainRenderPass{ VK_NULL_HANDLE }
    , swapchainRenderPass_hudOnly{ VK_NULL_HANDLE }
    , fbPing{}
    , fbPong{}
    , hudOnly{}
{
    assert( ShFramebuffers_Formats[ FB_IMAGE_INDEX_UPSCALED_PING ] ==
            ShFramebuffers_Formats[ FB_IMAGE_INDEX_UPSCALED_PONG ] );
    swapchainRenderPass =
        CreateSwapchainRenderPass( ShFramebuffers_Formats[ FB_IMAGE_INDEX_UPSCALED_PING ], false );

    swapchainRenderPass_hudOnly =
        CreateSwapchainRenderPass( ShFramebuffers_Formats[ FB_IMAGE_INDEX_HUD_ONLY ], true );

    swapchainPipelines =
        std::make_shared< RasterizerPipelines >( device,
                                                 _pipelineLayout,
                                                 swapchainRenderPass,
                                                 _shaderManager,
                                                 "VertDefault",
                                                 "FragSwapchain",
                                                 false,
                                                 _instanceInfo.rasterizedVertexColorGamma );
    swapchainPipelines_hudOnly =
        std::make_shared< RasterizerPipelines >( device,
                                                 _pipelineLayout,
                                                 swapchainRenderPass_hudOnly,
                                                 _shaderManager,
                                                 "VertDefault",
                                                 "FragSwapchain",
                                                 false,
                                                 _instanceInfo.rasterizedVertexColorGamma );
}

RTGL1::SwapchainPass::~SwapchainPass()
{
    vkDestroyRenderPass( device, swapchainRenderPass, nullptr );
    vkDestroyRenderPass( device, swapchainRenderPass_hudOnly, nullptr );
    DestroyFramebuffers();
}

void RTGL1::SwapchainPass::CreateFramebuffers( uint32_t      newSwapchainWidth,
                                               uint32_t      newSwapchainHeight,
                                               Framebuffers& storageFramebuffers )
{
    assert( fbPing == VK_NULL_HANDLE && fbPong == VK_NULL_HANDLE && hudOnly == VK_NULL_HANDLE );

    // ensure that no FRAMEBUF_FLAGS_STORE_PREV, to not create VkFramebuffer for each frameIndex
    assert( storageFramebuffers.GetImageView( FB_IMAGE_INDEX_UPSCALED_PING, 0 ) ==
            storageFramebuffers.GetImageView( FB_IMAGE_INDEX_UPSCALED_PING, 1 ) );
    assert( storageFramebuffers.GetImageView( FB_IMAGE_INDEX_UPSCALED_PONG, 0 ) ==
            storageFramebuffers.GetImageView( FB_IMAGE_INDEX_UPSCALED_PONG, 1 ) );
    assert( storageFramebuffers.GetImageView( FB_IMAGE_INDEX_HUD_ONLY, 0 ) ==
            storageFramebuffers.GetImageView( FB_IMAGE_INDEX_HUD_ONLY, 1 ) );

    auto l_make = []( VkDevice     vkdevice,
                      VkRenderPass renderPass,
                      VkImageView  view,
                      uint32_t     width,
                      uint32_t     height,
                      const char*  name ) {
        auto info = VkFramebufferCreateInfo{
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = renderPass,
            .attachmentCount = 1,
            .pAttachments    = &view,
            .width           = width,
            .height          = height,
            .layers          = 1,
        };

        VkFramebuffer fb = VK_NULL_HANDLE;
        VkResult      r  = vkCreateFramebuffer( vkdevice, &info, nullptr, &fb );
        VK_CHECKERROR( r );
        SET_DEBUG_NAME( vkdevice, fb, VK_OBJECT_TYPE_FRAMEBUFFER, name );
        return fb;
    };

    fbPing = l_make( device,
                     swapchainRenderPass,
                     storageFramebuffers.GetImageView( FB_IMAGE_INDEX_UPSCALED_PING, 0 ),
                     newSwapchainWidth,
                     newSwapchainHeight,
                     "Rasterizer swapchain ping framebuffer" );

    fbPong = l_make( device,
                     swapchainRenderPass,
                     storageFramebuffers.GetImageView( FB_IMAGE_INDEX_UPSCALED_PONG, 0 ),
                     newSwapchainWidth,
                     newSwapchainHeight,
                     "Rasterizer swapchain pong framebuffer" );

    // TODO: alloc hud only only when needed (framegen = on
    hudOnly = l_make( device,
                      swapchainRenderPass_hudOnly,
                      storageFramebuffers.GetImageView( FB_IMAGE_INDEX_HUD_ONLY, 0 ),
                      newSwapchainWidth,
                      newSwapchainHeight,
                      "Rasterizer swapchain hud-only framebuffer" );
}

void RTGL1::SwapchainPass::DestroyFramebuffers()
{
    if( fbPing != VK_NULL_HANDLE )
    {
        vkDestroyFramebuffer( device, fbPing, nullptr );
        fbPing = VK_NULL_HANDLE;
    }
    if( fbPong != VK_NULL_HANDLE )
    {
        vkDestroyFramebuffer( device, fbPong, nullptr );
        fbPong = VK_NULL_HANDLE;
    }
    if( hudOnly != VK_NULL_HANDLE )
    {
        vkDestroyFramebuffer( device, hudOnly, nullptr );
        hudOnly = VK_NULL_HANDLE;
    }
}

void RTGL1::SwapchainPass::OnShaderReload( const ShaderManager* shaderManager )
{
    swapchainPipelines->OnShaderReload( shaderManager );
    swapchainPipelines_hudOnly->OnShaderReload( shaderManager );
}

VkRenderPass RTGL1::SwapchainPass::GetSwapchainRenderPass(
    FramebufferImageIndex framebufIndex ) const
{
    switch( framebufIndex )
    {
        case FB_IMAGE_INDEX_UPSCALED_PING:
        case FB_IMAGE_INDEX_UPSCALED_PONG:
            assert( swapchainRenderPass );
            return swapchainRenderPass;
        case FB_IMAGE_INDEX_HUD_ONLY:
            assert( swapchainRenderPass_hudOnly );
            return swapchainRenderPass_hudOnly;
        default: assert( 0 ); return VK_NULL_HANDLE;
    }
}

RTGL1::RasterizerPipelines* RTGL1::SwapchainPass::GetSwapchainPipelines(
    FramebufferImageIndex framebufIndex ) const
{
    switch( framebufIndex )
    {
        case FB_IMAGE_INDEX_UPSCALED_PING:
        case FB_IMAGE_INDEX_UPSCALED_PONG: assert( swapchainPipelines ); return swapchainPipelines.get();
        case FB_IMAGE_INDEX_HUD_ONLY:
            assert( swapchainPipelines_hudOnly );
            return swapchainPipelines_hudOnly.get();
        default: assert( 0 ); return nullptr;
    }
}

VkFramebuffer RTGL1::SwapchainPass::GetSwapchainFramebuffer(
    FramebufferImageIndex framebufIndex ) const
{
    switch( framebufIndex )
    {
        case FB_IMAGE_INDEX_UPSCALED_PING: assert( fbPing ); return fbPing;
        case FB_IMAGE_INDEX_UPSCALED_PONG: assert( fbPong ); return fbPong;
        case FB_IMAGE_INDEX_HUD_ONLY: assert( hudOnly ); return hudOnly;
        default: assert( 0 ); return VK_NULL_HANDLE;
    }
}

VkRenderPass RTGL1::SwapchainPass::CreateSwapchainRenderPass( VkFormat format, bool clear ) const
{
    VkAttachmentDescription colorAttch = {
        .format         = format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_GENERAL,
        .finalLayout    = VK_IMAGE_LAYOUT_GENERAL,
    };

    VkAttachmentReference colorRef = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &colorRef,
        .pDepthStencilAttachment = nullptr,
    };

    VkSubpassDependency dependency = {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo passInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments    = &colorAttch,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 1,
        .pDependencies   = &dependency,
    };

    VkRenderPass renderPass;
    VkResult     r = vkCreateRenderPass( device, &passInfo, nullptr, &renderPass );

    VK_CHECKERROR( r );
    SET_DEBUG_NAME(
        device, renderPass, VK_OBJECT_TYPE_RENDER_PASS, "Rasterizer swapchain render pass" );
    return renderPass;
}
