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

#ifdef DESC_SET_GLOBAL_UNIFORM
#ifdef DESC_SET_VERTEX_DATA
layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_VERTEX_BUFFER_STATIC)
    #ifndef VERTEX_BUFFER_WRITEABLE
    readonly 
    #endif
    buffer VertexBufferStatic_BT
{
    ShVertex g_staticVertices[];
};

layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_VERTEX_BUFFER_DYNAMIC)
    #ifndef VERTEX_BUFFER_WRITEABLE
    readonly 
    #endif
    buffer VertexBufferDynamic_BT
{
    ShVertex g_dynamicVertices[];
};

layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_INDEX_BUFFER_STATIC)
    readonly 
    buffer IndexBufferStatic_BT
{
    uint staticIndices[];
};

layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_INDEX_BUFFER_DYNAMIC)
    readonly 
    buffer IndexBufferDynamic_BT
{
    uint dynamicIndices[];
};

layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_GEOMETRY_INSTANCES)
    readonly 
    buffer GeometryInstances_BT
{
    ShGeometryInstance geometryInstances[];
};

layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_GEOMETRY_INSTANCES_MATCH_PREV)
    readonly 
    buffer GeometryIndicesPrevToCur_BT
{
    int geomIndexPrevToCur[];
};

layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_PREV_POSITIONS_BUFFER_DYNAMIC)
    #ifndef VERTEX_BUFFER_WRITEABLE
    readonly 
    #endif
    buffer PrevPositionsBufferDynamic_BT
{
    ShVertex g_dynamicVertices_Prev[];
};

layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_PREV_INDEX_BUFFER_DYNAMIC)
    #ifndef VERTEX_BUFFER_WRITEABLE
    readonly 
    #endif
    buffer PrevIndexBufferDynamic_BT
{
    uint prevDynamicIndices[];
};

layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_STATIC_TEXCOORD_LAYER_1)
    readonly
    buffer StaticTexCoordLayer1_BT
{
    vec2 g_staticTexCoords_Layer1[];
};
layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_STATIC_TEXCOORD_LAYER_2)
    readonly
    buffer StaticTexCoordLayer2_BT
{
    vec2 g_staticTexCoords_Layer2[];
};
layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_STATIC_TEXCOORD_LAYER_3)
    readonly
    buffer StaticTexCoordLayer3_BT
{
    vec2 g_staticTexCoords_Layer3[];
};
layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_DYNAMIC_TEXCOORD_LAYER_1)
    readonly
    buffer DynamicTexCoordLayer1_BT
{
    vec2 g_dynamicTexCoords_Layer1[];
};
layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_DYNAMIC_TEXCOORD_LAYER_2)
    readonly
    buffer DynamicTexCoordLayer2_BT
{
    vec2 g_dynamicTexCoords_Layer2[];
};
layout(
    set = DESC_SET_VERTEX_DATA,
    binding = BINDING_DYNAMIC_TEXCOORD_LAYER_3)
    readonly
    buffer DynamicTexCoordLayer3_BT
{
    vec2 g_dynamicTexCoords_Layer3[];
};


vec3 getStaticVerticesPositions(uint index)
{
    return g_staticVertices[index].position.xyz;
}

vec3 getStaticVerticesNormals(uint index)
{
    return g_staticVertices[index].normal.xyz;
}

vec3 getDynamicVerticesPositions(uint index)
{
    return g_dynamicVertices[index].position.xyz;
}

vec3 getDynamicVerticesNormals(uint index)
{
    return g_dynamicVertices[index].normal.xyz;
}

#ifdef VERTEX_BUFFER_WRITEABLE
void setStaticVerticesNormals(uint index, vec3 value)
{
    g_staticVertices[index].normal = vec4(value, 0.0);
}

void setDynamicVerticesNormals(uint index, vec3 value)
{
    g_dynamicVertices[index].normal = vec4(value, 0.0);
}
#endif // VERTEX_BUFFER_WRITEABLE

