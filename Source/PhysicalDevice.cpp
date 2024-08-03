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

#include "PhysicalDevice.h"

#include <string>
#include <vector>

#include "RgException.h"

using namespace RTGL1;

PhysicalDevice::PhysicalDevice( VkInstance instance )
    : physDevice( VK_NULL_HANDLE ), memoryProperties{}, rtPipelineProperties{}, asProperties{}
{
    std::vector< VkPhysicalDevice > physicalDevices;
    {
        uint32_t physCount = 0;
        {
            VkResult r = vkEnumeratePhysicalDevices( instance, &physCount, nullptr );
            VK_CHECKERROR( r );
        }

        if( physCount == 0 )
        {
            throw RgException( RG_RESULT_CANT_FIND_SUPPORTED_PHYSICAL_DEVICE,
                               "Can't find physical devices" );
        }
        physicalDevices.resize( physCount );

        VkResult r = vkEnumeratePhysicalDevices( instance, &physCount, physicalDevices.data() );
        VK_CHECKERROR( r );
    }

    for( VkPhysicalDevice p : physicalDevices )
    {
        auto positionFetchFeatures = VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR,
            .pNext = nullptr,
        };
        auto rayQueryFeatures = VkPhysicalDeviceRayQueryFeaturesKHR{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
            .pNext = &positionFetchFeatures,
        };
        auto rtFeatures = VkPhysicalDeviceRayTracingPipelineFeaturesKHR{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
            .pNext = &rayQueryFeatures,
        };
        auto deviceFeatures2 = VkPhysicalDeviceFeatures2{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &rtFeatures,
        };
        vkGetPhysicalDeviceFeatures2( p, &deviceFeatures2 );


        if( rtFeatures.rayTracingPipeline )
        {
            physDevice = p;

            supportsRayQuery      = rayQueryFeatures.rayQuery;
            supportsPositionFetch = positionFetchFeatures.rayTracingPositionFetch;

            asProperties = VkPhysicalDeviceAccelerationStructurePropertiesKHR{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR
            };
            rtPipelineProperties = VkPhysicalDeviceRayTracingPipelinePropertiesKHR{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
                .pNext = &asProperties,
            };
            auto deviceProp2 = VkPhysicalDeviceProperties2{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = &rtPipelineProperties,
            };

            vkGetPhysicalDeviceProperties2( physDevice, &deviceProp2 );
            vkGetPhysicalDeviceMemoryProperties( physDevice, &memoryProperties );

            break;
        }
    }

    if( physDevice == VK_NULL_HANDLE )
    {
        throw RgException( RG_RESULT_CANT_FIND_SUPPORTED_PHYSICAL_DEVICE,
                           "Can't find physical device with ray tracing support" );
    }
}

VkPhysicalDevice PhysicalDevice::Get() const
{
    return physDevice;
}

auto PhysicalDevice::GetMemoryTypeIndex( uint32_t memoryTypeBits, VkFlags requirementsMask ) const
    -> std::optional< uint32_t >
{
    VkMemoryPropertyFlags flagsToIgnore = 0;

    if( requirementsMask & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
    {
        // device-local memory must not be host visible
        flagsToIgnore = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    else
    {
        // host visible memory must not be device-local
        flagsToIgnore = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }


    // for each memory type available for this device
    for( uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++ )
    {
        // if type is available
        if( ( memoryTypeBits & 1u ) == 1 )
        {
            VkMemoryPropertyFlags flags = memoryProperties.memoryTypes[ i ].propertyFlags;

            bool                  isSuitable = ( flags & requirementsMask ) == requirementsMask;
            bool                  isIgnored  = ( flags & flagsToIgnore ) == flagsToIgnore;

            if( isSuitable && !isIgnored )
            {
                return i;
            }
        }

        memoryTypeBits >>= 1u;
    }

    debug::Error( "Can't find memory type for given memory property flags ({})", requirementsMask );
    return std::nullopt;
}

const VkPhysicalDeviceMemoryProperties& PhysicalDevice::GetMemoryProperties() const
{
    return memoryProperties;
}

const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& PhysicalDevice::GetRTPipelineProperties()
    const
{
    return rtPipelineProperties;
}

const VkPhysicalDeviceAccelerationStructurePropertiesKHR& PhysicalDevice::GetASProperties() const
{
    return asProperties;
}

auto RTGL1::PhysicalDevice::GetLUID() const -> std::optional< uint64_t >
{
#ifdef RG_USE_DX12
    auto id = VkPhysicalDeviceIDProperties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
        .pNext = nullptr,
    };

    auto info = VkPhysicalDeviceProperties2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &id,
    };

    vkGetPhysicalDeviceProperties2( physDevice, &info );

    if( !id.deviceLUIDValid )
    {
        assert( 0 );
        return std::nullopt;
    }

    uint64_t luid = 0;

    static_assert( sizeof( id.deviceLUID ) == sizeof( luid ) );
    memcpy( &luid, id.deviceLUID, sizeof( luid ) );

    return luid;

#else
    assert( 0 );
    return {};
#endif
}
