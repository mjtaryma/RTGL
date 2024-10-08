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

#include "ASManager.h"

#include "CmdLabel.h"
#include "DrawFrameInfo.h"
#include "Fluid.h"
#include "GeomInfoManager.h"
#include "Matrix.h"
#include "Utils.h"

#include "Generated/ShaderCommonC.h"

#include <array>
#include <cstring>

namespace
{
bool IsFastBuild( RTGL1::VertexCollectorFilterTypeFlags filter )
{
    using FT = RTGL1::VertexCollectorFilterTypeFlagBits;

    // fast trace for static non-movable,
    // fast build for dynamic and movable
    // (TODO: fix: device lost occurs on heavy scenes if with movable)
    return ( filter & FT::CF_DYNAMIC ) /* || (filter & FT::CF_STATIC_MOVABLE)*/;
}

bool IsFastTrace( RTGL1::VertexCollectorFilterTypeFlags filter )
{
    return !IsFastBuild( filter );
}

uint32_t floatToUint8( float v )
{
    return static_cast< uint32_t >( std::clamp( v, 0.f, 1.f ) * 255.f );
}
}

RTGL1::ASManager::ASManager( VkDevice                                _device,
                             const PhysicalDevice&                   _physDevice,
                             std::shared_ptr< MemoryAllocator >      _allocator,
                             std::shared_ptr< CommandBufferManager > _cmdManager,
                             std::shared_ptr< GeomInfoManager >      _geomInfoManager,
                             uint64_t                                _maxReplacementsVerts,
                             uint64_t                                _maxDynamicVerts,
                             bool                                    _enableTexCoordLayer1,
                             bool                                    _enableTexCoordLayer2,
                             bool                                    _enableTexCoordLayer3 )
    : device( _device )
    , allocator( std::move( _allocator ) )
    , staticCopyFence( VK_NULL_HANDLE )
    , cmdManager( std::move( _cmdManager ) )
    , geomInfoMgr( std::move( _geomInfoManager ) )
    , descPool( VK_NULL_HANDLE )
    , buffersDescSetLayout( VK_NULL_HANDLE )
    , buffersDescSets{}
    , asDescSetLayout( VK_NULL_HANDLE )
    , asDescSets{}
{
    for( auto& t : tlas )
    {
        t = std::make_unique< TLASComponent >( device, "TLAS main" );
    }

    {
        const uint32_t scratchOffsetAligment =
            _physDevice.GetASProperties().minAccelerationStructureScratchOffsetAlignment;

        scratchBuffer = std::make_shared< ChunkedStackAllocator >(
            allocator,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            32 * 1024 * 1024,
            scratchOffsetAligment,
            "Scratch buffer" );

        asBuilder = std::make_unique< ASBuilder >( scratchBuffer );
    }

    {
        constexpr auto asAlignment = 256;
        constexpr auto usage       = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        allocStaticGeom = std::make_unique< ChunkedStackAllocator >(
            allocator, usage, 16 * 1024 * 1024, asAlignment, "BLAS common buffer for static" );

        allocReplacementsGeom =
            std::make_unique< ChunkedStackAllocator >( allocator,
                                                       usage,
                                                       64 * 1024 * 1024,
                                                       asAlignment,
                                                       "BLAS common buffer for replacements" );

        for( uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
        {
            allocDynamicGeom[ i ] = std::make_unique< ChunkedStackAllocator >(
                allocator, usage, 16 * 1024 * 1024, asAlignment, "BLAS common buffer for dynamic" );

            allocTlas[ i ] = std::make_unique< ChunkedStackAllocator >(
                allocator, usage, 16 * 1024 * 1024, asAlignment, "TLAS common buffer" );
        }
    }

    _maxReplacementsVerts = _maxReplacementsVerts > 0 ? _maxReplacementsVerts : 2097152;
    {
        const size_t maxVertsPerLayer[] = {
            _maxReplacementsVerts,
            _enableTexCoordLayer1 ? _maxReplacementsVerts : 0,
            _enableTexCoordLayer2 ? _maxReplacementsVerts : 0,
            _enableTexCoordLayer3 ? _maxReplacementsVerts : 0,
        };
        const size_t maxIndices = _maxReplacementsVerts * 3;

        collectorStatic = std::make_unique< VertexCollector >(
            device, *allocator, maxVertsPerLayer, maxIndices, false, "Static" );
    }

    _maxDynamicVerts = _maxDynamicVerts > 0 ? _maxDynamicVerts : 2097152;
    {
        const size_t maxVertsPerLayer[] = {
            _maxDynamicVerts,
            _enableTexCoordLayer1 ? _maxDynamicVerts : 0,
            _enableTexCoordLayer2 ? _maxDynamicVerts : 0,
            _enableTexCoordLayer3 ? _maxDynamicVerts : 0,
        };
        const size_t maxIndices = _maxDynamicVerts * 3;

        collectorDynamic[ 0 ] = std::make_unique< VertexCollector >(
            device, *allocator, maxVertsPerLayer, maxIndices, true, "Dynamic 0" );

        // share device-local buffer with 0
        collectorDynamic[ 1 ] = VertexCollector::CreateWithSameDeviceLocalBuffers(
            *( collectorDynamic[ 0 ] ), *allocator, "Dynamic 1" );

        assert( std::size( collectorDynamic ) == 2 );
    }

    previousDynamicPositions.Init( *allocator,
                                   _maxDynamicVerts * sizeof( ShVertex ),
                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                   "Previous frame's vertex data" );
    previousDynamicIndices.Init( *allocator,
                                 _maxDynamicVerts * sizeof( uint32_t ),
                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 "Previous frame's index data" );


    // instance buffer for TLAS
    instanceBuffer = std::make_unique< AutoBuffer >( allocator );

    constexpr VkDeviceSize instanceBufferSize =
        MAX_INSTANCE_COUNT * sizeof( VkAccelerationStructureInstanceKHR );

    instanceBuffer->Create(
        instanceBufferSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        "TLAS instance buffer" );


    CreateDescriptors();

    // buffers won't be changing, update once
    for( uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
    {
        UpdateBufferDescriptors( i );
    }


    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags             = 0;
    VkResult r                  = vkCreateFence( device, &fenceInfo, nullptr, &staticCopyFence );
    VK_CHECKERROR( r );

    SET_DEBUG_NAME( device, staticCopyFence, VK_OBJECT_TYPE_FENCE, "Static BLAS fence" );
}

namespace
{

template< size_t N >
constexpr bool CheckBindings( const VkDescriptorSetLayoutBinding ( &bindings )[ N ] )
{
    for( size_t i = 0; i < N; i++ )
    {
        if( bindings[ i ].binding != i )
        {
            return false;
        }
    }
    return true;
}

template< size_t N >
constexpr bool CheckBindings( const VkWriteDescriptorSet ( &bindings )[ N ] )
{
    for( size_t i = 0; i < N; i++ )
    {
        if( bindings[ i ].dstBinding != i )
        {
            return false;
        }
    }
    return true;
}

}

void RTGL1::ASManager::CreateDescriptors()
{
    VkResult                              r;
    std::array< VkDescriptorPoolSize, 2 > poolSizes{};

    {
        constexpr VkDescriptorSetLayoutBinding bindings[] = {
            {
                .binding         = BINDING_VERTEX_BUFFER_STATIC,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_VERTEX_BUFFER_DYNAMIC,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_INDEX_BUFFER_STATIC,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_INDEX_BUFFER_DYNAMIC,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_GEOMETRY_INSTANCES,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_GEOMETRY_INSTANCES_MATCH_PREV,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_PREV_POSITIONS_BUFFER_DYNAMIC,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_PREV_INDEX_BUFFER_DYNAMIC,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_STATIC_TEXCOORD_LAYER_1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_STATIC_TEXCOORD_LAYER_2,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_STATIC_TEXCOORD_LAYER_3,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_DYNAMIC_TEXCOORD_LAYER_1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_DYNAMIC_TEXCOORD_LAYER_2,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
            {
                .binding         = BINDING_DYNAMIC_TEXCOORD_LAYER_3,
                .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_ALL,
            },
        };
        static_assert( CheckBindings( bindings ) );

        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = std::size( bindings ),
            .pBindings    = bindings,
        };
        r = vkCreateDescriptorSetLayout( device, &layoutInfo, nullptr, &buffersDescSetLayout );
        VK_CHECKERROR( r );

        poolSizes[ 0 ] = {
            .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = MAX_FRAMES_IN_FLIGHT * std::size( bindings ),
        };
    }

    {
        VkDescriptorSetLayoutBinding bnd = {
            .binding         = BINDING_ACCELERATION_STRUCTURE_MAIN,
            .descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT,
        };
        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings    = &bnd,
        };
        r = vkCreateDescriptorSetLayout( device, &layoutInfo, nullptr, &asDescSetLayout );
        VK_CHECKERROR( r );

        poolSizes[ 1 ] = {
            .type            = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = MAX_FRAMES_IN_FLIGHT,
        };
    }

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = MAX_FRAMES_IN_FLIGHT * 2,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes    = poolSizes.data(),
    };
    r = vkCreateDescriptorPool( device, &poolInfo, nullptr, &descPool );
    VK_CHECKERROR( r );

    SET_DEBUG_NAME( device, descPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "AS manager Desc pool" );

    VkDescriptorSetAllocateInfo descSetInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descPool,
        .descriptorSetCount = 1,
    };

    SET_DEBUG_NAME( device,
                    buffersDescSetLayout,
                    VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                    "Vertex data Desc set layout" );
    SET_DEBUG_NAME(
        device, asDescSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "TLAS Desc set layout" );

    for( uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
    {
        descSetInfo.pSetLayouts = &buffersDescSetLayout;
        r = vkAllocateDescriptorSets( device, &descSetInfo, &buffersDescSets[ i ] );
        VK_CHECKERROR( r );

        descSetInfo.pSetLayouts = &asDescSetLayout;
        r = vkAllocateDescriptorSets( device, &descSetInfo, &asDescSets[ i ] );
        VK_CHECKERROR( r );

        SET_DEBUG_NAME(
            device, buffersDescSets[ i ], VK_OBJECT_TYPE_DESCRIPTOR_SET, "Vertex data Desc set" );
        SET_DEBUG_NAME( device, asDescSets[ i ], VK_OBJECT_TYPE_DESCRIPTOR_SET, "TLAS Desc set" );
    }
}

