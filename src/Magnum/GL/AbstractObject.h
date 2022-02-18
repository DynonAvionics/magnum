#ifndef Magnum_GL_AbstractObject_h
#define Magnum_GL_AbstractObject_h
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

/** @file
 * @brief Class @ref Magnum::GL::AbstractObject
 */

#include <string>
#include <Corrade/Containers/EnumSet.h>

#include "Magnum/Magnum.h"
#include "Magnum/GL/OpenGL.h"
#include "Magnum/GL/visibility.h"

namespace Magnum { namespace GL {

namespace Implementation { struct DebugState; }

/**
@brief Object wrapping flag

@see @ref ObjectFlags, @ref Buffer::wrap(), @ref BufferTexture::wrap(),
    @ref CubeMapTexture::wrap(), @ref CubeMapTextureArray::wrap(),
    @ref Framebuffer::wrap(), @ref Mesh::wrap(),
    @ref MultisampleTexture::wrap(), @ref PipelineStatisticsQuery::wrap(),
    @ref PrimitiveQuery::wrap(), @ref RectangleTexture::wrap(),
    @ref Renderbuffer::wrap(), @ref SampleQuery::wrap(), @ref Texture::wrap(),
    @ref TextureArray::wrap(), @ref TimeQuery::wrap(),
    @ref TransformFeedback::wrap()
*/
enum class ObjectFlag: UnsignedByte {
    /**
     * The object is known to be already created, either by using `glCreate*()`
     * function or by binding object previously generated by `glGen*()`
     * function. If you are not sure, don't specify this flag.
     */
    Created = 1 << 0,

    /** Delete the object on destruction. */
    DeleteOnDestruction = 1 << 1
};

/**
@brief Object wrapping flags

@see @ref Buffer::wrap(), @ref BufferTexture::wrap(),
    @ref CubeMapTexture::wrap(), @ref CubeMapTextureArray::wrap(),
    @ref Framebuffer::wrap(), @ref Mesh::wrap(),
    @ref MultisampleTexture::wrap(), @ref PipelineStatisticsQuery::wrap(),
    @ref PrimitiveQuery::wrap(), @ref RectangleTexture::wrap(),
    @ref Renderbuffer::wrap(), @ref SampleQuery::wrap(), @ref Texture::wrap(),
    @ref TextureArray::wrap(), @ref TimeQuery::wrap(),
    @ref TransformFeedback::wrap()
*/
typedef Containers::EnumSet<ObjectFlag> ObjectFlags;

/**
@brief Base for all OpenGL objects
*/
class MAGNUM_GL_EXPORT AbstractObject {
    friend Implementation::DebugState;

    public:
        #ifndef MAGNUM_TARGET_WEBGL
        /**
         * @brief Max object label length
         *
         * The result is cached, repeated queries don't result in repeated
         * OpenGL calls. If OpenGL 4.3 / OpenGL ES 3.2 is not supported and
         * @gl_extension{KHR,debug} desktop or ES extension (covered also by
         * @gl_extension{ANDROID,extension_pack_es31a}) is not available,
         * returns `0`. Note that @gl_extension{EXT,debug_label} has no such
         * limit.
         * @see @ref AbstractQuery::setLabel(), @ref AbstractShaderProgram::setLabel(),
         *      @ref AbstractTexture::setLabel(), @ref Buffer::setLabel(),
         *      @ref BufferTexture::setLabel(), @ref Framebuffer::setLabel(),
         *      @ref Mesh::setLabel(), @ref Renderbuffer::setLabel(),
         *      @ref Shader::setLabel(), @fn_gl{Get} with
         *      @def_gl_keyword{MAX_LABEL_LENGTH}
         * @requires_gles Debug output is not available in WebGL.
         */
        static Int maxLabelLength();
        #endif

    protected:
        MAGNUM_GL_LOCAL AbstractObject() = default;
        MAGNUM_GL_LOCAL ~AbstractObject() = default;

    private:
        #ifndef MAGNUM_TARGET_WEBGL
        static MAGNUM_GL_LOCAL void labelImplementationNoOp(GLenum, GLuint, Containers::ArrayView<const char> label);
        static MAGNUM_GL_LOCAL void labelImplementationExt(GLenum identifier, GLuint name, Containers::ArrayView<const char> label);
        #ifndef MAGNUM_TARGET_GLES2
        static MAGNUM_GL_LOCAL void labelImplementationKhrDesktopES32(GLenum identifier, GLuint name, Containers::ArrayView<const char> label);
        #endif
        #ifdef MAGNUM_TARGET_GLES
        static MAGNUM_GL_LOCAL void labelImplementationKhrES(GLenum identifier, GLuint name, Containers::ArrayView<const char> label);
        #endif
        static MAGNUM_GL_LOCAL std::string getLabelImplementationNoOp(GLenum, GLuint);
        static MAGNUM_GL_LOCAL std::string getLabelImplementationExt(GLenum identifier, GLuint name);
        #ifndef MAGNUM_TARGET_GLES2
        static MAGNUM_GL_LOCAL std::string getLabelImplementationKhrDesktopES32(GLenum identifier, GLuint name);
        #endif
        #ifdef MAGNUM_TARGET_GLES
        static MAGNUM_GL_LOCAL std::string getLabelImplementationKhrES(GLenum identifier, GLuint name);
        #endif
        #endif
};

CORRADE_ENUMSET_OPERATORS(ObjectFlags)

}}

#endif
