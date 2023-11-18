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

#include <array>
#include <list>
#include <string>

#include "CommandBufferManager.h"
#include "Common.h"
#include "IFileDependency.h"
#include "ImageLoader.h"
#include "ImageLoaderDev.h"
#include "JsonParser.h"
#include "Material.h"
#include "MemoryAllocator.h"
#include "SamplerManager.h"
#include "TextureDescriptors.h"
#include "TextureOverrides.h"
#include "TextureUploader.h"

namespace RTGL1
{


struct TextureOverrides;


class TextureManager : public IFileDependency
{
public:
    TextureManager( VkDevice                                device,
                    std::shared_ptr< MemoryAllocator >      memAllocator,
                    std::shared_ptr< SamplerManager >       samplerManager,
                    std::shared_ptr< CommandBufferManager > cmdManager,
                    const std::filesystem::path&            waterNormalTexturePath,
                    const std::filesystem::path&            dirtMaskTexturePath,
                    RgTextureSwizzling                      pbrSwizzling,
                    bool                                    forceNormalMapFilterLinear,
                    const LibraryConfig&                    config );
    ~TextureManager() override;

    TextureManager( const TextureManager& other )                = delete;
    TextureManager( TextureManager&& other ) noexcept            = delete;
    TextureManager& operator=( const TextureManager& other )     = delete;
    TextureManager& operator=( TextureManager&& other ) noexcept = delete;

    void PrepareForFrame( uint32_t frameIndex );
    void TryHotReload( VkCommandBuffer cmd, uint32_t frameIndex );

    void SubmitDescriptors( uint32_t                         frameIndex,
                            const RgDrawFrameTexturesParams& texturesParams,
                            bool                             forceUpdateAllDescriptors =
                                false /* true, if mip lod bias was changed, for example */ );

    bool TryCreateMaterial( VkCommandBuffer              cmd,
                            uint32_t                     frameIndex,
                            const RgOriginalTextureInfo& info,
                            const std::filesystem::path& ovrdFolder );

    bool TryCreateImportedMaterial( VkCommandBuffer                     cmd,
                                    uint32_t                            frameIndex,
                                    const std::string&                  materialName,
                                    std::span< std::filesystem::path >  fullPaths,
                                    std::span< SamplerManager::Handle > samplers,
                                    RgTextureSwizzling                  customPbrSwizzling,
                                    bool                                isReplacement );
    void FreeAllImportedMaterials( uint32_t frameIndex, bool freeReplacements );

    bool TryDestroyMaterial( uint32_t frameIndex, const char* materialName );


    VkDescriptorSet       GetDescSet( uint32_t frameIndex ) const;
    VkDescriptorSetLayout GetDescSetLayout() const;

    auto GetWaterNormalTextureIndex() const -> uint32_t;
    auto GetDirtMaskTextureIndex() const -> uint32_t;

    auto GetMaterialTextures( const char* materialName ) const -> MaterialTextures;

    auto GetTexturesForLayers( const RgMeshPrimitiveInfo& primitive ) const
        -> std::array< MaterialTextures, 4 >;

    auto GetColorForLayers( const RgMeshPrimitiveInfo& primitive ) const
        -> std::array< RgColor4DPacked32, 4 >;


    struct ExportResult
    {
        std::string          relativePath;
        RgSamplerAddressMode addressModeU{ RG_SAMPLER_ADDRESS_MODE_REPEAT };
        RgSamplerAddressMode addressModeV{ RG_SAMPLER_ADDRESS_MODE_REPEAT };
    };

    auto ExportMaterialTextures( const char*                  materialName,
                                 const std::filesystem::path& folder,
                                 bool                         overwriteExisting,
                                 const std::filesystem::path* lookupFolder ) const
        -> std::array< ExportResult, TEXTURES_PER_MATERIAL_COUNT >;

    void ExportOriginalMaterialTextures( const std::filesystem::path& folder ) const;

    bool ShouldExportAsExternal( const char* materialName ) const;