void RTGL1::ASManager::UpdateBufferDescriptors( uint32_t frameIndex )
{
    VkDescriptorBufferInfo infos[] = {
        {
            .buffer = collectorStatic->GetVertexBuffer(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = collectorDynamic[ frameIndex ]->GetVertexBuffer(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = collectorStatic->GetIndexBuffer(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = collectorDynamic[ frameIndex ]->GetIndexBuffer(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = geomInfoMgr->GetBuffer(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = geomInfoMgr->GetMatchPrevBuffer(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = previousDynamicPositions.GetBuffer(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = previousDynamicIndices.GetBuffer(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = collectorStatic->GetTexcoordBuffer_Layer1(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = collectorStatic->GetTexcoordBuffer_Layer2(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = collectorStatic->GetTexcoordBuffer_Layer3(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = collectorDynamic[ frameIndex ]->GetTexcoordBuffer_Layer1(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = collectorDynamic[ frameIndex ]->GetTexcoordBuffer_Layer2(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
        {
            .buffer = collectorDynamic[ frameIndex ]->GetTexcoordBuffer_Layer3(),
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        },
    };

    VkWriteDescriptorSet writes[] = {
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_VERTEX_BUFFER_STATIC,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_VERTEX_BUFFER_STATIC ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_VERTEX_BUFFER_DYNAMIC,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_VERTEX_BUFFER_DYNAMIC ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_INDEX_BUFFER_STATIC,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_INDEX_BUFFER_STATIC ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_INDEX_BUFFER_DYNAMIC,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_INDEX_BUFFER_DYNAMIC ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_GEOMETRY_INSTANCES,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_GEOMETRY_INSTANCES ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_GEOMETRY_INSTANCES_MATCH_PREV,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_GEOMETRY_INSTANCES_MATCH_PREV ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_PREV_POSITIONS_BUFFER_DYNAMIC,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_PREV_POSITIONS_BUFFER_DYNAMIC ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_PREV_INDEX_BUFFER_DYNAMIC,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_PREV_INDEX_BUFFER_DYNAMIC ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_STATIC_TEXCOORD_LAYER_1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_STATIC_TEXCOORD_LAYER_1 ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_STATIC_TEXCOORD_LAYER_2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_STATIC_TEXCOORD_LAYER_2 ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_STATIC_TEXCOORD_LAYER_3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_STATIC_TEXCOORD_LAYER_3 ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_DYNAMIC_TEXCOORD_LAYER_1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_DYNAMIC_TEXCOORD_LAYER_1 ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_DYNAMIC_TEXCOORD_LAYER_2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_DYNAMIC_TEXCOORD_LAYER_2 ],
        },
        {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = buffersDescSets[ frameIndex ],
            .dstBinding      = BINDING_DYNAMIC_TEXCOORD_LAYER_3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &infos[ BINDING_DYNAMIC_TEXCOORD_LAYER_3 ],
        },
    };
    assert( CheckBindings( writes ) );

    static_assert( std::size( infos ) == std::size( writes ) );

    vkUpdateDescriptorSets( device, std::size( writes ), writes, 0, nullptr );
}

void RTGL1::ASManager::UpdateASDescriptors( uint32_t frameIndex )
{
    VkAccelerationStructureKHR asHandle = tlas[ frameIndex ]->GetAS();
    assert( asHandle != VK_NULL_HANDLE );

    VkWriteDescriptorSetAccelerationStructureKHR asInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures    = &asHandle,
    };

    VkWriteDescriptorSet wrt = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext           = &asInfo,
        .dstSet          = asDescSets[ frameIndex ],
        .dstBinding      = BINDING_ACCELERATION_STRUCTURE_MAIN,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    };

    vkUpdateDescriptorSets( device, 1, &wrt, 0, nullptr );
}

RTGL1::ASManager::~ASManager()
{
    vkDestroyDescriptorPool( device, descPool, nullptr );
    vkDestroyDescriptorSetLayout( device, buffersDescSetLayout, nullptr );
    vkDestroyDescriptorSetLayout( device, asDescSetLayout, nullptr );
    vkDestroyFence( device, staticCopyFence, nullptr );
}

RTGL1::StaticGeometryToken RTGL1::ASManager::BeginStaticGeometry( bool freeReplacements )
{
    // static vertex data must be recreated, clear previous data
    // (just statics or fully, if need to erase replacements)
    collectorStatic->Reset( freeReplacements ? nullptr : &collectorStatic_replacements );
    geomInfoMgr->ResetOnlyStatic();

    // static geometry submission happens very infrequently, e.g. on level load
    vkDeviceWaitIdle( device );

    // destroy previous AS
    builtStaticInstances.clear();
    if( freeReplacements )
    {
        builtReplacements.clear();
    }

    // free AS memory
    allocStaticGeom->Reset();
    if( freeReplacements )
    {
        allocReplacementsGeom->Reset();
    }

    erase_if( curFrame_objects, []( const Object& o ) { return o.isStatic; } );

    collectorStatic->AllocateStaging( *allocator );
    assert( asBuilder->IsEmpty() );
    return StaticGeometryToken( InitAsExisting );
}

void RTGL1::ASManager::MarkReplacementsRegionEnd( const StaticGeometryToken& token )
{
    assert( token );
    collectorStatic_replacements = collectorStatic->GetCurrentRanges();
}

void RTGL1::ASManager::SubmitStaticGeometry( StaticGeometryToken& token, bool buildReplacements )
{
    assert( token );
    token = {};

    if( buildReplacements )
    {
        if( builtReplacements.empty() && builtStaticInstances.empty() )
        {
            collectorStatic->DeleteStaging();
            return;
        }
    }
    else
    {
        if( builtStaticInstances.empty() )
        {
            collectorStatic->DeleteStaging();
            return;
        }
    }

    VkCommandBuffer cmd = cmdManager->StartGraphicsCmd();

    // copy from staging with barrier
    if( buildReplacements )
    {
        collectorStatic->CopyFromStaging( cmd );
    }
    else
    {
        auto onlyStaticRange = VertexCollector::CopyRanges::RemoveAtStart(
            collectorStatic->GetCurrentRanges(), collectorStatic_replacements );

        collectorStatic->CopyFromStaging( cmd, onlyStaticRange );
    }

    assert( !asBuilder->IsEmpty() );
    asBuilder->BuildBottomLevel( cmd );

    // submit and wait
    cmdManager->Submit( cmd, staticCopyFence );
    Utils::WaitAndResetFence( device, staticCopyFence );

    collectorStatic->DeleteStaging();
}

RTGL1::DynamicGeometryToken RTGL1::ASManager::BeginDynamicGeometry( VkCommandBuffer cmd,
                                                                    uint32_t        frameIndex )
{
    // store data of current frame to use it in the next one
    CopyDynamicDataToPrevBuffers( cmd,
                                  Utils::GetPreviousByModulo( frameIndex, MAX_FRAMES_IN_FLIGHT ) );

    scratchBuffer->Reset();

    // dynamic vertices are refilled each frame
    collectorDynamic[ frameIndex ]->Reset( nullptr );
    // destroy dynamic instances from N-2
    builtDynamicInstances[ frameIndex ].clear();
    allocDynamicGeom[ frameIndex ]->Reset();

    erase_if( curFrame_objects, []( const Object& o ) { return !o.isStatic; } );

    assert( asBuilder->IsEmpty() );
    return DynamicGeometryToken( InitAsExisting );
}

auto RTGL1::ASManager::UploadAndBuildAS( const RgMeshPrimitiveInfo&     primitive,
                                         VertexCollectorFilterTypeFlags geomFlags,
                                         VertexCollector&               vertexAlloc,
                                         ChunkedStackAllocator&         accelStructAlloc,
                                         const bool isDynamic ) -> std::unique_ptr< BuiltAS >
{
    auto uploadedData = vertexAlloc.Upload( geomFlags, primitive );
    if( !uploadedData )
    {
        return {};
    }

    // NOTE: dedicated allocation, so pointers in asBuilder
    //       are valid until end of the frame
    auto newlyBuilt = new BuiltAS{
        .flags    = geomFlags,
        .blas     = BLASComponent{ device },
        .geometry = *uploadedData,
    };
    {
        const bool fastTrace = isDynamic ? false : true;

        // get AS size and create buffer for AS
        const auto buildSizes =
            ASBuilder::GetBottomBuildSizes( device,
                                            newlyBuilt->geometry.asGeometryInfo,
                                            newlyBuilt->geometry.asRange.primitiveCount,
                                            fastTrace );
        newlyBuilt->blas.RecreateIfNotValid( buildSizes, accelStructAlloc );

        // add BLAS, all passed arrays must be alive until BuildBottomLevel() call
        asBuilder->AddBLAS( newlyBuilt->blas.GetAS(),
                            { &newlyBuilt->geometry.asGeometryInfo, 1 },
                            { &newlyBuilt->geometry.asRange, 1 },
                            buildSizes,
                            fastTrace,
                            false,
                            false );
    }
    return std::unique_ptr< BuiltAS >{ newlyBuilt };
}

bool RTGL1::ASManager::AddMeshPrimitive( uint32_t                   frameIndex,
                                         const RgMeshInfo&          mesh,
                                         const RgMeshPrimitiveInfo& primitive,
                                         const PrimitiveUniqueID&   uniqueID,
                                         const bool                 isStatic,
                                         const bool                 isReplacement,
                                         const TextureManager&      textureManager,
                                         GeomInfoManager&           geomInfoManager )
{
    if( geomInfoManager.GetCount( frameIndex ) >= MAX_GEOM_INFO_COUNT )
    {
        debug::Error( "Too many geometry infos: the limit is {}", MAX_GEOM_INFO_COUNT );
        return false;
    }


    const auto geomFlags =
        VertexCollectorFilterTypeFlags_GetForGeometry( mesh, primitive, isStatic, isReplacement );

    // if exceeds a limit of geometries in a group with specified geomFlags
    if( curFrame_objects.size() >= MAX_INSTANCE_COUNT )
    {
        using FT = VertexCollectorFilterTypeFlagBits;
        debug::Error( "Too many geometries in a group ({}-{}-{}). Limit is {}",
                      uint32_t( geomFlags & FT::MASK_CHANGE_FREQUENCY_GROUP ),
                      uint32_t( geomFlags & FT::MASK_PASS_THROUGH_GROUP ),
                      uint32_t( geomFlags & FT::MASK_PRIMARY_VISIBILITY_GROUP ),
                      MAX_INSTANCE_COUNT );
        return false;
    }


    auto builtInstance = static_cast< BuiltAS* >( nullptr );

    if( isReplacement )
    {
        assert( mesh.pMeshName );
        if( auto replacePrims = find_p( builtReplacements, mesh.pMeshName ) )
        {
            // TODO: primitiveIndex must be sequential for now,
            //       do a mapping "primitiveIndex<->builtInstance" instead of vector
            if( uniqueID.primitiveIndex < replacePrims->size() )
            {
                builtInstance = ( *replacePrims )[ uniqueID.primitiveIndex ].get();
            }
            else
            {
                assert( 0 );
            }
        }
    }

    // if AS doesn't exist, create it
    if( !builtInstance )
    {
        if( isReplacement )
        {
            // expected AS to be created for a replacement
            assert( 0 );
            return false;
        }

        std ::unique_ptr< BuiltAS > created =
            UploadAndBuildAS( primitive,
                              geomFlags,
                              isStatic ? *collectorStatic : *collectorDynamic[ frameIndex ],
                              isStatic ? *allocStaticGeom : *( allocDynamicGeom[ frameIndex ] ),
                              !isStatic );
        builtInstance = created.get();

        if( isStatic )
        {
            builtStaticInstances.push_back( std::move( created ) );
        }
        else
        {
            builtDynamicInstances[ frameIndex ].push_back( std::move( created ) );
        }
    }


    // register the built instance as an instance in this frame
    curFrame_objects.push_back( Object{
        .builtInstance = builtInstance,
        .isStatic      = isStatic,
        .uniqueID      = uniqueID,
        .transform     = mesh.transform,
        .instanceFlags = geomFlags,
    } );

    // make geom info
    {
        const auto pbrInfo       = pnext::find< RgMeshPrimitivePBREXT >( &primitive );
        const auto layerTextures = textureManager.GetTexturesForLayers( primitive );
        const auto layerColors   = textureManager.GetColorForLayers( primitive );

        auto geomInfo = ShGeometryInstance{
            .model_0 = { RG_ACCESS_VEC4( mesh.transform.matrix[ 0 ] ) },
            .model_1 = { RG_ACCESS_VEC4( mesh.transform.matrix[ 1 ] ) },
            .model_2 = { RG_ACCESS_VEC4( mesh.transform.matrix[ 2 ] ) },

            .prevModel_0 = { /* set in geomInfoManager */ },
            .prevModel_1 = { /* set in geomInfoManager */ },
            .prevModel_2 = { /* set in geomInfoManager */ },

            .flags = GeomInfoManager::GetPrimitiveFlags( &mesh, primitive, !isStatic && !isReplacement ),

            .texture_base = layerTextures[ 0 ].indices[ TEXTURE_ALBEDO_ALPHA_INDEX ],
            .texture_base_ORM =
                layerTextures[ 0 ].indices[ TEXTURE_OCCLUSION_ROUGHNESS_METALLIC_INDEX ],
            .texture_base_N = layerTextures[ 0 ].indices[ TEXTURE_NORMAL_INDEX ],
            .texture_base_E = layerTextures[ 0 ].indices[ TEXTURE_EMISSIVE_INDEX ],

            .texture_layer1 = layerTextures[ 1 ].indices[ TEXTURE_ALBEDO_ALPHA_INDEX ],
            .texture_layer2 = layerTextures[ 2 ].indices[ TEXTURE_ALBEDO_ALPHA_INDEX ],
            .texture_layer3 = layerTextures[ 3 ].indices[ TEXTURE_ALBEDO_ALPHA_INDEX ],

            .colorFactor_base   = layerColors[ 0 ],
            .colorFactor_layer1 = layerColors[ 1 ],
            .colorFactor_layer2 = layerColors[ 2 ],
            .colorFactor_layer3 = layerColors[ 3 ],

            .baseVertexIndex     = builtInstance->geometry.firstVertex,
            .baseIndexIndex      = builtInstance->geometry.firstIndex
                                       ? *builtInstance->geometry.firstIndex
                                       : UINT32_MAX,
            .prevBaseVertexIndex = { /* set in geomInfoManager */ },
            .prevBaseIndexIndex  = { /* set in geomInfoManager */ },
            .vertexCount         = primitive.vertexCount,
            .indexCount = builtInstance->geometry.firstIndex ? primitive.indexCount : UINT32_MAX,

            .texture_base_D = layerTextures[ 0 ].indices[ TEXTURE_HEIGHT_INDEX ],

            .roughnessDefault_metallicDefault =
                ( floatToUint8( pbrInfo ? pbrInfo->roughnessDefault : 1.0f ) << 0 ) |
                ( floatToUint8( pbrInfo ? pbrInfo->metallicDefault : 0.0f ) << 8 ),

            .emissiveMult = Utils::Saturate( primitive.emissive ),

            // values ignored if doesn't exist
            .firstVertex_Layer1 = builtInstance->geometry.firstVertex_Layer1,
            .firstVertex_Layer2 = builtInstance->geometry.firstVertex_Layer2,
            .firstVertex_Layer3 = builtInstance->geometry.firstVertex_Layer3,
        };

        // global geometry index -- for indexing in geom infos buffer
        // local geometry index -- index of geometry in BLAS
        geomInfoManager.WriteGeomInfo( frameIndex,
                                       uniqueID,
                                       geomInfo,
                                       isStatic,
                                       ( primitive.flags & RG_MESH_PRIMITIVE_NO_MOTION_VECTORS ) );
    }

    return true;
}

void RTGL1::ASManager::Hack_PatchTexturesForStaticPrimitive( const PrimitiveUniqueID& uniqueID,
                                                             const char*              pTextureName,
                                                             const TextureManager& textureManager )
{
    const auto textures_layer0 = textureManager.GetMaterialTextures( pTextureName );

    geomInfoMgr->Hack_PatchGeomInfoTexturesForStatic(
        uniqueID,
        textures_layer0.indices[ TEXTURE_ALBEDO_ALPHA_INDEX ],
        textures_layer0.indices[ TEXTURE_OCCLUSION_ROUGHNESS_METALLIC_INDEX ],
        textures_layer0.indices[ TEXTURE_NORMAL_INDEX ],
        textures_layer0.indices[ TEXTURE_EMISSIVE_INDEX ],
        textures_layer0.indices[ TEXTURE_HEIGHT_INDEX ] );
}

void RTGL1::ASManager::Hack_PatchGeomInfoTransformForStatic( const PrimitiveUniqueID& geomUniqueID,
                                                             const RgTransform&       transform )
{
    for( Object& obj : curFrame_objects )
    {
        if( obj.isStatic && obj.uniqueID == geomUniqueID )
        {
            obj.transform = transform;
            break;
        }
    }

    geomInfoMgr->Hack_PatchGeomInfoTransformForStatic( geomUniqueID, transform );
}

void RTGL1::ASManager::CacheReplacement( std::string_view           meshName,
                                         const RgMeshPrimitiveInfo& primitive,
                                         uint32_t                   index )
{
    constexpr bool isReplacement = true;
    constexpr bool isStatic      = false;
    constexpr bool isDynamic     = false;

    const auto geomFlags =
        VertexCollectorFilterTypeFlags_GetForGeometry( {}, primitive, isStatic, isReplacement );

    std::unique_ptr< BuiltAS > builtInstance = UploadAndBuildAS(
        primitive, geomFlags, *collectorStatic, *allocReplacementsGeom, isDynamic );

    if( !builtInstance )
    {
        debug::Warning( "Failed to upload vertex data of a replacement primitive" );
        return;
    }

    // TODO: look at the note above on primitiveIndex
    assert( builtReplacements[ meshName ].size() == index );
    builtReplacements[ meshName ].push_back( std::move( builtInstance ) );
}

void RTGL1::ASManager::SubmitDynamicGeometry( DynamicGeometryToken& token,
                                              VkCommandBuffer       cmd,
                                              uint32_t              frameIndex )
{
    assert( token );
    token = {};

    {
        auto label = CmdLabel{ cmd, "Vertex data" };
        collectorDynamic[ frameIndex ]->CopyFromStaging( cmd );
    }


    if( asBuilder->BuildBottomLevel( cmd ) )
    {
        // sync AS access
        Utils::ASBuildMemoryBarrier( cmd );
    }
}

auto RTGL1::ASManager::MakeVkTLAS( const BuiltAS&                 builtAS,
                                   uint32_t                       rayCullMaskWorld,
                                   const RgTransform&             instanceTransform,
                                   VertexCollectorFilterTypeFlags instanceFlags )
    -> std::optional< VkAccelerationStructureInstanceKHR >
{
    using FT = VertexCollectorFilterTypeFlagBits;

    auto rgToVkTransform = []( const RgTransform& t ) {
        return VkTransformMatrixKHR{ {
            { t.matrix[ 0 ][ 0 ], t.matrix[ 0 ][ 1 ], t.matrix[ 0 ][ 2 ], t.matrix[ 0 ][ 3 ] },
            { t.matrix[ 1 ][ 0 ], t.matrix[ 1 ][ 1 ], t.matrix[ 1 ][ 2 ], t.matrix[ 1 ][ 3 ] },
            { t.matrix[ 2 ][ 0 ], t.matrix[ 2 ][ 1 ], t.matrix[ 2 ][ 2 ], t.matrix[ 2 ][ 3 ] },
        } };
    };


    if( builtAS.blas.GetAS() == VK_NULL_HANDLE )
    {
        assert( 0 );
        return {};
    }


    auto instance = VkAccelerationStructureInstanceKHR{
        .transform                              = rgToVkTransform( instanceTransform ),
        .instanceCustomIndex                    = 0,
        .mask                                   = 0,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags                                  = 0,
        .accelerationStructureReference         = builtAS.blas.GetASAddress(),
    };

    // inherit some bits from an instance
    const auto filter =
        builtAS.flags | ( instanceFlags & ( FT::PV_FIRST_PERSON | FT::PV_FIRST_PERSON_VIEWER |
                                            // SHIPPING_HACK
                                            FT::PT_REFRACT ) );
                                            // SHIPPING_HACK


    if( filter & FT::PV_FIRST_PERSON )
    {
        instance.mask = INSTANCE_MASK_FIRST_PERSON;
        instance.instanceCustomIndex |= INSTANCE_CUSTOM_INDEX_FLAG_FIRST_PERSON;
    }
    else if( filter & FT::PV_FIRST_PERSON_VIEWER )
    {
        instance.mask = INSTANCE_MASK_FIRST_PERSON_VIEWER;
        instance.instanceCustomIndex |= INSTANCE_CUSTOM_INDEX_FLAG_FIRST_PERSON_VIEWER;
    }
    else
    {
        // also check rayCullMaskWorld, if world part is not included in the cull mask,
        // then don't add it to BLAS at all, it helps culling PT_REFLECT if it was a world part

        if( filter & FT::PV_WORLD_0 )
        {
            instance.mask = INSTANCE_MASK_WORLD_0;

            if( !( rayCullMaskWorld & INSTANCE_MASK_WORLD_0 ) )
            {
                return {};
            }
        }
        else if( filter & FT::PV_WORLD_1 )
        {
            instance.mask = INSTANCE_MASK_WORLD_1;
        }
        else if( filter & FT::PV_WORLD_2 )
        {
            instance.mask = INSTANCE_MASK_WORLD_2;
            instance.instanceCustomIndex |= INSTANCE_CUSTOM_INDEX_FLAG_SKY;

            if( !( rayCullMaskWorld & INSTANCE_MASK_WORLD_2 ) )
            {
                return {};
            }
        }
        else
        {
            assert( 0 );
        }
    }


    if( filter & FT::PT_REFRACT )
    {
        // don't touch first-person
        bool isworld =
            !( filter & FT::PV_FIRST_PERSON ) && !( filter & FT::PV_FIRST_PERSON_VIEWER );

        if( isworld )
        {
            // completely rewrite mask, ignoring INSTANCE_MASK_WORLD_*,
            // if mask contains those world bits, then (mask & (~INSTANCE_MASK_REFRACT))
            // won't actually cull INSTANCE_MASK_REFRACT
            instance.mask = INSTANCE_MASK_REFRACT;
        }
    }


    if( filter & FT::PT_ALPHA_TESTED )
    {
        instance.instanceShaderBindingTableRecordOffset = SBT_INDEX_HITGROUP_ALPHA_TESTED;
        instance.flags = VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR |
                         VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR /*|
                         VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR*/
            ;
    }
    else
    {
        assert( ( filter & FT::PT_OPAQUE ) || ( filter & FT::PT_REFRACT ) );

        instance.instanceShaderBindingTableRecordOffset = SBT_INDEX_HITGROUP_FULLY_OPAQUE;
        instance.flags = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR |
                         VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR /*|
                         VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR*/
            ;
    }


    return instance;
}


auto RTGL1::ASManager::MakeUniqueIDToTlasID( bool disableRTGeometry ) const -> UniqueIDToTlasID
{
    auto all = UniqueIDToTlasID{};
    if( !disableRTGeometry )
    {
        all.reserve( curFrame_objects.size() );
        for( uint32_t i = 0; i < curFrame_objects.size(); i++ )
        {
            all[ curFrame_objects[ i ].uniqueID ] = i;
        }
    }
    return all;
}

void RTGL1::ASManager::BuildTLAS( VkCommandBuffer cmd,
                                  uint32_t        frameIndex,
                                  uint32_t        uniformData_rayCullMaskWorld,
                                  bool            disableRTGeometry )
{
    auto label = CmdLabel{ cmd, "Building TLAS" };


    auto allVkTlas = std::vector< VkAccelerationStructureInstanceKHR >{};
    if( !disableRTGeometry )
    {
        allVkTlas.reserve( curFrame_objects.size() );
        for( const auto& obj : curFrame_objects )
        {
            auto vkTlas = MakeVkTLAS( *obj.builtInstance,
                                      uniformData_rayCullMaskWorld,
                                      obj.transform,
                                      obj.instanceFlags );

            if( !vkTlas )
            {
                debug::Error( "MakeVkTLAS has failed for UniqueID {}-{}",
                              obj.uniqueID.objectId,
                              obj.uniqueID.primitiveIndex );
                allVkTlas.clear();
                break;
            }

            allVkTlas.push_back( *vkTlas );
        }
    }
    assert( MakeUniqueIDToTlasID( disableRTGeometry ).size() == allVkTlas.size() );


    if( !allVkTlas.empty() )
    {
        // fill buffer
        auto mapped =
            instanceBuffer->GetMappedAs< VkAccelerationStructureInstanceKHR* >( frameIndex );

        memcpy( mapped,
                allVkTlas.data(),
                allVkTlas.size() * sizeof( VkAccelerationStructureInstanceKHR ) );

        instanceBuffer->CopyFromStaging( cmd, frameIndex );
    }


    auto instGeom = VkAccelerationStructureGeometryKHR{
        .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry     = {
            .instances = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .arrayOfPointers = VK_FALSE,
                .data = {
                    .deviceAddress = instanceBuffer->GetDeviceAddress() ,
                },
            },
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    auto range = VkAccelerationStructureBuildRangeInfoKHR{
        .primitiveCount = static_cast< uint32_t >( allVkTlas.size() ),
    };

    constexpr bool fastTrace = true;

    bool tlasWasRecreated = false;

    TLASComponent* curTlas = tlas[ frameIndex ].get();
    {
        // get AS size and create buffer for AS
        const auto buildSizes = ASBuilder::GetTopBuildSizes(
            device, instGeom, static_cast< uint32_t >( allVkTlas.size() ), fastTrace );

        // if previous buffer's size is not enough
        tlasWasRecreated = curTlas->RecreateIfNotValid( buildSizes, *( allocTlas[ frameIndex ] ), true );

        // ASBuilder requires 'instGeom', 'range' to be alive
        assert( asBuilder->IsEmpty() );
        asBuilder->AddTLAS( curTlas->GetAS(), &instGeom, &range, buildSizes, fastTrace, false );
        asBuilder->BuildTopLevel( cmd );
    }


    // sync AS access
    Utils::ASBuildMemoryBarrier( cmd );


    if( tlasWasRecreated )
    {
        UpdateASDescriptors( frameIndex );
    }
}

void RTGL1::ASManager::CopyDynamicDataToPrevBuffers( VkCommandBuffer cmd, uint32_t frameIndex )
{
    uint32_t vertCount  = collectorDynamic[ frameIndex ]->GetCurrentVertexCount();
    uint32_t indexCount = collectorDynamic[ frameIndex ]->GetCurrentIndexCount();

    if( vertCount > 0 )
    {
        VkBufferCopy vertRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size      = vertCount * sizeof( ShVertex ),
        };

        vkCmdCopyBuffer( cmd,
                         collectorDynamic[ frameIndex ]->GetVertexBuffer(),
                         previousDynamicPositions.GetBuffer(),
                         1,
                         &vertRegion );
    }

    if( indexCount > 0 )
    {
        VkBufferCopy indexRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size      = indexCount * sizeof( uint32_t ),
        };

        vkCmdCopyBuffer( cmd,
                         collectorDynamic[ frameIndex ]->GetIndexBuffer(),
                         previousDynamicIndices.GetBuffer(),
                         1,
                         &indexRegion );
    }
}

void RTGL1::ASManager::OnVertexPreprocessingBegin( VkCommandBuffer cmd,
                                                   uint32_t        frameIndex,
                                                   bool            onlyDynamic )
{
    if( !onlyDynamic )
    {
        collectorStatic->InsertVertexPreprocessBarrier( cmd, true );
    }

    collectorDynamic[ frameIndex ]->InsertVertexPreprocessBarrier( cmd, true );
}

void RTGL1::ASManager::OnVertexPreprocessingFinish( VkCommandBuffer cmd,
                                                    uint32_t        frameIndex,
                                                    bool            onlyDynamic )
{
    if( !onlyDynamic )
    {
        collectorStatic->InsertVertexPreprocessBarrier( cmd, false );
    }

    collectorDynamic[ frameIndex ]->InsertVertexPreprocessBarrier( cmd, false );
}

VkDescriptorSet RTGL1::ASManager::GetBuffersDescSet( uint32_t frameIndex ) const
{
    return buffersDescSets[ frameIndex ];
}

VkDescriptorSet RTGL1::ASManager::GetTLASDescSet( uint32_t frameIndex ) const
{
    // if TLAS wasn't built, return null
    if( tlas[ frameIndex ]->GetAS() == VK_NULL_HANDLE )
    {
        return VK_NULL_HANDLE;
    }

    return asDescSets[ frameIndex ];
}

VkDescriptorSetLayout RTGL1::ASManager::GetBuffersDescSetLayout() const
{
    return buffersDescSetLayout;
}

VkDescriptorSetLayout RTGL1::ASManager::GetTLASDescSetLayout() const
{
    return asDescSetLayout;
}
