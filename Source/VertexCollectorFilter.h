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

#include <vector>

#include "Common.h"
#include "VertexCollectorFilterType.h"

namespace RTGL1
{

// Instances of this class are added to VertexCollector to
// collect AS data separately for specific filter types.
class VertexCollectorFilter
{
public:
    explicit VertexCollectorFilter( VertexCollectorFilterTypeFlags _filter ) : filter( _filter ) {}
    ~VertexCollectorFilter() = default;

    VertexCollectorFilter( const VertexCollectorFilter& other )            = delete;
    VertexCollectorFilter& operator=( const VertexCollectorFilter& other ) = delete;
    VertexCollectorFilter( VertexCollectorFilter&& other )                 = delete;
    VertexCollectorFilter& operator=( VertexCollectorFilter&& other )      = delete;

    auto GetPrimitiveCounts() const -> const std::vector< uint32_t >&;
    auto GetASGeometries() const -> const std::vector< VkAccelerationStructureGeometryKHR >&;
    auto GetASBuildRangeInfos() const
        -> const std::vector< VkAccelerationStructureBuildRangeInfoKHR >&;

    void Reset();

    auto PushGeometry( VertexCollectorFilterTypeFlags                  type,
                       const VkAccelerationStructureGeometryKHR&       geom,
                       const VkAccelerationStructureBuildRangeInfoKHR& rangeInfo ) -> uint32_t;

    auto FilterFlags() const -> VertexCollectorFilterTypeFlags { return filter; }
    auto GetGeometryCount() const -> uint32_t;

private:
    VertexCollectorFilterTypeFlags filter;

    std::vector< uint32_t >                                 primitiveCounts;
    std::vector< VkAccelerationStructureGeometryKHR >       asGeometries;
    std::vector< VkAccelerationStructureBuildRangeInfoKHR > asBuildRangeInfos;
};

}