// Get indices in vertex buffer. If geom uses index buffer then it flattens them to vertex buffer indices.
uvec3 getVertIndicesStatic(uint baseVertexIndex, uint baseIndexIndex, uint primitiveId)
{
    // if to use indices
    if (baseIndexIndex != UINT32_MAX)
    {
        return uvec3(
            baseVertexIndex + staticIndices[baseIndexIndex + primitiveId * 3 + 0],
            baseVertexIndex + staticIndices[baseIndexIndex + primitiveId * 3 + 1],
            baseVertexIndex + staticIndices[baseIndexIndex + primitiveId * 3 + 2]);
    }
    else
    {
        return uvec3(
            baseVertexIndex + primitiveId * 3 + 0,
            baseVertexIndex + primitiveId * 3 + 1,
            baseVertexIndex + primitiveId * 3 + 2);
    }
}

uvec3 getVertIndicesDynamic(uint baseVertexIndex, uint baseIndexIndex, uint primitiveId)
{
    // if to use indices
    if (baseIndexIndex != UINT32_MAX)
    {
        return uvec3(
            baseVertexIndex + dynamicIndices[baseIndexIndex + primitiveId * 3 + 0],
            baseVertexIndex + dynamicIndices[baseIndexIndex + primitiveId * 3 + 1],
            baseVertexIndex + dynamicIndices[baseIndexIndex + primitiveId * 3 + 2]);
    }
    else
    {
        return uvec3(
            baseVertexIndex + primitiveId * 3 + 0,
            baseVertexIndex + primitiveId * 3 + 1,
            baseVertexIndex + primitiveId * 3 + 2);
    }
}

// Only for dynamic, static geom vertices are not changed.
uvec3 getPrevVertIndicesDynamic(uint prevBaseVertexIndex, uint prevBaseIndexIndex, uint primitiveId)
{
    // if to use indices
    if (prevBaseIndexIndex != UINT32_MAX)
    {
        return uvec3(
            prevBaseVertexIndex + prevDynamicIndices[prevBaseIndexIndex + primitiveId * 3 + 0],
            prevBaseVertexIndex + prevDynamicIndices[prevBaseIndexIndex + primitiveId * 3 + 1],
            prevBaseVertexIndex + prevDynamicIndices[prevBaseIndexIndex + primitiveId * 3 + 2]);
    }
    else
    {
        return uvec3(
            prevBaseVertexIndex + primitiveId * 3 + 0,
            prevBaseVertexIndex + primitiveId * 3 + 1,
            prevBaseVertexIndex + primitiveId * 3 + 2);
    }
}

vec3 getPrevDynamicVerticesPositions(uint index)
{
    return g_dynamicVertices_Prev[index].position.xyz;
}

vec4 getTangent(const mat3 localPos, const vec3 normal, const mat3x2 texCoord)
{
    const vec3 e1 = localPos[1] - localPos[0];
    const vec3 e2 = localPos[2] - localPos[0];

    const vec2 u1 = texCoord[1] - texCoord[0];
    const vec2 u2 = texCoord[2] - texCoord[0];

    const float invDet = 1.0 / (u1.x * u2.y - u2.x * u1.y);

    const vec3 tangent   = normalize((e1 * u2.y - e2 * u1.y) * invDet);
    const vec3 bitangent = normalize((e2 * u1.x - e1 * u2.x) * invDet);

    // Don't store bitangent, only store cross(normal, tangent) handedness.
    // If that cross product and bitangent have the same sign,
    // then handedness is 1, otherwise -1
    float handedness = float(dot(cross(normal, tangent), bitangent) > 0.0);
    handedness = handedness * 2.0 - 1.0;

    return vec4(tangent, handedness);
}

ShTriangle makeTriangle(const ShVertex a, const ShVertex b, const ShVertex c)
{    
    ShTriangle tr;

    tr.positions[0] = a.position.xyz;
    tr.positions[1] = b.position.xyz;
    tr.positions[2] = c.position.xyz;

    tr.normals[0] = a.normal.xyz;
    tr.normals[1] = b.normal.xyz;
    tr.normals[2] = c.normal.xyz;

    tr.layerTexCoord[0][0] = a.texCoord;
    tr.layerTexCoord[0][1] = b.texCoord;
    tr.layerTexCoord[0][2] = c.texCoord;

    tr.vertexColors[0] = a.color;
    tr.vertexColors[1] = b.color;
    tr.vertexColors[2] = c.color;

    // get very coarse normal for triangle to determine bitangent's handedness
    tr.tangent = getTangent(tr.positions, safeNormalize(tr.normals[0] + tr.normals[1] + tr.normals[2]), tr.layerTexCoord[0]);

    return tr;
}

