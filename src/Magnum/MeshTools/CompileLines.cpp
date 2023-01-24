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
        "Trade::MeshTools::compileLines(): expected a line primitive, got" << lineMesh.primitive(), GL::Mesh{});

    CORRADE_INTERNAL_ASSERT(!flags); // TODO

    // TODO this will assert if the count in MeshData is wrong, check here?
    //  TODO make some isElementCountValid() utility?
    const UnsignedInt quadCount = primitiveCount(lineMesh.primitive(), lineMesh.isIndexed() ? lineMesh.indexCount() : lineMesh.vertexCount());

    Containers::Array<UnsignedInt> originalIndexData;
    Containers::StridedArrayView2D<const char> originalIndices;
    if(lineMesh.primitive() == MeshPrimitive::Lines) {
        if(lineMesh.isIndexed())
            originalIndices = lineMesh.indices();
    } else {
        if(lineMesh.primitive() == MeshPrimitive::LineStrip) {
            if(lineMesh.isIndexed())
                originalIndexData = generateLineStripIndices(lineMesh.indices());
            else
                originalIndexData = generateLineStripIndices(lineMesh.vertexCount());
        } else if(lineMesh.primitive() == MeshPrimitive::LineLoop) {
            if(lineMesh.isIndexed())
                originalIndexData = generateLineLoopIndices(lineMesh.indices());
            else
                originalIndexData = generateLineLoopIndices(lineMesh.vertexCount());
        } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

        originalIndices = Containers::arrayCast<2, const char>(stridedArrayView(originalIndexData));
    }

    /* Create a source index array for duplicate() by combining indices of a
       form 00112233 (i.e., duplicating every point twice) with the original
       mesh indices (if there are any). The same memory will be subsequently
       reused for the actual index buffer so it's not a wasted allocation. */
    Containers::Array<UnsignedInt> pointDuplicationIndices{NoInit, quadCount*4}; // TODO make larger for the actual index buffer below (*6 instead of *4)
    for(UnsignedInt i = 0; i != quadCount; ++i) {
        pointDuplicationIndices[i*4 + 0] =
            pointDuplicationIndices[i*4 + 1] = i*2 + 0;
        pointDuplicationIndices[i*4 + 2] =
            pointDuplicationIndices[i*4 + 3] = i*2 + 1;
    }

    // TODO document what is this actually
    if(originalIndices) // TODO what if the mesh is empty? does it treat this as non-indexed? test
        // TODO ffs this doesn't work if the original indices have different size :/
        duplicateInto(pointDuplicationIndices, originalIndices, Containers::arrayCast<2, char>(stridedArrayView(pointDuplicationIndices)));

    /* Position is required, everything else is optional */
    const Containers::Optional<UnsignedInt> positionAttributeId = lineMesh.findAttributeId(Trade::MeshAttribute::Position);
    CORRADE_ASSERT(positionAttributeId,
        "Trade::MeshTools::compileLines(): the mesh has no positions", GL::Mesh{});

    /* TODO update the doc to say what's this actually
     *
     * Allocate the output interleaved mesh including three additional
       attributes; the original position attribute should stay on the same
       index */
    // TODO test with a zero-attribute & zero-vertex mesh
    Trade::MeshData mesh = duplicate(Trade::MeshData{MeshPrimitive::Triangles, {}, pointDuplicationIndices, Trade::MeshIndexData{pointDuplicationIndices}, {}, lineMesh.vertexData(), Trade::meshAttributeDataNonOwningArray(lineMesh.attributeData()), lineMesh.vertexCount()}, {
        Trade::MeshAttributeData{MeshAttributePreviousPosition,
            lineMesh.attributeFormat(*positionAttributeId), nullptr},
        Trade::MeshAttributeData{MeshAttributeNextPosition,
            lineMesh.attributeFormat(*positionAttributeId), nullptr},
        Trade::MeshAttributeData{MeshAttributeAnnotation,
            VertexFormat::UnsignedInt, nullptr},
    });

    CORRADE_INTERNAL_ASSERT(mesh.attributeName(*positionAttributeId) == Trade::MeshAttribute::Position);

    /* Fill in previous/next positions -- given AABBCCDDEEFF, we want to copy
       Position from __BB__DD__FF to AA__CC__EE__'s NextPosition; and Position
       from AA__CC__EE__ to __BB__DD__FF's NextPosition. Form 3D arrays again,
       strip a prefix of either 0 or 2, pick every 2nd in the first dimension,
       and copy. */
    // TODO well, this comment is only for the first half
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

        /* Fill in previous/next neighbor positions if this is a line loop /
           line strip */
        if(lineMesh.primitive() == MeshPrimitive::LineStrip ||
           lineMesh.primitive() == MeshPrimitive::LineLoop) {
            Utility::copy(
                positions3.exceptSuffix(2).every(2),
                previousPositions3.exceptPrefix(2).every(2));
            Utility::copy(
                positions3.exceptPrefix(3).every(2),
                nextPositions3.exceptPrefix(1).exceptSuffix(2).every(2));
        }
        if(lineMesh.primitive() == MeshPrimitive::LineLoop) {
            Utility::copy(positions3[1], nextPositions3.back());
            Utility::copy(positions3[positions3.size()[0] - 2], previousPositions3.front());
        }
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

    /* A line strip has joins everywhere except the first and last two vertices,
       a line loop has them everywhere */
    // TODO have some unified handling, this is awful
    if(lineMesh.primitive() == MeshPrimitive::LineStrip) {
        // TODO
    } else if(lineMesh.primitive() == MeshPrimitive::LineLoop) {
        for(UnsignedInt i = 0; i != quadCount; ++i) {
            annotations[i*4 + 0] |= Shaders::LineVertexAnnotation::Join;
            annotations[i*4 + 1] |= Shaders::LineVertexAnnotation::Join;
            annotations[i*4 + 2] |= Shaders::LineVertexAnnotation::Join;
            annotations[i*4 + 3] |= Shaders::LineVertexAnnotation::Join;
        }
    }

    /* Create an index buffer */
    Containers::Array<UnsignedInt> indices;
    arrayReserve(indices, quadCount*6); // TODO reserve more if it's a single loop / strip; or maybe count the joins above?
    // TODO no, actually, use the pointDuplicationIndices array allocated above
    for(UnsignedInt i = 0; i != quadCount; ++i) {
        arrayAppend(indices, {
            // TODO document (and test!) that this is the order that's
            //  compatible with GL_LINES
            i*4 + 1,
            i*4 + 2,
            i*4 + 0,

            i*4 + 3,
            i*4 + 2,
            i*4 + 1
        });

        /* Add also indices for the bevel in both orientations (one will always
           degenerate) */
        if(annotations[i*4 + 3] & Shaders::LineVertexAnnotation::Join) {
            arrayAppend(indices, {
                i*4 + 2,
                i*4 + 3,
                (i*4 + 4) % (quadCount*4),
                (i*4 + 4) % (quadCount*4),
                i*4 + 3,
                (i*4 + 5) % (quadCount*4) // TODO workaround for loops, fix better
            });
        }
    }

    // TODO at this point it should spit out a MeshData for easier testing

    /* Upload the buffers, bind the line-specific attributes manually */
    GL::Buffer vertices{GL::Buffer::TargetHint::Array, mesh.vertexData()};
    GL::Mesh out = compile(mesh, GL::Buffer{NoCreate}, vertices);
    out.setPrimitive(MeshPrimitive::Triangles); // TODO do this elsewhere
    out.setIndexBuffer(GL::Buffer{GL::Buffer::TargetHint::ElementArray, indices}, 0, MeshIndexType::UnsignedInt); // TODO ugh!!!
    out.setCount(indices.size()); // TODO eeeughhhh
    out.addVertexBuffer(vertices,
        mesh.attributeOffset(MeshAttributePreviousPosition),
        mesh.attributeStride(MeshAttributePreviousPosition),
                        // TODO document that both are the same but we have to use 3D to make it possible to trim it down to 2 components for 2D (won't work the other way)
        GL::DynamicAttribute{Shaders::LineGL3D::PreviousPosition{}, mesh.attributeFormat(MeshAttributePreviousPosition)});
    out.addVertexBuffer(vertices,
        mesh.attributeOffset(MeshAttributeNextPosition),
        mesh.attributeStride(MeshAttributeNextPosition),
        GL::DynamicAttribute{Shaders::LineGL3D::NextPosition{}, mesh.attributeFormat(MeshAttributeNextPosition)});
    out.addVertexBuffer(std::move(vertices),
        mesh.attributeOffset(MeshAttributeAnnotation),
        mesh.attributeStride(MeshAttributeAnnotation),
        GL::DynamicAttribute{Shaders::LineGL3D::Annotation{}, mesh.attributeFormat(MeshAttributeAnnotation)});
    return out;
}

}}
