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

#include "RTGL1/RTGL1.h"

#include <functional>

namespace RTGL1
{

constexpr uint32_t VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_CF = 0;
constexpr uint32_t VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PT = 3;
constexpr uint32_t VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PV = 6;

// clang-format off
enum class VertexCollectorFilterTypeFlagBits : uint32_t
{
    NONE = 0,

    CF_STATIC_NON_MOVABLE         = 0b00000001 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_CF,
    CF_REPLACEMENT                = 0b00000010 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_CF,
    CF_DYNAMIC                    = 0b00000100 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_CF,
    MASK_CHANGE_FREQUENCY_GROUP   = CF_STATIC_NON_MOVABLE | CF_REPLACEMENT | CF_DYNAMIC,

    PT_OPAQUE                     = 0b00000001 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PT,
    PT_ALPHA_TESTED               = 0b00000010 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PT,
    PT_REFRACT                    = 0b00000100 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PT,
    MASK_PASS_THROUGH_GROUP       = PT_OPAQUE | PT_ALPHA_TESTED | PT_REFRACT,

    PV_WORLD_0                    = 0b00000001 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PV,
    PV_WORLD_1                    = 0b00000010 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PV,
    PV_WORLD_2                    = 0b00000100 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PV,
    PV_FIRST_PERSON               = 0b00001000 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PV,
    PV_FIRST_PERSON_VIEWER        = 0b00010000 << VERTEX_COLLECTOR_FILTER_TYPE_BIT_OFFSET_PV,
    MASK_PRIMARY_VISIBILITY_GROUP = PV_WORLD_0 | PV_WORLD_1 | PV_WORLD_2 | PV_FIRST_PERSON | PV_FIRST_PERSON_VIEWER,
};
// clang-format on
using VertexCollectorFilterTypeFlags = uint32_t;


constexpr VertexCollectorFilterTypeFlagBits VertexCollectorFilterGroup_ChangeFrequency[] = {
    VertexCollectorFilterTypeFlagBits::CF_STATIC_NON_MOVABLE,
    VertexCollectorFilterTypeFlagBits::CF_REPLACEMENT,
    VertexCollectorFilterTypeFlagBits::CF_DYNAMIC,
};

constexpr VertexCollectorFilterTypeFlagBits VertexCollectorFilterGroup_PassThrough[] = {
    VertexCollectorFilterTypeFlagBits::PT_OPAQUE,
    VertexCollectorFilterTypeFlagBits::PT_ALPHA_TESTED,
    VertexCollectorFilterTypeFlagBits::PT_REFRACT,
};

constexpr VertexCollectorFilterTypeFlagBits VertexCollectorFilterGroup_PrimaryVisibility[] = {
    VertexCollectorFilterTypeFlagBits::PV_WORLD_0,
    VertexCollectorFilterTypeFlagBits::PV_WORLD_1,
    VertexCollectorFilterTypeFlagBits::PV_WORLD_2,
    VertexCollectorFilterTypeFlagBits::PV_FIRST_PERSON,
    VertexCollectorFilterTypeFlagBits::PV_FIRST_PERSON_VIEWER,
};


inline constexpr VertexCollectorFilterTypeFlags operator|( VertexCollectorFilterTypeFlagBits a,
                                                           VertexCollectorFilterTypeFlagBits b )
{
    return VertexCollectorFilterTypeFlags( a ) | VertexCollectorFilterTypeFlags( b );
}

inline constexpr VertexCollectorFilterTypeFlags operator|( VertexCollectorFilterTypeFlags    a,
                                                           VertexCollectorFilterTypeFlagBits b )
{
    return a | VertexCollectorFilterTypeFlags( b );
}

inline constexpr VertexCollectorFilterTypeFlags operator|( VertexCollectorFilterTypeFlagBits a,
                                                           VertexCollectorFilterTypeFlags    b )
{
    return VertexCollectorFilterTypeFlags( a ) | b;
}

inline constexpr VertexCollectorFilterTypeFlags operator&( VertexCollectorFilterTypeFlagBits a,
                                                           VertexCollectorFilterTypeFlagBits b )
{
    return VertexCollectorFilterTypeFlags( a ) & VertexCollectorFilterTypeFlags( b );
}

inline constexpr VertexCollectorFilterTypeFlags operator&( VertexCollectorFilterTypeFlags    a,
                                                           VertexCollectorFilterTypeFlagBits b )
{
    return a & VertexCollectorFilterTypeFlags( b );
}

inline constexpr VertexCollectorFilterTypeFlags operator&( VertexCollectorFilterTypeFlagBits a,
                                                           VertexCollectorFilterTypeFlags    b )
{
    return VertexCollectorFilterTypeFlags( a ) & b;
}

const char* VertexCollectorFilterTypeFlags_GetNameForBLAS( VertexCollectorFilterTypeFlags flags );

auto VertexCollectorFilterTypeFlags_GetForGeometry( const RgMeshInfo&          mesh,
                                                    const RgMeshPrimitiveInfo& primitive,
                                                    bool                       isStatic,
                                                    bool                       isReplacement )
    -> VertexCollectorFilterTypeFlags;

}