bool getCurrentGeometryIndexByPrev(int prevInstanceID, int prevLocalGeometryIndex, out int curFrameGlobalGeomIndex)
{
    // try to find instance index in current frame by it
    curFrameGlobalGeomIndex = geomIndexPrevToCur[prevInstanceID];

    // -1 : no prev to cur exist
    return curFrameGlobalGeomIndex >= 0;
}

// instanceID  - index in TLAS
// primitiveId - index of a triangle
ShTriangle getTriangle(int instanceID, int instanceCustomIndex, int localGeometryIndex, int primitiveId)
{
    ShTriangle tr;

    const ShGeometryInstance inst = geometryInstances[instanceID];

    const bool isDynamic = (instanceCustomIndex & INSTANCE_CUSTOM_INDEX_FLAG_DYNAMIC) == INSTANCE_CUSTOM_INDEX_FLAG_DYNAMIC;

    if (isDynamic)
    {
        {
            const uvec3 vertIndices = getVertIndicesDynamic(inst.baseVertexIndex, inst.baseIndexIndex, primitiveId);

            tr = makeTriangle(
                g_dynamicVertices[vertIndices[0]],
                g_dynamicVertices[vertIndices[1]],
                g_dynamicVertices[vertIndices[2]]);
        }

        if( ( inst.flags & GEOM_INST_FLAG_EXISTS_LAYER1 ) != 0 )
        {
            const uvec3 vertIndices =
                getVertIndicesDynamic( inst.firstVertex_Layer1, inst.baseIndexIndex, primitiveId );
            tr.layerTexCoord[ 1 ][ 0 ] = g_dynamicTexCoords_Layer1[ vertIndices[ 0 ] ];
            tr.layerTexCoord[ 1 ][ 1 ] = g_dynamicTexCoords_Layer1[ vertIndices[ 1 ] ];
            tr.layerTexCoord[ 1 ][ 2 ] = g_dynamicTexCoords_Layer1[ vertIndices[ 2 ] ];
        }
        if( ( inst.flags & GEOM_INST_FLAG_EXISTS_LAYER2 ) != 0 )
        {
            const uvec3 vertIndices =
                getVertIndicesDynamic( inst.firstVertex_Layer2, inst.baseIndexIndex, primitiveId );
            tr.layerTexCoord[ 2 ][ 0 ] = g_dynamicTexCoords_Layer2[ vertIndices[ 0 ] ];
            tr.layerTexCoord[ 2 ][ 1 ] = g_dynamicTexCoords_Layer2[ vertIndices[ 1 ] ];
            tr.layerTexCoord[ 2 ][ 2 ] = g_dynamicTexCoords_Layer2[ vertIndices[ 2 ] ];
        }
        if( ( inst.flags & GEOM_INST_FLAG_EXISTS_LAYER3 ) != 0 )
        {
            const uvec3 vertIndices =
                getVertIndicesDynamic( inst.firstVertex_Layer3, inst.baseIndexIndex, primitiveId );
            tr.layerTexCoord[ 3 ][ 0 ] = g_dynamicTexCoords_Layer3[ vertIndices[ 0 ] ];
            tr.layerTexCoord[ 3 ][ 1 ] = g_dynamicTexCoords_Layer3[ vertIndices[ 1 ] ];
            tr.layerTexCoord[ 3 ][ 2 ] = g_dynamicTexCoords_Layer3[ vertIndices[ 2 ] ];
        }

        // to world space
        tr.positions[0] = (inst.model * vec4(tr.positions[0], 1.0)).xyz;
        tr.positions[1] = (inst.model * vec4(tr.positions[1], 1.0)).xyz;
        tr.positions[2] = (inst.model * vec4(tr.positions[2], 1.0)).xyz;
        
        // dynamic     -- use prev model matrix and prev positions if exist
        const bool hasPrevInfo = inst.prevBaseVertexIndex != UINT32_MAX;

        if (hasPrevInfo)
        {
            const uvec3 prevVertIndices = getPrevVertIndicesDynamic(inst.prevBaseVertexIndex, inst.prevBaseIndexIndex, primitiveId);

            const vec4 prevLocalPos[] =
            {
                vec4(getPrevDynamicVerticesPositions(prevVertIndices[0]), 1.0),
                vec4(getPrevDynamicVerticesPositions(prevVertIndices[1]), 1.0),
                vec4(getPrevDynamicVerticesPositions(prevVertIndices[2]), 1.0)
            };

            tr.prevPositions[0] = (inst.prevModel * prevLocalPos[0]).xyz;
            tr.prevPositions[1] = (inst.prevModel * prevLocalPos[1]).xyz;
            tr.prevPositions[2] = (inst.prevModel * prevLocalPos[2]).xyz;
        }
        else
        {
            tr.prevPositions[0] = tr.positions[0];
            tr.prevPositions[1] = tr.positions[1];
            tr.prevPositions[2] = tr.positions[2];
        }
    }
    else
    {
        {
            const uvec3 vertIndices = getVertIndicesStatic(inst.baseVertexIndex, inst.baseIndexIndex, primitiveId);
        
            tr = makeTriangle(
                g_staticVertices[vertIndices[0]],
                g_staticVertices[vertIndices[1]],
                g_staticVertices[vertIndices[2]]);
        }

        if( ( inst.flags & GEOM_INST_FLAG_EXISTS_LAYER1 ) != 0 )
        {
            const uvec3 vertIndices =
                getVertIndicesStatic( inst.firstVertex_Layer1, inst.baseIndexIndex, primitiveId );
            tr.layerTexCoord[ 1 ][ 0 ] = g_staticTexCoords_Layer1[ vertIndices[ 0 ] ];
            tr.layerTexCoord[ 1 ][ 1 ] = g_staticTexCoords_Layer1[ vertIndices[ 1 ] ];
            tr.layerTexCoord[ 1 ][ 2 ] = g_staticTexCoords_Layer1[ vertIndices[ 2 ] ];
        }
        if( ( inst.flags & GEOM_INST_FLAG_EXISTS_LAYER2 ) != 0 )
        {
            const uvec3 vertIndices =
                getVertIndicesStatic( inst.firstVertex_Layer2, inst.baseIndexIndex, primitiveId );
            tr.layerTexCoord[ 2 ][ 0 ] = g_staticTexCoords_Layer2[ vertIndices[ 0 ] ];
            tr.layerTexCoord[ 2 ][ 1 ] = g_staticTexCoords_Layer2[ vertIndices[ 1 ] ];
            tr.layerTexCoord[ 2 ][ 2 ] = g_staticTexCoords_Layer2[ vertIndices[ 2 ] ];
        }
        if( ( inst.flags & GEOM_INST_FLAG_EXISTS_LAYER3 ) != 0 )
        {
            const uvec3 vertIndices =
                getVertIndicesStatic( inst.firstVertex_Layer3, inst.baseIndexIndex, primitiveId );
            tr.layerTexCoord[ 3 ][ 0 ] = g_staticTexCoords_Layer3[ vertIndices[ 0 ] ];
            tr.layerTexCoord[ 3 ][ 1 ] = g_staticTexCoords_Layer3[ vertIndices[ 1 ] ];
            tr.layerTexCoord[ 3 ][ 2 ] = g_staticTexCoords_Layer3[ vertIndices[ 2 ] ];
        }

        const vec4 prevLocalPos[] =
        {
            vec4(tr.positions[0], 1.0),
            vec4(tr.positions[1], 1.0),
            vec4(tr.positions[2], 1.0),
        };

        // to world space
        tr.positions[0] = (inst.model * prevLocalPos[0]).xyz;
        tr.positions[1] = (inst.model * prevLocalPos[1]).xyz;
        tr.positions[2] = (inst.model * prevLocalPos[2]).xyz;
        
        const bool isMovable = (inst.flags & GEOM_INST_FLAG_IS_MOVABLE) != 0;
        const bool hasPrevInfo = inst.prevBaseVertexIndex != UINT32_MAX;

        // movable     -- use prev model matrix if exist
        // non-movable -- use current model matrix
        if (isMovable && hasPrevInfo)
        {
            // static geoms' local positions are constant, 
            // only model matrices are changing
            tr.prevPositions[0] = (inst.prevModel * prevLocalPos[0]).xyz;
            tr.prevPositions[1] = (inst.prevModel * prevLocalPos[1]).xyz;
            tr.prevPositions[2] = (inst.prevModel * prevLocalPos[2]).xyz;
        }
        else
        {
            tr.prevPositions[0] = tr.positions[0];
            tr.prevPositions[1] = tr.positions[1];
            tr.prevPositions[2] = tr.positions[2];
        }
    }


    tr.layerColorTextures = uint[](
        inst.texture_base,
        inst.texture_layer1,
        inst.texture_layer2,
        inst.texture_layer3
    );

    tr.layerColors = uint[](
        inst.colorFactor_base,
        inst.colorFactor_layer1,
        inst.colorFactor_layer2,
        inst.colorFactor_layer3
    );

    tr.roughnessDefault = inst.roughnessDefault;
    tr.metallicDefault = inst.metallicDefault;
    tr.occlusionRougnessMetallicTexture = inst.texture_base_ORM;
    
    tr.normalTexture = inst.texture_base_N;

    tr.emissiveMult = inst.emissiveMult;
    tr.emissiveTexture = inst.texture_base_E;

    {
        const mat3 model3 = mat3(inst.model);

        // to world space
        tr.normals[0] = model3 * tr.normals[0];
        tr.normals[1] = model3 * tr.normals[1];
        tr.normals[2] = model3 * tr.normals[2];
        tr.tangent.xyz = model3 * tr.tangent.xyz;
    }

    tr.geometryInstanceFlags = inst.flags;
    tr.portalIndex = 0;

    return tr;
}

