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

#include <vector>
#include <string>

#include "RTGL1/RTGL1.h"
#include "Common.h"
#include "Material.h"
#include "MemoryAllocator.h"
#include "SamplerManager.h"
#include "TextureDescriptors.h"
#include "CubemapUploader.h"
#include "CommandBufferManager.h"
#include "ImageLoader.h"

namespace RTGL1
{


// Was in the main API
typedef struct RgOriginalCubemapInfo
{
    RgStructureType sType;
    void*           pNext;
    const char*     pTextureName;
    // R8G8B8A8 pixel data. Each must be (sideSize * sideSize * 4) bytes.
    const void*     pPixelsPositiveX;
    const void*     pPixelsNegativeX;
    const void*     pPixelsPositiveY;
    const void*     pPixelsNegativeY;
    const void*     pPixelsPositiveZ;
    const void*     pPixelsNegativeZ;
    uint32_t        sideSize;
} RgOriginalCubemapInfo;


class CubemapManager
{
public:
    CubemapManager( VkDevice                           device,
                    std::shared_ptr< MemoryAllocator > allocator,
                    std::shared_ptr< SamplerManager >  samplerManager,
                    CommandBufferManager&              cmdManager );
    ~CubemapManager();

    CubemapManager( const CubemapManager& other )                = delete;
    CubemapManager( CubemapManager&& other ) noexcept            = delete;
    CubemapManager& operator=( const CubemapManager& other )     = delete;
    CubemapManager& operator=( CubemapManager&& other ) noexcept = delete;


    bool TryCreateCubemap( VkCommandBuffer              cmd,
                           uint32_t                     frameIndex,
                           const RgOriginalCubemapInfo& info,
                           const std::filesystem::path& folder );
    bool TryDestroyCubemap( uint32_t frameIndex, const char* pTextureName );


    uint32_t TryGetDescriptorIndex( const char* pTextureName );


    VkDescriptorSetLayout GetDescSetLayout() const;
    VkDescriptorSet       GetDescSet( uint32_t frameIndex ) const;

    void PrepareForFrame( uint32_t frameIndex );
    void SubmitDescriptors( uint32_t frameIndex );

    bool IsCubemapValid( uint32_t cubemapIndex ) const;

private:
    void CreateEmptyCubemap( VkCommandBuffer cmd );
    void AddForDeletion( uint32_t frameIndex, Texture& txd );

private:
    VkDevice device;

    std::shared_ptr< MemoryAllocator >    allocator;
    std::shared_ptr< ImageLoader >        imageLoader;
    std::shared_ptr< SamplerManager >     samplerManager;
    std::shared_ptr< TextureDescriptors > cubemapDesc;
    std::shared_ptr< CubemapUploader >    cubemapUploader;

    rgl::string_map< Texture >                 cubemaps;
    std::vector< Texture >                     cubemapsToDestroy[ MAX_FRAMES_IN_FLIGHT ];
};

}