    void OnFileChanged( FileType type, const std::filesystem::path& filepath ) override;

private:
    struct Material
    {
        MaterialTextures textures;
        bool             isUpdateable;
    };

private:
    void     CreateEmptyTexture( VkCommandBuffer cmd, uint32_t frameIndex );
    uint32_t CreateWaterNormalTexture( VkCommandBuffer              cmd,
                                       uint32_t                     frameIndex,
                                       const std::filesystem::path& filepath );
    uint32_t CreateDirtMaskTexture( VkCommandBuffer              cmd,
                                    uint32_t                     frameIndex,
                                    const std::filesystem::path& filepath );

    void MakeMaterial( VkCommandBuffer                                  cmd,
                       uint32_t                                         frameIndex,
                       std::string_view                                 materialName,
                       std::span< TextureOverrides >                    ovrd,
                       std::span< SamplerManager::Handle >              samplers,
                       std::span< std::optional< RgTextureSwizzling > > swizzlings );

    uint32_t PrepareTexture( VkCommandBuffer                                 cmd,
                             uint32_t                                        frameIndex,
                             const std::optional< ImageLoader::ResultInfo >& info,
                             SamplerManager::Handle                          samplerHandle,
                             bool                                            useMipmaps,
                             const char*                                     debugName,
                             bool                                            isUpdateable,
                             std::optional< RgTextureSwizzling >             swizzling,
                             std::filesystem::path&&                         filepath,
                             std::vector< Texture >::iterator                targetSlot );

    void DestroyTexture( const Texture& texture );
    void AddToBeDestroyed( uint32_t frameIndex, Texture& texture );

    void InsertMaterial( uint32_t         frameIndex,
                         std::string_view materialName,
                         const Material&  material );
    void DestroyMaterialTextures( uint32_t frameIndex, const Material& material );

    TextureOverrides::Loader AnyImageLoader()
    {
        // prefer raw formats in devmode
        if( isdevmode )
        {
            return std::tuple{
                imageLoaderRaw.get(),
                imageLoaderKtx.get(),
            };
        }

        return std::tuple{
            imageLoaderKtx.get(),
            imageLoaderRaw.get(),
        };
    }

    TextureOverrides::Loader OnlyKTX2LoaderIfNonDevMode()
    {
        if( isdevmode )
        {
            return std::tuple{
                imageLoaderRaw.get(),
                imageLoaderKtx.get(),
            };
        }

        return std::tuple{
            imageLoaderKtx.get(),
        };
    }

private:
    VkDevice           device;
    RgTextureSwizzling pbrSwizzling;

    std::shared_ptr< ImageLoader >    imageLoaderKtx;
    std::shared_ptr< ImageLoaderDev > imageLoaderRaw;
    bool                              isdevmode;

    std::shared_ptr< MemoryAllocator >      memAllocator;
    std::shared_ptr< CommandBufferManager > cmdManager;

    std::shared_ptr< SamplerManager >     samplerMgr;
    std::shared_ptr< TextureDescriptors > textureDesc;
    std::shared_ptr< TextureUploader >    textureUploader;

    std::vector< Texture >               textures;
    // Textures are not destroyed immediately, but only when they are not in use anymore
    std::vector< Texture >               texturesToDestroy[ MAX_FRAMES_IN_FLIGHT ];
    std::vector< std::filesystem::path > texturesToReload;

    enum class ImportedType
    {
        ForReplacement,
        ForStatic,
    };

    rgl::string_map< Material >     materials;
    rgl::string_map< ImportedType > importedMaterials;

    uint32_t waterNormalTextureIndex;
    uint32_t dirtMaskTextureIndex;

    RgSamplerFilter currentDynamicSamplerFilter;

    std::string postfixes[ TEXTURES_PER_MATERIAL_COUNT ];

    bool forceNormalMapFilterLinear;

    rgl::string_set forceExportAsExternal;

public:
    struct Debug_MaterialInfo
    {
        MaterialTextures textures;
        std::string      materialName;
        bool             isOriginal;
    };
    std::vector< Debug_MaterialInfo > Debug_GetMaterials() const;
};

}
