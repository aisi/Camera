//-----------------------------------------------------------------------------
// Copyright (c) 2007 dhpoware. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "normal_mapping_utils.h"

void CalcTangentVector(const D3DXVECTOR3 &pos1,
                       const D3DXVECTOR3 &pos2,
                       const D3DXVECTOR3 &pos3,
                       const D3DXVECTOR2 &texCoord1,
                       const D3DXVECTOR2 &texCoord2,
                       const D3DXVECTOR2 &texCoord3,
                       const D3DXVECTOR3 &normal,
                       D3DXVECTOR4 &tangent)
{
    // Given the 3 vertices (position and texture coordinates) of a triangle
    // calculate and return the triangle's tangent vector. The handedness of
    // the local coordinate system is stored in tangent.w. The bitangent is
    // then: float3 bitangent = cross(normal, tangent.xyz) * tangent.w.

    // Create 2 vectors in object space.
    //
    // edge1 is the vector from vertex positions pos1 to pos2.
    // edge2 is the vector from vertex positions pos1 to pos3.
    D3DXVECTOR3 edge1 = pos2 - pos1;
    D3DXVECTOR3 edge2 = pos3 - pos1;

    D3DXVec3Normalize(&edge1, &edge1);
    D3DXVec3Normalize(&edge2, &edge2);

    // Create 2 vectors in tangent (texture) space that point in the same
    // direction as edge1 and edge2 (in object space).
    //
    // texEdge1 is the vector from texture coordinates texCoord1 to texCoord2.
    // texEdge2 is the vector from texture coordinates texCoord1 to texCoord3.
    D3DXVECTOR2 texEdge1 = texCoord2 - texCoord1;
    D3DXVECTOR2 texEdge2 = texCoord3 - texCoord1;

    D3DXVec2Normalize(&texEdge1, &texEdge1);
    D3DXVec2Normalize(&texEdge2, &texEdge2);

    // These 2 sets of vectors form the following system of equations:
    //
    //  edge1 = (texEdge1.x * tangent) + (texEdge1.y * bitangent)
    //  edge2 = (texEdge2.x * tangent) + (texEdge2.y * bitangent)
    //
    // Using matrix notation this system looks like:
    //
    //  [ edge1 ]     [ texEdge1.x  texEdge1.y ]  [ tangent   ]
    //  [       ]  =  [                        ]  [           ]
    //  [ edge2 ]     [ texEdge2.x  texEdge2.y ]  [ bitangent ]
    //
    // The solution is:
    //
    //  [ tangent   ]        1     [ texEdge2.y  -texEdge1.y ]  [ edge1 ]
    //  [           ]  =  -------  [                         ]  [       ]
    //  [ bitangent ]      det A   [-texEdge2.x   texEdge1.x ]  [ edge2 ]
    //
    //  where:
    //        [ texEdge1.x  texEdge1.y ]
    //    A = [                        ]
    //        [ texEdge2.x  texEdge2.y ]
    //
    //    det A = (texEdge1.x * texEdge2.y) - (texEdge1.y * texEdge2.x)
    //
    // From this solution the tangent space basis vectors are:
    //
    //    tangent = (1 / det A) * ( texEdge2.y * edge1 - texEdge1.y * edge2)
    //  bitangent = (1 / det A) * (-texEdge2.x * edge1 + texEdge1.x * edge2)
    //     normal = cross(tangent, bitangent)

    D3DXVECTOR3 bitangent;
    float det = (texEdge1.x * texEdge2.y) - (texEdge1.y * texEdge2.x);

    if (fabsf(det) < 1e-6f)    // almost equal to zero
    {
        tangent.x = 1.0f;
        tangent.y = 0.0f;
        tangent.z = 0.0f;

        bitangent.x = 0.0f;
        bitangent.y = 1.0f;
        bitangent.z = 0.0f;
    }
    else
    {
        det = 1.0f / det;

        tangent.x = (texEdge2.y * edge1.x - texEdge1.y * edge2.x) * det;
        tangent.y = (texEdge2.y * edge1.y - texEdge1.y * edge2.y) * det;
        tangent.z = (texEdge2.y * edge1.z - texEdge1.y * edge2.z) * det;
        tangent.w = 0.0f;

        bitangent.x = (-texEdge2.x * edge1.x + texEdge1.x * edge2.x) * det;
        bitangent.y = (-texEdge2.x * edge1.y + texEdge1.x * edge2.y) * det;
        bitangent.z = (-texEdge2.x * edge1.z + texEdge1.x * edge2.z) * det;

        D3DXVec4Normalize(&tangent, &tangent);
        D3DXVec3Normalize(&bitangent, &bitangent);
    }

    // Calculate the handedness of the local tangent space.
    // The bitangent vector is the cross product between the triangle face
    // normal vector and the calculated tangent vector. The resulting bitangent
    // vector should be the same as the bitangent vector calculated from the
    // set of linear equations above. If they point in different directions
    // then we need to invert the cross product calculated bitangent vector. We
    // store this scalar multiplier in the tangent vector's 'w' component so
    // that the correct bitangent vector can be generated in the normal mapping
    // shader's vertex shader.

    D3DXVECTOR3 n(normal.x, normal.y, normal.z);
    D3DXVECTOR3 t(tangent.x, tangent.y, tangent.z);
    D3DXVECTOR3 b;

    D3DXVec3Cross(&b, &n, &t);
    tangent.w = (D3DXVec3Dot(&b, &bitangent) < 0.0f) ? -1.0f : 1.0f;
}