mat3 getOnlyCurPositions(int globalGeometryIndex, int instanceCustomIndex, int primitiveId)
{
    mat3 positions;

    const ShGeometryInstance inst = geometryInstances[globalGeometryIndex];

    const bool isDynamic = (instanceCustomIndex & INSTANCE_CUSTOM_INDEX_FLAG_DYNAMIC) == INSTANCE_CUSTOM_INDEX_FLAG_DYNAMIC;

    if (isDynamic)
    {
        const uvec3 vertIndices = getVertIndicesDynamic(inst.baseVertexIndex, inst.baseIndexIndex, primitiveId);

        // to world space
        positions[0] = (inst.model * vec4(getDynamicVerticesPositions(vertIndices[0]), 1.0)).xyz;
        positions[1] = (inst.model * vec4(getDynamicVerticesPositions(vertIndices[1]), 1.0)).xyz;
        positions[2] = (inst.model * vec4(getDynamicVerticesPositions(vertIndices[2]), 1.0)).xyz;
    }
    else
    {
        const uvec3 vertIndices = getVertIndicesStatic(inst.baseVertexIndex, inst.baseIndexIndex, primitiveId);

        // to world space
        positions[0] = (inst.model * vec4(getStaticVerticesPositions(vertIndices[0]), 1.0)).xyz;
        positions[1] = (inst.model * vec4(getStaticVerticesPositions(vertIndices[1]), 1.0)).xyz;
        positions[2] = (inst.model * vec4(getStaticVerticesPositions(vertIndices[2]), 1.0)).xyz;
    }
    
    return positions;
}

