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

#if !defined(NORMAL_MAPPING_UTILS_H)
#define NORMAL_MAPPING_UTILS_H

#include <d3dx9math.h>
#include <d3d9types.h>

//-----------------------------------------------------------------------------
// Given the 3 vertices (position and texture coordinates) of a triangle
// calculate and return the triangle's tangent vector. The handedness of
// the local coordinate system is stored in tangent.w. The bitangent is
// then: float3 bitangent = cross(normal, tangent.xyz) * tangent.w.
//-----------------------------------------------------------------------------

extern void CalcTangentVector(const D3DXVECTOR3 &pos1,
                              const D3DXVECTOR3 &pos2,
                              const D3DXVECTOR3 &pos3,
                              const D3DXVECTOR2 &texCoord1,
                              const D3DXVECTOR2 &texCoord2,
                              const D3DXVECTOR2 &texCoord3,
                              const D3DXVECTOR3 &normal,
                              D3DXVECTOR4 &tangent);

//-----------------------------------------------------------------------------
// The NormalMappedQuad class is used to procedurally generate a quad. The
// generated quad contains tangent and bitangent vectors for use with normal
// mapping.
//-----------------------------------------------------------------------------

class NormalMappedQuad
{
public:
    struct Vertex
    {
        float pos[3];
        float texCoord[2];
        float normal[3];
        float tangent[4];
    };

    NormalMappedQuad();
    ~NormalMappedQuad();

    void generate(const D3DXVECTOR3 &origin, const D3DXVECTOR3 &normal,
                  const D3DXVECTOR3 &up, float width, float height,
                  float uTile, float vTile);

    int getPrimitiveCount() const
    { return 2; }

    int getVertexCount() const
    { return static_cast<int>(sizeof(m_vertices) / sizeof(m_vertices[0])); }

    const D3DVERTEXELEMENT9 *getVertexElements() const
    { return VERTEX_ELEMENTS; }

    int getVertexSize() const
    { return static_cast<int>(sizeof(Vertex)); }

    const Vertex *getVertices() const
    { return m_vertices; }

private:
    static const D3DVERTEXELEMENT9 VERTEX_ELEMENTS[];

    void setVertex(int i, const D3DXVECTOR3 &pos, const D3DXVECTOR2 &texCoord,
                   const D3DXVECTOR3 &normal, const D3DXVECTOR4 &tangent);

    Vertex m_vertices[6];
};

#endif