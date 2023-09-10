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


#if defined(VERTEX_PREPROCESS_PARTIAL_DYNAMIC)
    #define FUNC_NAME convertForInstance_Dynamic
    #define GET_POSITIONS getDynamicVerticesPositions
    #define GET_NORMALS getDynamicVerticesNormals
    #define SET_NORMALS setDynamicVerticesNormals
    #define INDICES dynamicIndices
#elif defined(VERTEX_PREPROCESS_PARTIAL_STATIC)
    #define FUNC_NAME convertForInstance_Static
    #define GET_POSITIONS getStaticVerticesPositions
    #define GET_NORMALS getStaticVerticesNormals
    #define SET_NORMALS setStaticVerticesNormals
    #define INDICES staticIndices
#else
    #error
#endif



void FUNC_NAME (const ShGeometryInstance inst)
{
    const bool useIndices = inst.baseIndexIndex != UINT32_MAX;
    const bool genNormals = ( inst.flags & GEOM_INST_FLAG_GENERATE_NORMALS ) != 0 &&
                            ( inst.flags & GEOM_INST_FLAG_EXACT_NORMALS ) == 0;
    // -1 if normals should be inverted
    const float normalSign = float((inst.flags & GEOM_INST_FLAG_INVERTED_NORMALS) == 0) * 2.0 - 1.0;

    if (useIndices)
    {
        for (uint tri = 0; tri < inst.indexCount / 3; tri++)
        {
            const uint i = inst.baseIndexIndex + tri * 3;

            const uvec3 vertexIndices = uvec3(
                inst.baseVertexIndex + INDICES[i + 0],
                inst.baseVertexIndex + INDICES[i + 1],
                inst.baseVertexIndex + INDICES[i + 2]);

            const vec3 localPos[] = 
            {
                GET_POSITIONS(vertexIndices[0]),
                GET_POSITIONS(vertexIndices[1]),
                GET_POSITIONS(vertexIndices[2])
            };

            vec3 localNormal;
            
            if (genNormals)
            {
                localNormal = normalSign * normalize(cross(localPos[1] - localPos[0], localPos[2] - localPos[0]));
                
                SET_NORMALS(vertexIndices[0], localNormal);
                SET_NORMALS(vertexIndices[1], localNormal);
                SET_NORMALS(vertexIndices[2], localNormal);
            }
        }
    }
    else
    {
        for (uint tri = 0; tri < inst.vertexCount / 3; tri++)
        {
            const uint v = inst.baseVertexIndex + tri * 3;

            const uvec3 vertexIndices = uvec3(
                v + 0,
                v + 1,
                v + 2);

            const vec3 localPos[] = 
            {
                GET_POSITIONS(vertexIndices[0]),
                GET_POSITIONS(vertexIndices[1]),
                GET_POSITIONS(vertexIndices[2])
            };

            vec3 localNormal;

            if (genNormals)
            {
                localNormal = normalSign * normalize(cross(localPos[1] - localPos[0], localPos[2] - localPos[0]));

                SET_NORMALS(vertexIndices[0], localNormal);
                SET_NORMALS(vertexIndices[1], localNormal);
                SET_NORMALS(vertexIndices[2], localNormal);
            }
        } 
    }
}


#undef FUNC_NAME
#undef GET_POSITIONS
#undef GET_NORMALS
#undef SET_NORMALS
#undef INDICES

#undef VERTEX_PREPROCESS_PARTIAL_STATIC
#undef VERTEX_PREPROCESS_PARTIAL_DYNAMIC