mat3 getOnlyPrevPositions(int globalGeometryIndex, int instanceCustomIndex, int primitiveId)
{
    mat3 prevPositions;

    const ShGeometryInstance inst = geometryInstances[globalGeometryIndex];

    const bool isDynamic = (instanceCustomIndex & INSTANCE_CUSTOM_INDEX_FLAG_DYNAMIC) == INSTANCE_CUSTOM_INDEX_FLAG_DYNAMIC;

    if (isDynamic)
    {
        // dynamic     -- use prev model matrix and prev positions if exist
        const bool hasPrevInfo = inst.prevBaseVertexIndex != UINT32_MAX;

        if (hasPrevInfo)
        {
            const uvec3 prevVertIndices = getPrevVertIndicesDynamic(inst.prevBaseVertexIndex, inst.prevBaseIndexIndex, primitiveId);

            const vec4 prevLocalPos[] =
            {
                vec4(getPrevDynamicVerticesPositions(prevVertIndices[0]), 1.0),
                vec4(getPrevDynamicVerticesPositions(prevVertIndices[1]), 1.0),
                vec4(getPrevDynamicVerticesPositions(prevVertIndices[2]), 1.0)
            };

            prevPositions[0] = (inst.prevModel * prevLocalPos[0]).xyz;
            prevPositions[1] = (inst.prevModel * prevLocalPos[1]).xyz;
            prevPositions[2] = (inst.prevModel * prevLocalPos[2]).xyz;
        }
        else
        {
            const uvec3 vertIndices = getVertIndicesDynamic(inst.baseVertexIndex, inst.baseIndexIndex, primitiveId);
            
            const vec4 localPos[] =
            {
                vec4(getDynamicVerticesPositions(vertIndices[0]), 1.0),
                vec4(getDynamicVerticesPositions(vertIndices[1]), 1.0),
                vec4(getDynamicVerticesPositions(vertIndices[2]), 1.0)
            };

            prevPositions[0] = (inst.model * localPos[0]).xyz;
            prevPositions[1] = (inst.model * localPos[1]).xyz;
            prevPositions[2] = (inst.model * localPos[2]).xyz;
        }
    }
    else
    {
        const uvec3 vertIndices = getVertIndicesStatic(inst.baseVertexIndex, inst.baseIndexIndex, primitiveId);
        
        const vec4 localPos[] =
        {
            vec4(getStaticVerticesPositions(vertIndices[0]), 1.0),
            vec4(getStaticVerticesPositions(vertIndices[1]), 1.0),
            vec4(getStaticVerticesPositions(vertIndices[2]), 1.0),
        };

        const bool isMovable = (inst.flags & GEOM_INST_FLAG_IS_MOVABLE) != 0;
        const bool hasPrevInfo = inst.prevBaseVertexIndex != UINT32_MAX;

        // movable     -- use prev model matrix if exist
        // non-movable -- use current model matrix
        if (isMovable && hasPrevInfo)
        {
            // static geoms' local positions are constant, 
            // only model matrices are changing
            prevPositions[0] = (inst.prevModel * localPos[0]).xyz;
            prevPositions[1] = (inst.prevModel * localPos[1]).xyz;
            prevPositions[2] = (inst.prevModel * localPos[2]).xyz;
        }
        else
        {
            prevPositions[0] = (inst.model * localPos[0]).xyz;
            prevPositions[1] = (inst.model * localPos[1]).xyz;
            prevPositions[2] = (inst.model * localPos[2]).xyz;
        }
    }

    return prevPositions;
}

