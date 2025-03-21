#ifndef Magnum_GL_Implementation_MeshState_h
#define Magnum_GL_Implementation_MeshState_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
              Vladimír Vondruš <mosra@centrum.cz>

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

#include "Magnum/GL/Mesh.h"

namespace Magnum { namespace GL { namespace Implementation {

struct ContextState;

struct MeshState {
    explicit MeshState(Context& context, ContextState& contextState, Containers::StaticArrayView<Implementation::ExtensionCount, const char*> extensions);
    #ifndef MAGNUM_TARGET_GLES
    ~MeshState();
    #endif

    void reset();

    void(*createImplementation)(Mesh&);
    void(*destroyImplementation)(Mesh&);
    void(*attributePointerImplementation)(Mesh&, Mesh::AttributeLayout&&);
    #if !defined(MAGNUM_TARGET_GLES) || defined(MAGNUM_TARGET_GLES2)
    void(*vertexAttribDivisorImplementation)(Mesh&, GLuint, GLuint);
    #endif
    void(*acquireVertexBufferImplementation)(Mesh&, Buffer&&);
    void(*bindIndexBufferImplementation)(Mesh&, Buffer&);
    void(*bindImplementation)(Mesh&);
    void(*unbindImplementation)(Mesh&);

    #ifdef MAGNUM_TARGET_GLES
    #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
    void(APIENTRY *drawElementsBaseVertexImplementation)(GLenum, GLsizei, GLenum, const void*, GLint);
    #endif
    #ifndef MAGNUM_TARGET_GLES2
    void(APIENTRY *drawRangeElementsBaseVertexImplementation)(GLenum, GLuint, GLuint, GLsizei, GLenum, const void*, GLint);
    #endif
    #endif

    #ifdef MAGNUM_TARGET_GLES2
    void(APIENTRY *drawArraysInstancedImplementation)(GLenum, GLint, GLsizei, GLsizei);
    #endif
    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_GLES2)
    void(APIENTRY *drawArraysInstancedBaseInstanceImplementation)(GLenum, GLint, GLsizei, GLsizei, GLuint);
    #endif
    #ifdef MAGNUM_TARGET_GLES2
    void(APIENTRY *drawElementsInstancedImplementation)(GLenum, GLsizei, GLenum, const void*, GLsizei);
    #endif
    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_GLES2)
    void(APIENTRY *drawElementsInstancedBaseVertexImplementation)(GLenum, GLsizei, GLenum, const void*, GLsizei, GLint);
    void(APIENTRY *drawElementsInstancedBaseInstanceImplementation)(GLenum, GLsizei, GLenum, const void*, GLsizei, GLuint);
    void(APIENTRY *drawElementsInstancedBaseVertexBaseInstanceImplementation)(GLenum, GLsizei, GLenum, const void*, GLsizei, GLint, GLuint);
    #endif

    #ifdef MAGNUM_TARGET_GLES
    void(*multiDrawViewImplementation)(const Containers::Iterable<MeshView>&);
    void(APIENTRY *multiDrawArraysImplementation)(GLenum, const GLint*, const GLsizei*, GLsizei);
    void(APIENTRY *multiDrawElementsImplementation)(GLenum, const GLsizei*, GLenum, const void* const*, GLsizei);
    #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
    void(APIENTRY *multiDrawElementsBaseVertexImplementation)(GLenum, const GLsizei*, GLenum, const void* const*, GLsizei, const GLint*);
    #endif
    #ifdef MAGNUM_TARGET_GLES
    void(APIENTRY *multiDrawArraysInstancedImplementation)(GLenum, const GLint*, const GLsizei*, const GLsizei*, GLsizei);
    void(APIENTRY *multiDrawElementsInstancedImplementation)(GLenum, const GLint*, GLenum, const void* const*, const GLsizei*, GLsizei);
    #ifndef MAGNUM_TARGET_GLES2
    void(APIENTRY *multiDrawArraysInstancedBaseInstanceImplementation)(GLenum, const GLint*, const GLsizei*, const GLsizei*, const GLuint*, GLsizei);
    void(APIENTRY *multiDrawElementsInstancedBaseVertexBaseInstanceImplementation)(GLenum, const GLint*, GLenum, const void* const*, const GLsizei*, const GLint*, const GLuint*, GLsizei);
    #endif
    #endif
    #endif

    void(*bindVAOImplementation)(GLuint);

    #ifndef MAGNUM_TARGET_GLES
    GLuint defaultVAO{}, /* Used on core profile in case ARB_VAO is disabled */
        /* Used for non-VAO-aware external GL code on core profile in case
           ARB_VAO is *not* disabled */
        scratchVAO{};
    #endif

    GLuint currentVAO;
    #if !defined(MAGNUM_TARGET_WEBGL) && !defined(MAGNUM_TARGET_GLES2)
    GLint maxVertexAttributeStride{};
    #endif
    #ifndef MAGNUM_TARGET_GLES2
    #ifndef MAGNUM_TARGET_WEBGL
    GLint64 maxElementIndex;
    #else
    GLint maxElementIndex;
    #endif
    GLint maxElementsIndices, maxElementsVertices;
    #endif
};

}}}

#endif