//-----------------------------------------------------------------------------
// NormalMappedQuad.
//-----------------------------------------------------------------------------

const D3DVERTEXELEMENT9 NormalMappedQuad::VERTEX_ELEMENTS[] =
{
    {0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    {0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
    {0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,  0},
    D3DDECL_END()
};

NormalMappedQuad::NormalMappedQuad()
{
    memset(m_vertices, 0, sizeof(m_vertices));
}

NormalMappedQuad::~NormalMappedQuad()
{
}

void NormalMappedQuad::generate(const D3DXVECTOR3 &origin,
                                const D3DXVECTOR3 &normal,
                                const D3DXVECTOR3 &up,
                                float width,
                                float height,
                                float uTile,
                                float vTile)
{
    D3DXVECTOR2 textureUpperLeft(0.0f, 0.0f);
    D3DXVECTOR2 textureUpperRight(1.0f * uTile, 0.0f);
    D3DXVECTOR2 textureLowerLeft(0.0f, 1.0f * vTile);
    D3DXVECTOR2 textureLowerRight(1.0f * uTile, 1.0f * vTile);

    D3DXVECTOR3 left;
    D3DXVec3Cross(&left, &up, &normal);

    D3DXVECTOR3 posUpperCenter = (up * height / 2.0f) + origin;
    D3DXVECTOR3 posUpperLeft = posUpperCenter + (left * width / 2.0f);
    D3DXVECTOR3 posUpperRight = posUpperCenter - (left * width / 2.0f);
    D3DXVECTOR3 posLowerLeft = posUpperLeft - (up * height);
    D3DXVECTOR3 posLowerRight = posUpperRight - (up * height);

    D3DXVECTOR4 tangent;

    CalcTangentVector(
        posUpperLeft, posUpperRight, posLowerLeft,
        textureUpperLeft, textureUpperRight, textureLowerLeft,
        normal, tangent);

    setVertex(0, posUpperLeft, textureUpperLeft, normal, tangent);
    setVertex(1, posUpperRight, textureUpperRight, normal, tangent);
    setVertex(2, posLowerLeft, textureLowerLeft, normal, tangent);

    CalcTangentVector(
        posLowerLeft, posUpperRight, posLowerRight,
        textureLowerLeft, textureUpperRight, textureLowerRight,
        normal, tangent);

    setVertex(3, posLowerLeft, textureLowerLeft, normal, tangent);
    setVertex(4, posUpperRight, textureUpperRight, normal, tangent);
    setVertex(5, posLowerRight, textureLowerRight, normal, tangent);
}

void NormalMappedQuad::setVertex(int i,
                                 const D3DXVECTOR3 &pos,
                                 const D3DXVECTOR2 &texCoord,
                                 const D3DXVECTOR3 &normal,
                                 const D3DXVECTOR4 &tangent)
{
    m_vertices[i].pos[0] = pos.x;
    m_vertices[i].pos[1] = pos.y;
    m_vertices[i].pos[2] = pos.z;

    m_vertices[i].texCoord[0] = texCoord.x;
    m_vertices[i].texCoord[1] = texCoord.y;

    m_vertices[i].normal[0] = normal.x;
    m_vertices[i].normal[1] = normal.y;
    m_vertices[i].normal[2] = normal.z;

    m_vertices[i].tangent[0] = tangent.x;
    m_vertices[i].tangent[1] = tangent.y;
    m_vertices[i].tangent[2] = tangent.z;
    m_vertices[i].tangent[3] = tangent.w;
}