vec4 packVisibilityBuffer(const ShPayload p)
{
    return vec4(uintBitsToFloat(p.instIdAndIndex), uintBitsToFloat(p.geomAndPrimIndex), p.baryCoords);
}

int unpackInstCustomIndexFromVisibilityBuffer(const vec4 v)
{
    int instanceID, instCustomIndex;
    unpackInstanceIdAndCustomIndex(floatBitsToUint(v[0]), instanceID, instCustomIndex);

    return instCustomIndex;
}

void unpackVisibilityBuffer(
    const vec4 v,
    out int instanceID, out int instCustomIndex,
    out int localGeomIndex, out int primIndex,
    out vec2 bary)
{
    unpackInstanceIdAndCustomIndex(floatBitsToUint(v[0]), instanceID, instCustomIndex);
    unpackGeometryAndPrimitiveIndex(floatBitsToUint(v[1]), localGeomIndex, primIndex);
    bary = vec2(v[2], v[3]);
}

// v must be fetched from framebufVisibilityBuffer_Prev_Sampler
bool unpackPrevVisibilityBuffer(const vec4 v, out vec3 prevPos)
{
    int prevInstanceID, instCustomIndex;
    int prevLocalGeomIndex, primIndex;

    unpackInstanceIdAndCustomIndex(floatBitsToUint(v[0]), prevInstanceID, instCustomIndex);
    unpackGeometryAndPrimitiveIndex(floatBitsToUint(v[1]), prevLocalGeomIndex, primIndex);

    int curFrameGlobalGeomIndex;
    const bool matched = getCurrentGeometryIndexByPrev(prevInstanceID, prevLocalGeomIndex, curFrameGlobalGeomIndex);

    if (!matched)
    {
        return false;
    }

    const mat3 prevVerts = getOnlyCurPositions(curFrameGlobalGeomIndex, instCustomIndex, primIndex);
    const vec3 baryCoords = vec3(1.0 - v[2] - v[3], v[2], v[3]);

    prevPos = prevVerts * baryCoords;

    return true;
}
#endif // DESC_SET_VERTEX_DATA
#endif // DESC_SET_GLOBAL_UNIFORM