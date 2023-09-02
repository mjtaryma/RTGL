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

#ifndef RTGL1_H_
#define RTGL1_H_

#include <stdint.h>

#if defined( _WIN32 )
    #ifdef RG_LIBRARY_EXPORTS
        #define RGAPI __declspec( dllexport )
    #else
        #define RGAPI __declspec( dllimport )
    #endif // RTGL1_EXPORTS
    #define RGCONV __cdecl
#else
    #define RGAPI
    #define RGCONV
#endif // defined(_WIN32)

#define RG_RTGL_VERSION_API "1.03.0000"

#ifdef RG_USE_SURFACE_WIN32
    #include <windows.h>
#endif // RG_USE_SURFACE_WIN32
#ifdef RG_USE_SURFACE_METAL
    #ifdef __OBJC__
@class CAMetalLayer;
    #else
typedef void CAMetalLayer;
    #endif
#endif // RG_USE_SURFACE_METAL
#ifdef RG_USE_SURFACE_WAYLAND
    #include <wayland-client.h>
#endif // RG_USE_SURFACE_WAYLAND
#ifdef RG_USE_SURFACE_XCB
    #include <xcb/xcb.h>
#endif // RG_USE_SURFACE_XCB
#ifdef RG_USE_SURFACE_XLIB
    #include <X11/Xlib.h>
#endif // RG_USE_SURFACE_XLIB

