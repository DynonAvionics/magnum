/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "CompileLines.h"

#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>

// #include "Magnum/DimensionTraits.h"
#include "Magnum/GL/Buffer.h"
#include "Magnum/GL/Mesh.h"
#include "Magnum/Math/Vector3.h"
#include "Magnum/MeshTools/Compile.h"
#include "Magnum/MeshTools/Duplicate.h"
#include "Magnum/MeshTools/GenerateIndices.h"
#include "Magnum/MeshTools/Interleave.h"
#include "Magnum/MeshTools/Reference.h"
#include "Magnum/Trade/MeshData.h"

/* These headers are included only privately and don't introduce any linker
   dependency (taking just the LineVertexAnnotations enum and the
   PreviousPosition, NextPosition and Annotation attribute typedefs), thus it's
   completely safe to not link to the Shaders library */
#include "Magnum/Shaders/Line.h"
#include "Magnum/Shaders/LineGL.h"

namespace Magnum { namespace MeshTools {

namespace {

constexpr Trade::MeshAttribute MeshAttributePreviousPosition = Trade::meshAttributeCustom(32765);
constexpr Trade::MeshAttribute MeshAttributeNextPosition = Trade::meshAttributeCustom(32766);
constexpr Trade::MeshAttribute MeshAttributeAnnotation = Trade::meshAttributeCustom(32767);

}

GL::Mesh compileLines(const Trade::MeshData& lineMesh, const CompileLinesFlags flags) {
    CORRADE_ASSERT(lineMesh.primitive() == MeshPrimitive::Lines ||
                   lineMesh.primitive() == MeshPrimitive::LineStrip ||
                   lineMesh.primitive() == MeshPrimitive::LineLoop,
        "Trade::MeshTools::generateLines(): expected a line primitive, got" << lineMesh.primitive(), GL::Mesh{});

    CORRADE_INTERNAL_ASSERT(!flags); // TODO

    /* Convert to a non-indexed mesh with Lines as a primitive */
    Trade::MeshData lineMeshNonIndexed = MeshTools::reference(lineMesh);

    if(lineMeshNonIndexed.primitive() != MeshPrimitive::Lines) {
        if(lineMeshNonIndexed.isIndexed()) // TODO drop once generateIndices is fixed
            lineMeshNonIndexed = MeshTools::duplicate(lineMeshNonIndexed);
        lineMeshNonIndexed = MeshTools::generateIndices(lineMeshNonIndexed);
    };
    if(lineMeshNonIndexed.isIndexed())
        lineMeshNonIndexed = MeshTools::duplicate(lineMeshNonIndexed);

    // TODO the above APIs should check this, but this can fire if the input is
    //  already a non-indexed Lines mesh
    CORRADE_INTERNAL_ASSERT(lineMeshNonIndexed.vertexCount() % 2 == 0);
    const UnsignedInt quadCount = lineMeshNonIndexed.vertexCount()/2;

    /* Position is required, everything else is optional */
    const Containers::Optional<UnsignedInt> positionAttributeId = lineMesh.findAttributeId(Trade::MeshAttribute::Position);
    CORRADE_ASSERT(positionAttributeId,
        "Trade::MeshTools::generateLines(): the mesh has no positions", GL::Mesh{});

    /* Allocate the output interleaved mesh including three additional
       attributes; the original position attribute should stay on the same
       index */
    // TODO need a way to change the primitive to Triangles, otherwise it'll
    //  stay Lines
    Trade::MeshData mesh = interleavedLayout(lineMeshNonIndexed, lineMeshNonIndexed.vertexCount()*2, {
        Trade::MeshAttributeData{MeshAttributePreviousPosition,
            lineMeshNonIndexed.attributeFormat(*positionAttributeId), nullptr},
        Trade::MeshAttributeData{MeshAttributeNextPosition,
            lineMeshNonIndexed.attributeFormat(*positionAttributeId), nullptr},
        Trade::MeshAttributeData{MeshAttributeAnnotation,
            VertexFormat::UnsignedInt, nullptr},
    });
    CORRADE_INTERNAL_ASSERT(mesh.attributeName(*positionAttributeId) == Trade::MeshAttribute::Position);

    /* Copy the original data over. Given input arrays ABCDEF, we want
       AABBCCDDEEFF, i.e. every point duplicated. That can be achieved by
       "reshaping" every array as 3D with dimensions (size, 1, elementSize),
       broadcasting the second dimensions to 2 items and copying to the output
       of size (size/2, 2, elementSize).

         ABCDEF -> ABCDEF
                \> ABCDEF */
    for(UnsignedInt i = 0; i != lineMeshNonIndexed.attributeCount(); ++i) {
        // TODO we could also just have an index buffer that's the original
        //  duplicated, and then call duplicate() just once .. urg but it
        //  wouldn't include the extra attribs
        const Containers::StridedArrayView2D<const char> in = lineMeshNonIndexed.attribute(i);
        const Containers::StridedArrayView3D<const char> in3{lineMeshNonIndexed.vertexData(), static_cast<const char*>(in.data()),
            {in.size()[0], 1, in.size()[1]},
            {in.stride()[0], 0, in.stride()[1]}};

        const Containers::StridedArrayView2D<char> out = mesh.mutableAttribute(i);
        const Containers::StridedArrayView3D<char> out3{mesh.mutableVertexData(), static_cast<char*>(out.data()),
            {out.size()[0]/2, 2, out.size()[1]},
            {out.stride()[0]*2, out.stride()[0], out.stride()[1]}};

        Utility::copy(in3.broadcasted<1>(2), out3);
    }

    /* Fill in previous/next positions -- given AABBCCDDEEFF, we want to copy
       Position from __BB__DD__FF to AA__CC__EE__'s NextPosition; and Position
       from AA__CC__EE__ to __BB__DD__FF's NextPosition. Form 3D arrays again,
       strip a prefix of either 0 or 2, pick every 2nd in the first dimension,
       and copy. */
    const Containers::StridedArrayView2D<const char> positions = mesh.attribute(Trade::MeshAttribute::Position);
    {
        const Containers::StridedArrayView3D<const char> positions3{mesh.vertexData(), static_cast<const char*>(positions.data()),
            {positions.size()[0]/2, 2, positions.size()[1]},
            {positions.stride()[0]*2, positions.stride()[0], positions.stride()[1]}};

        const Containers::StridedArrayView2D<char> previousPositions = mesh.mutableAttribute(MeshAttributePreviousPosition);
        const Containers::StridedArrayView3D<char> previousPositions3{mesh.mutableVertexData(), static_cast<char*>(previousPositions.data()),
            {previousPositions.size()[0]/2, 2, previousPositions.size()[1]},
            {previousPositions.stride()[0]*2, previousPositions.stride()[0], previousPositions.stride()[1]}};

        const Containers::StridedArrayView2D<char> nextPositions = mesh.mutableAttribute(MeshAttributeNextPosition);
        const Containers::StridedArrayView3D<char> nextPositions3{mesh.mutableVertexData(), static_cast<char*>(nextPositions.data()),
            {nextPositions.size()[0]/2, 2, nextPositions.size()[1]},
            {nextPositions.stride()[0]*2, nextPositions.stride()[0], nextPositions.stride()[1]}};

        Utility::copy(
            positions3.exceptSuffix(1).every(2),
            previousPositions3.exceptPrefix(1).every(2));
        Utility::copy(
            positions3.exceptPrefix(1).every(2),
            nextPositions3.exceptSuffix(1).every(2));
    }

    /* Fill in point annotation */
    const Containers::StridedArrayView1D<Shaders::LineVertexAnnotations> annotations = Containers::arrayCast<Shaders::LineVertexAnnotations>(mesh.mutableAttribute<UnsignedInt>(MeshAttributeAnnotation));
    for(UnsignedInt i = 0; i != quadCount; ++i) {
        // TODO errr use the original index buffer somehow to figure out
        //  joins?

        annotations[i*4 + 0] = Shaders::LineVertexAnnotation::Up|Shaders::LineVertexAnnotation::Begin;
        annotations[i*4 + 1] = Shaders::LineVertexAnnotation::Begin;
        annotations[i*4 + 2] = Shaders::LineVertexAnnotation::Up;
        annotations[i*4 + 3] = {};
    }

    /* Create an index buffer */
    Containers::Array<UnsignedInt> indices;
    arrayReserve(indices, quadCount*6); // TODO reserve more if it's a single loop / strip; or maybe count the joins above?
    for(UnsignedInt i = 0; i != quadCount; ++i) {
        arrayAppend(indices, {
            i*4 + 0,
            i*4 + 1,
            i*4 + 2,
            i*4 + 2,
            i*4 + 1,
            i*4 + 3
        });

        /* Add also indices for the bevel in both orientations (one will always
           degenerate) */
        if(annotations[i*4 + 3] & Shaders::LineVertexAnnotation::Join) {
            arrayAppend(indices, {
                i*4 + 2,
                i*4 + 3,
                i*4 + 4,
                i*4 + 4,
                i*4 + 3,
                i*4 + 5
            });
        }
    }

    /* Upload the buffers, bind the line-specific attributes manually */
    GL::Buffer vertices{GL::Buffer::TargetHint::Array, mesh.vertexData()};
    GL::Mesh out = compile(mesh, GL::Buffer{NoCreate}, vertices);
    out.setPrimitive(MeshPrimitive::Triangles); // TODO do this elsewhere
    out.setIndexBuffer(GL::Buffer{GL::Buffer::TargetHint::ElementArray, indices}, 0, MeshIndexType::UnsignedInt); // TODO ugh!!!
    out.setCount(indices.size()); // TODO eeeughhhh
    out.addVertexBuffer(vertices,
        mesh.attributeOffset(MeshAttributePreviousPosition),
        mesh.attributeStride(MeshAttributePreviousPosition),
        GL::DynamicAttribute{Shaders::LineGL2D::PreviousPosition{}, mesh.attributeFormat(MeshAttributePreviousPosition)});
    out.addVertexBuffer(vertices,
        mesh.attributeOffset(MeshAttributeNextPosition),
        mesh.attributeStride(MeshAttributeNextPosition),
        GL::DynamicAttribute{Shaders::LineGL2D::NextPosition{}, mesh.attributeFormat(MeshAttributeNextPosition)});
    out.addVertexBuffer(std::move(vertices),
        mesh.attributeOffset(MeshAttributeAnnotation),
        mesh.attributeStride(MeshAttributeAnnotation),
        GL::DynamicAttribute{Shaders::LineGL2D::Annotation{}, mesh.attributeFormat(MeshAttributeAnnotation)});
    return out;
}

}}
