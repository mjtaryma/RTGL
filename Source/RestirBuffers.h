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

#include "MemoryAllocator.h"
#include "IFramebuffersDependency.h"

namespace RTGL1
{
class RestirBuffers : public IFramebuffersDependency
{
public:
    RestirBuffers( VkDevice device, std::shared_ptr< MemoryAllocator > allocator );
    ~RestirBuffers() override;

    RestirBuffers( const RestirBuffers& other )     = delete;
    RestirBuffers( RestirBuffers&& other ) noexcept = delete;
    RestirBuffers&        operator=( const RestirBuffers& other ) = delete;
    RestirBuffers&        operator=( RestirBuffers&& other ) noexcept = delete;

    VkDescriptorSet       GetDescSet( uint32_t frameIndex ) const;
    VkDescriptorSetLayout GetDescSetLayout() const;

    void OnFramebuffersSizeChange( const ResolutionState& resolutionState ) override;

private:
    void CreateBuffers( uint32_t renderWidth, uint32_t renderHeight );
    void DestroyBuffers();

    void CreateDescriptors();
    void UpdateDescriptors();

public:
    struct BufferDef
    {
        VkBuffer       buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

private:
    VkDevice                           device    = {};
    std::shared_ptr< MemoryAllocator > allocator = {};

    VkDescriptorPool                   descPool                         = {};
    VkDescriptorSetLayout              descLayout                       = {};
    VkDescriptorSet                    descSets[ MAX_FRAMES_IN_FLIGHT ] = {};

    BufferDef                          reservoirs[ MAX_FRAMES_IN_FLIGHT ] = {};
};
}