#ifdef __cplusplus
extern "C" {
#endif

#if !defined( RG_DEFINE_NON_DISPATCHABLE_HANDLE )
    #if defined( __LP64__ ) || defined( _WIN64 ) || ( defined( __x86_64__ ) && !defined( __ILP32__ ) ) || defined( _M_X64 ) || defined( __ia64 ) || defined( _M_IA64 ) || defined( __aarch64__ ) || defined( __powerpc64__ )
        #define RG_DEFINE_NON_DISPATCHABLE_HANDLE( object ) typedef struct object##_T* object;
    #else
        #define RG_DEFINE_NON_DISPATCHABLE_HANDLE( object ) typedef uint64_t object;
    #endif
#endif

typedef uint32_t RgBool32;
#define RG_FALSE        0
#define RG_TRUE         1

RG_DEFINE_NON_DISPATCHABLE_HANDLE( RgInstance )
#define RG_NULL_HANDLE  0

typedef enum RgResult
{
    RG_RESULT_SUCCESS,
    RG_RESULT_SUCCESS_FOUND_MESH,
    RG_RESULT_SUCCESS_FOUND_TEXTURE,
    RG_RESULT_WRONG_INSTANCE,
    RG_RESULT_ALREADY_INITIALIZED,
    RG_RESULT_GRAPHICS_API_ERROR,
    RG_RESULT_INTERNAL_ERROR,
    RG_RESULT_CANT_FIND_SUPPORTED_PHYSICAL_DEVICE,
    RG_RESULT_FRAME_WASNT_STARTED,
    RG_RESULT_FRAME_WASNT_ENDED,
    RG_RESULT_WRONG_FUNCTION_CALL,
    RG_RESULT_WRONG_FUNCTION_ARGUMENT,
    RG_RESULT_WRONG_STRUCTURE_TYPE,
    RG_RESULT_ERROR_CANT_FIND_HARDCODED_RESOURCES,
    RG_RESULT_ERROR_CANT_FIND_SHADER,
    RG_RESULT_ERROR_MEMORY_ALIGNMENT,
} RgResult;

typedef enum RgMessageSeverityFlagBits
{
    RG_MESSAGE_SEVERITY_VERBOSE = 1,
    RG_MESSAGE_SEVERITY_INFO    = 2,
    RG_MESSAGE_SEVERITY_WARNING = 4,
    RG_MESSAGE_SEVERITY_ERROR   = 8,
} RgMessageSeverityFlagBits;
typedef uint32_t RgMessageSeverityFlags;

typedef void ( *PFN_rgPrint )( const char* pMessage, RgMessageSeverityFlags flags, void* pUserData );
typedef void ( *PFN_rgOpenFile )( const char* pFilePath, void* pUserData, const void** ppOutData, uint32_t* pOutDataSize, void** ppOutFileUserHandle );
typedef void ( *PFN_rgCloseFile )( void* pFileUserHandle, void* pUserData );

typedef struct RgWin32SurfaceCreateInfo   RgWin32SurfaceCreateInfo;
typedef struct RgMetalSurfaceCreateInfo   RgMetalSurfaceCreateInfo;
typedef struct RgWaylandSurfaceCreateInfo RgWaylandSurfaceCreateInfo;
typedef struct RgXcbSurfaceCreateInfo     RgXcbSurfaceCreateInfo;
typedef struct RgXlibSurfaceCreateInfo    RgXlibSurfaceCreateInfo;

#ifdef RG_USE_SURFACE_WIN32
typedef struct RgWin32SurfaceCreateInfo
{
    HINSTANCE           hinstance;
    HWND                hwnd;
} RgWin32SurfaceCreateInfo;
#endif // RG_USE_SURFACE_WIN32

#ifdef RG_USE_SURFACE_METAL
typedef struct RgMetalSurfaceCreateInfo
{
    const CAMetalLayer* pLayer;
} RgMetalSurfaceCreateInfo;
#endif // RG_USE_SURFACE_METAL

#ifdef RG_USE_SURFACE_WAYLAND
typedef struct RgWaylandSurfaceCreateInfo
{
    struct wl_display*  display;
    struct wl_surface*  surface;
} RgWaylandSurfaceCreateInfo;
#endif // RG_USE_SURFACE_WAYLAND

#ifdef RG_USE_SURFACE_XCB
typedef struct RgXcbSurfaceCreateInfo
{
    xcb_connection_t*   connection;
    xcb_window_t        window;
} RgXcbSurfaceCreateInfo;
#endif // RG_USE_SURFACE_XCB

#ifdef RG_USE_SURFACE_XLIB
typedef struct RgXlibSurfaceCreateInfo
{
    Display*            dpy;
    Window              window;
} RgXlibSurfaceCreateInfo;
#endif // RG_USE_SURFACE_XLIB

typedef enum RgStructureType
{
    RG_STRUCTURE_TYPE_NONE,
    RG_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    RG_STRUCTURE_TYPE_MESH_INFO,
    RG_STRUCTURE_TYPE_MESH_PRIMITIVE_INFO,
    RG_STRUCTURE_TYPE_MESH_PRIMITIVE_PORTAL_EXT,
    RG_STRUCTURE_TYPE_MESH_PRIMITIVE_TEXTURE_LAYERS_EXT,
    RG_STRUCTURE_TYPE_MESH_PRIMITIVE_PBR_EXT,
    RG_STRUCTURE_TYPE_MESH_PRIMITIVE_ATTACHED_LIGHT_EXT,
    RG_STRUCTURE_TYPE_MESH_PRIMITIVE_FORCE_RASTERIZED_EXT,
    RG_STRUCTURE_TYPE_LIGHT_INFO,
    RG_STRUCTURE_TYPE_LIGHT_DIRECTIONAL_EXT,
    RG_STRUCTURE_TYPE_LIGHT_SPHERICAL_EXT,
    RG_STRUCTURE_TYPE_LIGHT_POLYGONAL_EXT,
    RG_STRUCTURE_TYPE_LIGHT_SPOT_EXT,
    RG_STRUCTURE_TYPE_LIGHT_ADDITIONAL_EXT,
    RG_STRUCTURE_TYPE_ORIGINAL_TEXTURE_INFO,
    RG_STRUCTURE_TYPE_ORIGINAL_CUBEMAP_INFO,
    RG_STRUCTURE_TYPE_START_FRAME_INFO,
    RG_STRUCTURE_TYPE_DRAW_FRAME_INFO,
    RG_STRUCTURE_TYPE_DRAW_FRAME_RENDER_RESOLUTION_PARAMS,
    RG_STRUCTURE_TYPE_DRAW_FRAME_ILLUMINATION_PARAMS,
    RG_STRUCTURE_TYPE_DRAW_FRAME_VOLUMETRIC_PARAMS,
    RG_STRUCTURE_TYPE_DRAW_FRAME_TONEMAPPING_PARAMS,
    RG_STRUCTURE_TYPE_DRAW_FRAME_BLOOM_PARAMS,
    RG_STRUCTURE_TYPE_DRAW_FRAME_REFLECT_REFRACT_PARAMS,
    RG_STRUCTURE_TYPE_DRAW_FRAME_SKY_PARAMS,
    RG_STRUCTURE_TYPE_DRAW_FRAME_TEXTURES_PARAMS,
    RG_STRUCTURE_TYPE_DRAW_FRAME_LIGHTMAP_PARAMS,
    RG_STRUCTURE_TYPE_DRAW_FRAME_POST_EFFECTS_PARAMS,
    RG_STRUCTURE_TYPE_LENS_FLARE_INFO,
    RG_STRUCTURE_TYPE_DECAL_INFO,
    RG_STRUCTURE_TYPE_CAMERA_INFO,
} RgStructureType;

typedef enum RgTextureSwizzling
{
    RG_TEXTURE_SWIZZLING_NULL_ROUGHNESS_METALLIC,
    RG_TEXTURE_SWIZZLING_NULL_METALLIC_ROUGHNESS,
    RG_TEXTURE_SWIZZLING_OCCLUSION_ROUGHNESS_METALLIC,
    RG_TEXTURE_SWIZZLING_OCCLUSION_METALLIC_ROUGHNESS,
    RG_TEXTURE_SWIZZLING_ROUGHNESS_METALLIC,
    RG_TEXTURE_SWIZZLING_METALLIC_ROUGHNESS,
} RgTextureSwizzling;

typedef struct RgFloat2D
{
    float data[ 2 ];
} RgFloat2D;

typedef struct RgFloat3D
{
    float data[ 3 ];
} RgFloat3D;

typedef struct RgFloat4D
{
    float data[ 4 ];
} RgFloat4D;

typedef struct RgInstanceCreateInfo
{
    RgStructureType             sType;
    void*                       pNext;

    // Application name.
    const char*                 pAppName;
    // Application GUID. Generate it for your application and specify it here.
    const char*                 pAppGUID;

    // Exactly one of these surface create infos must be not null.
    RgWin32SurfaceCreateInfo*   pWin32SurfaceInfo;
    RgMetalSurfaceCreateInfo*   pMetalSurfaceCreateInfo;
    RgWaylandSurfaceCreateInfo* pWaylandSurfaceCreateInfo;
    RgXcbSurfaceCreateInfo*     pXcbSurfaceCreateInfo;
    RgXlibSurfaceCreateInfo*    pXlibSurfaceCreateInfo;

    // Path to the development configuration file. It's read line by line. Case-insensitive.
    // "VulkanValidation"   - validate each Vulkan API call and print using pfnPrint
    // "Developer"          - load PNG texture files instead of KTX2; reload a texture if its PNG file was changed
    // "FPSMonitor"         - show FPS at the window name
    // Default: "RayTracedGL1.txt"
    const char*                 pConfigPath;

    const char*                 pOverrideFolderPath;

    // Optional function to print messages from the library.
    // Requires "VulkanValidation" in the configuration file.
    PFN_rgPrint                 pfnPrint;
    // Custom user data that is passed to pfnUserPrint.
    void*                       pUserPrintData;
    RgMessageSeverityFlags      allowedMessages;

    // How many texture layers should be used to get albedo color for primary rays / indrect illumination.
    uint32_t                    primaryRaysMaxAlbedoLayers;
    uint32_t                    indirectIlluminationMaxAlbedoLayers;

    RgBool32                    rayCullBackFacingTriangles;
    // Allow RG_GEOMETRY_VISIBILITY_TYPE_SKY.
    // If true, RG_GEOMETRY_VISIBILITY_TYPE_WORLD_2 must not be used.
    RgBool32                    allowGeometryWithSkyFlag;
    RgBool32                    allowTexCoordLayer1;
    RgBool32                    allowTexCoordLayer2;
    RgBool32                    allowTexCoordLayer3;
    // Which layer to interpret as a lightmap. Can be 1, 2 or 3.
    uint32_t                    lightmapTexCoordLayerIndex;

    // Memory that must be allocated for vertex and index buffers of rasterized geometry.
    // It can't be changed after rgCreateInstance.
    // If buffer is full, rasterized data will be ignored
    uint32_t                    rasterizedMaxVertexCount;
    uint32_t                    rasterizedMaxIndexCount;
    // Apply gamma correction to packed rasterized vertex colors.
    RgBool32                    rasterizedVertexColorGamma;

    // Size of a cubemap side to render rasterized sky in.
    uint32_t                    rasterizedSkyCubemapSize;
    
    // If true, 'filter' in RgMaterialCreateInfo, RgCubemapCreateInfo
    // will set only magnification filter.
    RgBool32                    textureSamplerForceMinificationFilterLinear;
    RgBool32                    textureSamplerForceNormalMapFilterLinear;

    RgTextureSwizzling          pbrTextureSwizzling;

    RgBool32                    effectWipeIsUsed;

    // Used for exporting.
    // Up is also used for additional water flow calculations.
    RgFloat3D                   worldUp;
    RgFloat3D                   worldForward;
    // Used for exporting.
    // 1 game unit should correspond to (worldScale) meters.
    float                       worldScale;

} RgInstanceCreateInfo;

RGAPI RgResult RGCONV rgCreateInstance( const RgInstanceCreateInfo* pInfo, RgInstance* pResult );
RGAPI RgResult RGCONV rgDestroyInstance( RgInstance instance );


// Row-major transformation matrix.
typedef struct RgTransform
{
    float       matrix[ 3 ][ 4 ];
} RgTransform;

typedef struct RgMatrix3D
{
    float       matrix[ 3 ][ 3 ];
} RgMatrix3D;

typedef struct RgExtent2D
{
    uint32_t    width;
    uint32_t    height;
} RgExtent2D;

// Struct is used to transform from NDC to window coordinates.
// x, y, width, height are specified in pixels. (x,y) defines top-left corner.
typedef struct RgViewport
{
    float       x;
    float       y;
    float       width;
    float       height;
    float       minDepth;
    float       maxDepth;
} RgViewport;

typedef uint32_t RgColor4DPacked32;



typedef enum RgMeshPrimitiveFlagBits
{
    RG_MESH_PRIMITIVE_ALPHA_TESTED          = 1,
    RG_MESH_PRIMITIVE_TRANSLUCENT           = 2,
    RG_MESH_PRIMITIVE_FIRST_PERSON          = 4,
    RG_MESH_PRIMITIVE_FIRST_PERSON_VIEWER   = 8,
    RG_MESH_PRIMITIVE_SKY                   = 16,
    RG_MESH_PRIMITIVE_MIRROR                = 32,
    RG_MESH_PRIMITIVE_GLASS                 = 64,
    RG_MESH_PRIMITIVE_WATER                 = 128,
    RG_MESH_PRIMITIVE_DONT_GENERATE_NORMALS = 256,
    RG_MESH_PRIMITIVE_FORCE_EXACT_NORMALS   = 512,
    // If roughness is too small, act as a mirror (perfect reflection).
    RG_MESH_PRIMITIVE_MIRROR_IF_SMOOTH      = 1024,
    // If roughness is too small, act as a glass (perfect reflection/refraction).
    RG_MESH_PRIMITIVE_GLASS_IF_SMOOTH       = 2048,
    // Ignore refracting geometry behind this primitive.
    RG_MESH_PRIMITIVE_IGNORE_REFRACT_AFTER  = 4096,
    RG_MESH_PRIMITIVE_ACID                  = 8192,
    RG_MESH_PRIMITIVE_THIN_MEDIA            = 16384,
    RG_MESH_PRIMITIVE_SKY_VISIBILITY        = 32768,
} RgMeshPrimitiveFlagBits;
typedef uint32_t RgMeshPrimitiveFlags;

typedef enum RgMeshInfoFlagBits
{
    RG_MESH_INFO_EXPORT_AS_SEPARATE_FILE    = 1,
} RgMeshInfoFlagBits;
typedef uint32_t RgMeshInfoFlags;

typedef struct RgPrimitiveVertex
{
    float               position[ 3 ];      uint32_t _padding0;
    float               normal[ 3 ];        uint32_t _padding1;
    float               tangent[ 4 ];
    float               texCoord[ 2 ];
    RgColor4DPacked32   color;              uint32_t _padding2;
} RgPrimitiveVertex;

// Can be linked after RgMeshPrimitiveInfo.
typedef struct RgMeshPrimitivePortalEXT
{
    RgStructureType         sType;
    void*                   pNext;
    RgFloat3D               inPosition;
    RgFloat3D               inDirection;
    RgFloat3D               outPosition;
    RgFloat3D               outDirection;
} RgMeshPrimitivePortalEXT;

typedef enum RgTextureLayerBlendType
{
    RG_TEXTURE_LAYER_BLEND_TYPE_OPAQUE,
    RG_TEXTURE_LAYER_BLEND_TYPE_ALPHA,
    RG_TEXTURE_LAYER_BLEND_TYPE_ADD,
    RG_TEXTURE_LAYER_BLEND_TYPE_SHADE,
} RgTextureLayerBlendType;

typedef struct RgTextureLayer
{
    const RgFloat2D*        pTexCoord;
    const char*             pTextureName;
    RgTextureLayerBlendType blend;
    RgColor4DPacked32       color;
} RgTextureLayer;

// Can be linked after RgMeshPrimitiveInfo.
typedef struct RgMeshPrimitiveTextureLayersEXT
{
    RgStructureType         sType;
    void*                   pNext;
    RgTextureLayerBlendType baseLayerBlend;
    RgTextureLayer*         pLayer1;
    RgTextureLayer*         pLayer2;
    RgTextureLayer*         pLayer3;
} RgMeshPrimitiveTextureLayersEXT;

// Can be linked after RgMeshPrimitiveInfo.
typedef struct RgMeshPrimitivePBREXT
{
    RgStructureType         sType;
    void*                   pNext;
    // Multipliers for Roughness-Metallic texture.
    // If no texture present, multipliers are used directly as plain values.
    // Clamped to [0.0, 1.0]
    // Default: 1.0, if Roughness-Metallic texture exists
    //          0.0, otherwise
    float                   metallicDefault;
    // Default: 1.0
    float                   roughnessDefault;
} RgMeshPrimitivePBREXT;

// Can be linked after RgMeshPrimitiveInfo.
typedef struct RgMeshPrimitiveAttachedLightEXT
{
    RgStructureType         sType;
    void*                   pNext;
    float                   intensity;
    RgColor4DPacked32       color;
    RgBool32                evenOnDynamic;
} RgMeshPrimitiveAttachedLightEXT;

// Can be linked after RgMeshPrimitiveInfo.
typedef struct RgMeshPrimitiveForceRasterizedEXT
{
    RgStructureType         sType;
    void*                   pNext;
    const RgViewport*       pViewport;
    const float*            pView;
    const float*            pProjection;
    const float*            pViewProjection;
} RgMeshPrimitiveForceRasterizedEXT;

// Primitive is an indexed or non-indexed geometry with a material.
typedef struct RgMeshPrimitiveInfo
{
    RgStructureType             sType;
    void*                       pNext;
    RgMeshPrimitiveFlags        flags;
    uint32_t                    primitiveIndexInMesh;
    const RgPrimitiveVertex*    pVertices;
    uint32_t                    vertexCount;
    const uint32_t*             pIndices;
    uint32_t                    indexCount;
    const char*                 pTextureName;
    uint32_t                    textureFrame;
    // If alpha < 1.0, then RG_MESH_PRIMITIVE_TRANSLUCENT is assumed.
    RgColor4DPacked32           color;
    float                       emissive;
} RgMeshPrimitiveInfo;

// Mesh is a set of primitives.
typedef struct RgMeshInfo
{
    RgStructureType             sType;
    void*                       pNext;
    RgMeshInfoFlags             flags;
    // Object is an instance of a mesh.
    uint64_t                    uniqueObjectID;
    // Name and primitive index is used to override meshes.
    const char*                 pMeshName;
    RgTransform                 transform;
    // Set to true, if an object can be exported.
    // If RG_MESH_INFO_EXPORT_AS_SEPARATE_FILE is set, a separate 3D model file
    // will be found. If not set, a scene name is used.
    RgBool32                    isExportable;
    float                       animationTime;
} RgMeshInfo;

RGAPI RgResult RGCONV rgUploadMeshPrimitive( RgInstance                 instance,
                                             const RgMeshInfo*          pMesh,
                                             const RgMeshPrimitiveInfo* pPrimitive );



typedef struct RgDecalInfo
{
    RgStructureType             sType;
    void*                       pNext;
    // Transformation from [-0.5, 0.5] cube to a scaled oriented box.
    // Orientation should transform (0,0,1) to decal's normal.
    RgTransform                 transform;
    const char*                 pTextureName;
} RgDecalInfo;

RGAPI RgResult RGCONV rgUploadDecal( RgInstance instance, const RgDecalInfo* pInfo );

// Render specified vertex geometry, if 'pointToCheck' is not hidden.
typedef struct RgLensFlareInfo
{
    RgStructureType             sType;
    void*                       pNext;
    // Must be in world space.
    uint32_t                    vertexCount;
    const RgPrimitiveVertex*    pVertices;
    // Must not be null.
    uint32_t                    indexCount;
    const uint32_t*             pIndices;
    const char*                 pTextureName;
    // Point in the world space.
    RgFloat3D                   pointToCheck;
} RgLensFlareInfo;

RGAPI RgResult RGCONV rgUploadLensFlare( RgInstance instance, const RgLensFlareInfo* pInfo );



typedef uint32_t RgCameraFlags;

typedef struct RgCameraInfo
{
    RgStructureType sType;
    void*           pNext;
    RgCameraFlags   flags;
    // View matrix is column major.
    float           view[ 16 ];
    float           fovYRadians;
    float           aspect;
    // Near and far planes for a projection matrix.
    float           cameraNear;
    float           cameraFar;
} RgCameraInfo;

RGAPI RgResult RGCONV rgUploadCamera( RgInstance instance, const RgCameraInfo* pInfo );



// Can be linked after RgLightDirectionalEXT / RgLightSphericalEXT / RgLightPolygonalEXT /
// RgLightSpotEXT.
typedef struct RgLightAdditionalEXT
{
    RgStructureType     sType;
    void*               pNext;
    int                 lightstyle;
    // Use the light source for scattering.
    // Only one per scene is allowed.
    // If GLTF is used, this can be overwritten by a GLTF's sun.
    RgBool32            isVolumetric;
} RgLightAdditionalEXT;

// Can be linked after RgLightInfo.
typedef struct RgLightDirectionalEXT
{
    RgStructureType     sType;
    void*               pNext;
    RgColor4DPacked32   color;
    // Luminous flux received by a surface, in lumen / m^2
    // (i.e. illuminance, in lux)
    float               intensity;
    RgFloat3D           direction;
    float               angularDiameterDegrees;
} RgLightDirectionalEXT;

// Can be linked after RgLightInfo.
typedef struct RgLightSphericalEXT
{
    RgStructureType     sType;
    void*               pNext;
    RgColor4DPacked32   color;
    // Luminous flux in lumen
    float               intensity;
    RgFloat3D           position;
    float               radius;
} RgLightSphericalEXT;

// Can be linked after RgLightInfo.
typedef struct RgLightPolygonalEXT
{
    RgStructureType     sType;
    void*               pNext;
    RgColor4DPacked32   color;
    // Luminous flux in lumen
    float               intensity;
    RgFloat3D           positions[ 3 ];
} RgLightPolygonalEXT;

// Can be linked after RgLightInfo.
typedef struct RgLightSpotEXT
{
    RgStructureType     sType;
    void*               pNext;
    RgColor4DPacked32   color;
    // Luminous flux in lumen
    float               intensity;
    RgFloat3D           position;
    RgFloat3D           direction;
    float               radius;
    // Inner cone angle. In radians.
    float               angleOuter;
    // Outer cone angle. In radians.
    float               angleInner;
} RgLightSpotEXT;

typedef struct RgLightInfo
{
    RgStructureType sType;
    void*           pNext;
    // Used to match the same light source from the previous frame.
    uint64_t        uniqueID;
    RgBool32        isExportable;
} RgLightInfo;

RGAPI RgResult RGCONV rgUploadLight( RgInstance instance, const RgLightInfo* pInfo );



typedef enum RgSamplerFilter
{
    RG_SAMPLER_FILTER_AUTO,
    RG_SAMPLER_FILTER_LINEAR,
    RG_SAMPLER_FILTER_NEAREST,
} RgSamplerFilter;

typedef enum RgSamplerAddressMode
{
    RG_SAMPLER_ADDRESS_MODE_REPEAT,
    RG_SAMPLER_ADDRESS_MODE_CLAMP,
} RgSamplerAddressMode;

typedef struct RgOriginalTextureInfo
{
    RgStructureType         sType;
    void*                   pNext;
    const char*             pTextureName;
    // R8G8B8A8 pixel data. Must be (size.width * size.height * 4) bytes.
    const void*             pPixels;
    RgExtent2D              size;
    RgSamplerFilter         filter;
    RgSamplerAddressMode    addressModeU;
    RgSamplerAddressMode    addressModeV;
} RgOriginalTextureInfo;

typedef struct RgOriginalCubemapInfo
{
    RgStructureType         sType;
    void*                   pNext;
    const char*             pTextureName;
    // R8G8B8A8 pixel data. Each must be (sideSize * sideSize * 4) bytes.
    const void*             pPixelsPositiveX;
    const void*             pPixelsNegativeX;
    const void*             pPixelsPositiveY;
    const void*             pPixelsNegativeY;
    const void*             pPixelsPositiveZ;
    const void*             pPixelsNegativeZ;
    uint32_t                sideSize;
} RgOriginalCubemapInfo;

RGAPI RgResult RGCONV rgProvideOriginalTexture( RgInstance instance, const RgOriginalTextureInfo* pInfo );
RGAPI RgResult RGCONV rgProvideOriginalCubemapTexture( RgInstance instance, const RgOriginalCubemapInfo* pInfo );
RGAPI RgResult RGCONV rgMarkOriginalTextureAsDeleted( RgInstance instance, const char* pTextureName );



typedef struct RgStartFrameInfo
{
    RgStructureType sType;
    void*           pNext;
    const char*     pMapName;
    RgBool32        ignoreExternalGeometry;
    RgBool32        vsync;
} RgStartFrameInfo;

RGAPI RgResult RGCONV rgStartFrame( RgInstance instance, const RgStartFrameInfo* pInfo );



// Can be linked after RgDrawFrameInfo.
typedef enum RgRenderUpscaleTechnique
{
    RG_RENDER_UPSCALE_TECHNIQUE_LINEAR,
    RG_RENDER_UPSCALE_TECHNIQUE_NEAREST,
    RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2,
    RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS,
} RgRenderUpscaleTechnique;

typedef enum RgRenderSharpenTechnique
{
    RG_RENDER_SHARPEN_TECHNIQUE_NONE,
    RG_RENDER_SHARPEN_TECHNIQUE_NAIVE,
    RG_RENDER_SHARPEN_TECHNIQUE_AMD_CAS,
} RgRenderSharpenTechnique;

typedef enum RgRenderResolutionMode
{
    RG_RENDER_RESOLUTION_MODE_CUSTOM,
    RG_RENDER_RESOLUTION_MODE_ULTRA_PERFORMANCE,
    RG_RENDER_RESOLUTION_MODE_PERFORMANCE,
    RG_RENDER_RESOLUTION_MODE_BALANCED,
    RG_RENDER_RESOLUTION_MODE_QUALITY,
    RG_RENDER_RESOLUTION_MODE_DLAA, // Only available with DLSS
} RgRenderResolutionMode;

typedef struct RgDrawFrameRenderResolutionParams
{
    RgStructureType          sType;
    void*                    pNext;
    RgRenderUpscaleTechnique upscaleTechnique;
    RgRenderSharpenTechnique sharpenTechnique;
    RgRenderResolutionMode   resolutionMode;
    // Used, if resolutionMode is RG_RENDER_RESOLUTION_MODE_CUSTOM
    RgExtent2D               customRenderSize;
    // If true, final image will be downscaled to 'pixelizedRenderSize' at the very end.
    // Needed, if pixelized look is needed, but the actual rendering should
    // be done in higher resolution.
    RgBool32                 pixelizedRenderSizeEnable;
    RgExtent2D               pixelizedRenderSize;
    // Drop history, e.g. if there's camera changed its position drastically.
    RgBool32                 resetUpscalerHistory;
} RgDrawFrameRenderResolutionParams;

typedef enum RgSkyType
{
    RG_SKY_TYPE_COLOR,
    RG_SKY_TYPE_CUBEMAP,
    RG_SKY_TYPE_RASTERIZED_GEOMETRY,
} RgSkyType;

// Can be linked after RgDrawFrameInfo.
typedef struct RgDrawFrameTonemappingParams
{
    RgStructureType sType;
    void*           pNext;
    RgBool32        disableEyeAdaptation;
    float           ev100Min;
    float           ev100Max;
    float           luminanceWhitePoint;
    // A per channel adjustment, use <0 decrease, 0=no change, >0 increase.
    // Default: 0 0 0
    RgFloat3D       saturation;
    // One channel must be 1.0, the rest can be <= 1.0 but not zero.
    // Default: 1.0 1.0 1.0
    RgFloat3D       crosstalk;
} RgDrawFrameTonemappingParams;

// Can be linked after RgDrawFrameInfo.
typedef struct RgDrawFrameSkyParams
{
    RgStructureType sType;
    void*           pNext;
    RgSkyType       skyType;
    // Used as a main color for RG_SKY_TYPE_COLOR.
    RgFloat3D       skyColorDefault;
    // The result sky color is multiplied by this value.
    float           skyColorMultiplier;
    float           skyColorSaturation;
    // A point from which rays are traced while using RG_SKY_TYPE_RASTERIZED_GEOMETRY.
    RgFloat3D       skyViewerPosition;
    // If sky type is RG_SKY_TYPE_CUBEMAP, this cubemap is used.
    const char*     pSkyCubemapTextureName;
    // Apply this transform to the direction when sampling a sky cubemap (RG_SKY_TYPE_CUBEMAP).
    // If equals to zero, then default value is used.
    // Default: identity matrix.
    RgMatrix3D      skyCubemapRotationTransform;
} RgDrawFrameSkyParams;

// Can be linked after RgDrawFrameInfo.
typedef struct RgDrawFrameTexturesParams
{
    RgStructureType sType;
    void*           pNext;
    // What sampler filter to use for materials with RG_MATERIAL_CREATE_DYNAMIC_SAMPLER_FILTER_BIT.
    // Should be changed infrequently, as it reloads all texture descriptors.
    RgSamplerFilter dynamicSamplerFilter;
    float           normalMapStrength;
    // Multiplier for emission map values for indirect lighting.
    float           emissionMapBoost;
    // Upper bound for emissive materials in primary albedo channel (i.e. on screen).
    float           emissionMaxScreenColor;
    // Default: 0.0
    float           minRoughness;
} RgDrawFrameTexturesParams;

// Can be linked after RgDrawFrameInfo.
typedef struct RgDrawFrameIlluminationParams
{
    RgStructureType sType;
    void*           pNext;
    // Shadow rays are cast, if illumination bounce index is in [0, maxBounceShadows).
    uint32_t        maxBounceShadows;
    // If false, only one bounce will be cast from a primary surface.
    // If true, a bounce of that bounce will be also cast.
    // If false, reflections and indirect diffuse might appear darker,
    // since inside of them, shadowed areas are just pitch black.
    // Default: true
    RgBool32        enableSecondBounceForIndirect;
    // Size of the side of a cell for the light grid. Use RG_DEBUG_DRAW_LIGHT_GRID_BIT for the debug view.
    // Each cell is used to store a fixed amount of light samples that are important for the cell's center and radius.
    // Default: 1.0
    float           cellWorldSize;
    // If 0.0, then the change of illumination won't be checked, i.e. if a light source suddenly disappeared,
    // its lighting still will be visible. But if it's 1.0, then lighting will be dropped at the given screen region
    // and the accumulation will start from scratch.
    // Default: 0.5
    float           directDiffuseSensitivityToChange;
    // Default: 0.2
    float           indirectDiffuseSensitivityToChange;
    // Default: 0.5
    float           specularSensitivityToChange;
    // The higher the value, the more polygonal lights act like a spotlight. 
    // Default: 2.0
    float           polygonalLightSpotlightFactor;
    // For which light first-person viewer shadows should be ignored.
    // E.g. first-person flashlight.
    // Null, if none.
    uint64_t*       lightUniqueIdIgnoreFirstPersonViewerShadows;
    uint32_t        lightstyleValuesCount;
    const float*    pLightstyleValues;
} RgDrawFrameIlluminationParams;

// Can be linked after RgDrawFrameInfo.
typedef struct RgDrawFrameVolumetricParams
{
    RgStructureType sType;
    void*           pNext;
    RgBool32        enable;
    // Default: 8.0
    float           maxHistoryLength;
    // If true, volumetric illumination is not calculated, just
    // using simple depth-based fog with ambient color.
    RgBool32        useSimpleDepthBased;
    // Farthest distance for volumetric illumination calculation.
    // Should be minimal to have better precision around camera.
    // Default: 100.0
    float           volumetricFar;
    RgFloat3D       ambientColor;
    // Default: 0.2
    float           scaterring;
    // g parameter [-1..1] for the Henyey�Greenstein phase function.
    // Default: 0.0 (isotropic)
    float           assymetry;
    // If true, maintain a world-space grid, each cell of which contains
    // illumination, that is used for scattering.
    RgBool32        useIlluminationVolume;
    // If light source is not provided (RgLightExtraInfo::isVolumetric), use this fallback info.
    // If color is too dim or direction is invalid,
    // then no light sources for scattering.
    RgFloat3D       fallbackSourceColor;
    RgFloat3D       fallbackSourceDirection;
    // Multiplier for light for scattering.
    float           lightMultiplier;
    RgBool32        allowTintUnderwater;
    RgFloat3D       underwaterColor;
} RgDrawFrameVolumetricParams;

// Can be linked after RgDrawFrameInfo.
typedef struct RgDrawFrameBloomParams
{
    RgStructureType sType;
    void*           pNext;
    // Negative value disables bloom pass
    float           bloomIntensity;
    float           inputThreshold;
    float           bloomEmissionMultiplier;
    float           lensDirtIntensity;
} RgDrawFrameBloomParams;

typedef struct RgPostEffectWipe
{
    // [0..1] where 1 is whole screen width.
    float       stripWidth;
    RgBool32    beginNow;
    float       duration;
} RgPostEffectWipe;

typedef struct RgPostEffectRadialBlur
{
    RgBool32    isActive;
    float       transitionDurationIn;
    float       transitionDurationOut;
} RgPostEffectRadialBlur;

typedef struct RgPostEffectChromaticAberration
{
    RgBool32    isActive;
    float       transitionDurationIn;
    float       transitionDurationOut;
    float       intensity;
} RgPostEffectChromaticAberration;

typedef struct RgPostEffectInverseBlackAndWhite
{
    RgBool32    isActive;
    float       transitionDurationIn;
    float       transitionDurationOut;
} RgPostEffectInverseBlackAndWhite;

typedef struct RgPostEffectHueShift
{
    RgBool32    isActive;
    float       transitionDurationIn;
    float       transitionDurationOut;
} RgPostEffectHueShift;

typedef struct RgPostEffectDistortedSides
{
    RgBool32    isActive;
    float       transitionDurationIn;
    float       transitionDurationOut;
} RgPostEffectDistortedSides;

typedef struct RgPostEffectWaves
{
    RgBool32    isActive;
    float       transitionDurationIn;
    float       transitionDurationOut;
    float       amplitude;
    float       speed;
    float       xMultiplier;
} RgPostEffectWaves;

typedef struct RgPostEffectColorTint
{
    RgBool32    isActive;
    float       transitionDurationIn;
    float       transitionDurationOut;
    float       intensity;
    RgFloat3D   color;
} RgPostEffectColorTint;

typedef struct RgPostEffectCRT
{
    RgBool32    isActive;
} RgPostEffectCRT;

typedef struct RgPostEffectTeleport
{
    RgBool32    isActive;
    float       transitionDurationIn;
    float       transitionDurationOut;
} RgPostEffectTeleport;

// Can be linked after RgDrawFrameInfo.
typedef struct RgDrawFramePostEffectsParams
{
    RgStructureType                         sType;
    void*                                   pNext;
    // Must be null, if effectWipeIsUsed was false.
    const RgPostEffectWipe*                 pWipe;
    const RgPostEffectRadialBlur*           pRadialBlur;
    const RgPostEffectChromaticAberration*  pChromaticAberration;
    const RgPostEffectInverseBlackAndWhite* pInverseBlackAndWhite;
    const RgPostEffectHueShift*             pHueShift;
    const RgPostEffectDistortedSides*       pDistortedSides;
    const RgPostEffectWaves*                pWaves;
    const RgPostEffectColorTint*            pColorTint;
    const RgPostEffectTeleport*             pTeleport;
    const RgPostEffectCRT*                  pCRT;
} RgDrawFramePostEffectsParams;

typedef enum RgMediaType
{
    RG_MEDIA_TYPE_VACUUM,
    RG_MEDIA_TYPE_WATER,
    RG_MEDIA_TYPE_GLASS,
    RG_MEDIA_TYPE_ACID,
} RgMediaType;

// Can be linked after RgDrawFrameInfo.
typedef struct RgDrawFrameReflectRefractParams
{
    RgStructureType sType;
    void*           pNext;
    uint32_t        maxReflectRefractDepth;
    // Media type, in which camera currently is.
    RgMediaType     typeOfMediaAroundCamera;
    // Default: 1.52
    float           indexOfRefractionGlass;
    // Default: 1.33
    float           indexOfRefractionWater;
    float           thinMediaWidth;
    float           waterWaveSpeed;
    float           waterWaveNormalStrength;
    // Color at 1 meter depth.
    RgFloat3D       waterColor;
    // Color at 1 meter depth.
    RgFloat3D       acidColor;
    float           acidDensity;
    // The lower this value, the sharper water normal textures.
    // Default: 1.0
    float           waterWaveTextureDerivativesMultiplier;
    // The larger this value, the larger the area one water texture covers.
    // If equals to 0.0, then default value is used.
    // Default: 1.0
    float           waterTextureAreaScale;
    // If true, portal normal will be twirled around its 'inPosition'.
    RgBool32        portalNormalTwirl;
} RgDrawFrameReflectRefractParams;

// Can be linked after RgDrawFrameInfo.
typedef struct RgDrawFrameLightmapParams
{
    RgStructureType             sType;
    void*                       pNext;
    // How much of the screen should be rendered in a lightmap mode.
    // In [0.0, 1.0]
    float                       lightmapScreenCoverage;
} RgDrawFrameLightmapParams;

typedef enum RgDrawFrameRayCullFlagBits
{
    RG_DRAW_FRAME_RAY_CULL_WORLD_0_BIT  = 1,    // RG_GEOMETRY_VISIBILITY_TYPE_WORLD_0
    RG_DRAW_FRAME_RAY_CULL_WORLD_1_BIT  = 2,    // RG_GEOMETRY_VISIBILITY_TYPE_WORLD_1
    RG_DRAW_FRAME_RAY_CULL_WORLD_2_BIT  = 4,    // RG_GEOMETRY_VISIBILITY_TYPE_WORLD_2
    RG_DRAW_FRAME_RAY_CULL_SKY_BIT      = 8,    // RG_GEOMETRY_VISIBILITY_TYPE_SKY
} RgDrawFrameRayCullFlagBits;
typedef uint32_t RgDrawFrameRayCullFlags;

typedef struct RgDrawFrameInfo
{
    RgStructureType             sType;
    void*                       pNext;
    // Max value: 10000.0
    float                       rayLength;
    // What world parts to render. First-person related geometry is always enabled.
    RgDrawFrameRayCullFlags     rayCullMaskWorld;

    RgBool32                    disableRayTracedGeometry;
    RgBool32                    disableRasterization;
    RgBool32                    presentPrevFrame;

    double                      currentTime;
} RgDrawFrameInfo;

RGAPI RgResult RGCONV           rgDrawFrame( RgInstance instance, const RgDrawFrameInfo* pInfo );



typedef enum RgUtilImScratchTopology
{
    RG_UTIL_IM_SCRATCH_TOPOLOGY_TRIANGLES,
    RG_UTIL_IM_SCRATCH_TOPOLOGY_TRIANGLE_STRIP,
    RG_UTIL_IM_SCRATCH_TOPOLOGY_TRIANGLE_FAN,
    RG_UTIL_IM_SCRATCH_TOPOLOGY_QUADS,
} RgUtilImScratchTopology;

RGAPI RgPrimitiveVertex* RGCONV rgUtilScratchAllocForVertices( RgInstance instance, uint32_t vertexCount );
RGAPI void RGCONV               rgUtilScratchFree( RgInstance instance, const RgPrimitiveVertex* pPointer );
RGAPI void RGCONV               rgUtilScratchGetIndices( RgInstance instance, RgUtilImScratchTopology topology, uint32_t vertexCount, const uint32_t** ppOutIndices, uint32_t* pOutIndexCount );

RGAPI void RGCONV               rgUtilImScratchClear( RgInstance instance );
RGAPI void RGCONV               rgUtilImScratchStart( RgInstance instance, RgUtilImScratchTopology topology );
RGAPI void RGCONV               rgUtilImScratchVertex( RgInstance instance, float x, float y, float z );            // Push vertex to a list
RGAPI void RGCONV               rgUtilImScratchNormal( RgInstance instance, float x, float y, float z );
RGAPI void RGCONV               rgUtilImScratchTexCoord( RgInstance instance, float u, float v );
RGAPI void RGCONV               rgUtilImScratchTexCoord_Layer1( RgInstance instance, float u, float v );
RGAPI void RGCONV               rgUtilImScratchTexCoord_Layer2( RgInstance instance, float u, float v );
RGAPI void RGCONV               rgUtilImScratchTexCoord_Layer3( RgInstance instance, float u, float v );
RGAPI void RGCONV               rgUtilImScratchColor( RgInstance instance, RgColor4DPacked32 color );
RGAPI void RGCONV               rgUtilImScratchEnd( RgInstance instance );
RGAPI void RGCONV               rgUtilImScratchSetToPrimitive( RgInstance instance, RgMeshPrimitiveInfo* pTarget ); // Set accumulated vertices to pTarget

RGAPI RgBool32 RGCONV           rgUtilIsUpscaleTechniqueAvailable( RgInstance instance, RgRenderUpscaleTechnique technique );
RGAPI const char* RGCONV        rgUtilGetResultDescription( RgResult result );
RGAPI RgColor4DPacked32 RGCONV  rgUtilPackColorByte4D( uint8_t r, uint8_t g, uint8_t b, uint8_t a );
RGAPI RgColor4DPacked32 RGCONV  rgUtilPackColorFloat4D( float r, float g, float b, float a );
RGAPI void RGCONV               rgUtilExportAsTGA( RgInstance instance, const void* pPixels, uint32_t width, uint32_t height, const char* pPath );

#ifdef __cplusplus
}
#endif

#endif // RTGL1_H_
