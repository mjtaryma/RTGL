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

#include "Common.h"
#include "Const.h"
#include "UserFunction.h"

#include <filesystem>
#include <optional>
#include <span>
#include <vector>

struct ktxTexture;

namespace RTGL1
{

// Loading images from files.
class ImageLoader final
{
public:
    struct ResultInfo
    {
        size_t         levelOffsets[ MAX_PREGENERATED_MIPMAP_LEVELS ];
        size_t         levelSizes[ MAX_PREGENERATED_MIPMAP_LEVELS ];
        uint32_t       levelCount;
        bool           isPregenerated;
        const uint8_t* pData;
        size_t         dataSize;
        RgExtent2D     baseSize;
        VkFormat       format;
    };

    struct LayeredResultInfo
    {
        std::vector< const uint8_t* > layerData;
        size_t                        dataSize;
        RgExtent2D                    baseSize;
        VkFormat                      format;
    };

public:
    ImageLoader() = default;
    ~ImageLoader();

    ImageLoader( const ImageLoader& other )                = delete;
    ImageLoader( ImageLoader&& other ) noexcept            = delete;
    ImageLoader& operator=( const ImageLoader& other )     = delete;
    ImageLoader& operator=( ImageLoader&& other ) noexcept = delete;

    std::optional< ResultInfo >        Load( const std::filesystem::path& path );
    std::optional< LayeredResultInfo > LoadLayered( const std::filesystem::path& path );

    // Must be called after using the loaded data to free the allocated memory
    void FreeLoaded();

    static auto GetExtensions()
    {
        static const char* arr[] = { ".ktx2" };
        return std::span( arr );
    }
    static auto GetFolder() { return TEXTURES_FOLDER; }

private:
    bool LoadTextureFile( const std::filesystem::path& path, ktxTexture** ppTexture );

private:
    std::vector< ktxTexture* > loadedImages;
};

}