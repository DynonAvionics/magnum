#ifndef Magnum_Shaders_LineGL_h
#define Magnum_Shaders_LineGL_h
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
 * @brief Class @ref Magnum::Shaders::LineGL, typedef @ref Magnum::Shaders::LineGL2D, @ref Magnum::Shaders::LineGL3D
 * @m_since_latest
 */

#include "Magnum/DimensionTraits.h"
#include "Magnum/GL/AbstractShaderProgram.h"
#include "Magnum/Shaders/GenericGL.h"
#include "Magnum/Shaders/glShaderWrapper.h"
#include "Magnum/Shaders/visibility.h"

namespace Magnum { namespace Shaders {

namespace Implementation {
    enum class LineGLFlag: UnsignedShort {
        VertexColor = 1 << 0,
        #ifndef MAGNUM_TARGET_GLES2
        ObjectId = 1 << 1,
        InstancedObjectId = (1 << 2)|ObjectId,
        #endif
        InstancedTransformation = 1 << 3,
        #ifndef MAGNUM_TARGET_GLES2
        UniformBuffers = 1 << 4,
        MultiDraw = UniformBuffers|(1 << 5)
        #endif
    };
    typedef Containers::EnumSet<LineGLFlag> LineGLFlags;
    CORRADE_ENUMSET_OPERATORS(LineGLFlags)

    // TODO this is GL-independent probably, what to do? put in Line.h?
    enum class LineGLCapStyle: UnsignedByte {
        Butt,
        Square,
        Round,
        Triangle
    };
    enum class LineGLJoinStyle: UnsignedByte {
        Miter,
        Bevel,
        Round
    };
}

// TODO drop all ES2-specific ifdefs, this needs ES3 due to gl_VertexID

/**
@brief Line GL shader
@m_since_latest

Compared to builtin GPU line rendering, the shader implements support for lines
of arbitrary width, antialiasing and custom cap styles.

@ref TODOTODO document attributes

TODO mention also how points can be drawn with this
*/
template<UnsignedInt dimensions> class MAGNUM_SHADERS_EXPORT LineGL: public GL::AbstractShaderProgram {
    public:
        class Configuration;
        class CompileState;

        // TODO document what's the last attribute for
        typedef typename GL::Attribute<0, Math::Vector<dimensions + 1, Float>> Position;

        // TODO move to Generic?
        typedef GL::Attribute<3, VectorTypeFor<dimensions, Float>> PreviousPosition;
        typedef GL::Attribute<5, VectorTypeFor<dimensions, Float>> NextPosition;

        /**
         * @brief Three-component vertex color
         *
         * @ref shaders-generic "Generic attribute", @ref Magnum::Color3. Use
         * either this or the @ref Color4 attribute.
         */
        typedef typename GenericGL<dimensions>::Color3 Color3;

        /**
         * @brief Four-component vertex color
         *
         * @ref shaders-generic "Generic attribute", @ref Magnum::Color4. Use
         * either this or the @ref Color3 attribute.
         */
        typedef typename GenericGL<dimensions>::Color4 Color4;

        #ifndef MAGNUM_TARGET_GLES2
        /**
         * @brief (Instanced) object ID
         *
         * @ref shaders-generic "Generic attribute",
         * @relativeref{Magnum,UnsignedInt}. Used only if
         * @ref Flag::InstancedObjectId is set.
         * @requires_gl30 Extension @gl_extension{EXT,gpu_shader4}
         * @requires_gles30 Object ID output requires integer support in
         *      shaders, which is not available in OpenGL ES 2.0.
         * @requires_webgl20 Object ID output requires integer support in
         *      shaders, which is not available in WebGL 1.0.
         */
        typedef typename GenericGL<dimensions>::ObjectId ObjectId;
        #endif

        /**
         * @brief (Instanced) transformation matrix
         *
         * @ref shaders-generic "Generic attribute",
         * @relativeref{Magnum,Matrix3} in 2D, @relativeref{Magnum,Matrix4} in
         * 3D. Used only if @ref Flag::InstancedTransformation is set.
         * @requires_gl33 Extension @gl_extension{ARB,instanced_arrays}
         * @requires_gles30 Extension @gl_extension{ANGLE,instanced_arrays},
         *      @gl_extension{EXT,instanced_arrays} or
         *      @gl_extension{NV,instanced_arrays} in OpenGL ES 2.0.
         * @requires_webgl20 Extension @webgl_extension{ANGLE,instanced_arrays}
         *      in WebGL 1.0.
         */
        typedef typename GenericGL<dimensions>::TransformationMatrix TransformationMatrix;

        enum: UnsignedInt {
            /**
             * Color shader output. Present always, expects three- or
             * four-component floating-point or normalized buffer attachment.
             */
            ColorOutput = GenericGL<dimensions>::ColorOutput,

            #ifndef MAGNUM_TARGET_GLES2
            /**
             * Object ID shader output. @ref shaders-generic "Generic output",
             * present only if @ref Flag::ObjectId is set. Expects a
             * single-component unsigned integral attachment. Writes the value
             * set in @ref setObjectId() and possibly also a per-vertex ID and
             * an ID fetched from a texture, see @ref Shaders-LineGL-object-id
             * for more information.
             * @requires_gl30 Extension @gl_extension{EXT,texture_integer}
             * @requires_gles30 Object ID output requires integer support in
             *      shaders, which is not available in OpenGL ES 2.0.
             * @requires_webgl20 Object ID output requires integer support in
             *      shaders, which is not available in WebGL 1.0.
             */
            ObjectIdOutput = GenericGL<dimensions>::ObjectIdOutput
            #endif
        };

        #ifdef DOXYGEN_GENERATING_OUTPUT
        /**
         * @brief Flag
         *
         * @see @ref Flags, @ref flags(), @ref Configuration::setFlags()
         */
        enum class Flag: UnsignedShort {
            /**
             * Multiply the color with a vertex color. Requires either the
             * @ref Color3 or @ref Color4 attribute to be present.
             */
            VertexColor = 1 << 0,

            #ifndef MAGNUM_TARGET_GLES2
            /**
             * Enable object ID output. See @ref Shaders-LineGL-object-id for
             * more information.
             * @requires_gl30 Extension @gl_extension{EXT,gpu_shader4}
             * @requires_gles30 Object ID output requires integer support in
             *      shaders, which is not available in OpenGL ES 2.0.
             * @requires_webgl20 Object ID output requires integer support in
             *      shaders, which is not available in WebGL 1.0.
             */
            ObjectId = 1 << 1,

            /**
             * Instanced object ID. Retrieves a per-instance / per-vertex
             * object ID from the @ref ObjectId attribute, outputting a sum of
             * the per-vertex ID and ID coming from @ref setObjectId() or
             * @ref LineDrawUniform::objectId. Implicitly enables
             * @ref Flag::ObjectId. See @ref Shaders-LineGL-object-id for more
             * information.
             * @requires_gl30 Extension @gl_extension{EXT,gpu_shader4}
             * @requires_gles30 Object ID output requires integer support in
             *      shaders, which is not available in OpenGL ES 2.0.
             * @requires_webgl20 Object ID output requires integer support in
             *      shaders, which is not available in WebGL 1.0.
             */
            InstancedObjectId = (1 << 2)|ObjectId,
            #endif

            /**
             * Instanced transformation. Retrieves a per-instance
             * transformation matrix from the @ref TransformationMatrix
             * attribute and uses it together with the matrix coming from
             * @ref setTransformationProjectionMatrix() or
             * @ref TransformationProjectionUniform2D::transformationProjectionMatrix
             * / @ref TransformationProjectionUniform3D::transformationProjectionMatrix
             * (first the per-instance, then the uniform matrix). See
             * @ref Shaders-LineGL-instancing for more information.
             * @ref TODOTODO or is it separate projection? update docs
             * @requires_gl33 Extension @gl_extension{ARB,instanced_arrays}
             * @requires_gles30 Extension @gl_extension{ANGLE,instanced_arrays},
             *      @gl_extension{EXT,instanced_arrays} or
             *      @gl_extension{NV,instanced_arrays} in OpenGL ES 2.0.
             * @requires_webgl20 Extension @webgl_extension{ANGLE,instanced_arrays}
             *      in WebGL 1.0.
             */
            InstancedTransformation = 1 << 3,

            #ifndef MAGNUM_TARGET_GLES2
            /**
             * Use uniform buffers. Expects that uniform data are supplied via
             * @ref bindTransformationProjectionBuffer(),
             * @ref bindDrawBuffer(), @ref bindTextureTransformationBuffer()
             * and @ref bindMaterialBuffer() instead of direct uniform setters.
             * @ref TODOTODO or is it separate projection? update docs
             * @requires_gl31 Extension @gl_extension{ARB,uniform_buffer_object}
             * @requires_gles30 Uniform buffers are not available in OpenGL ES
             *      2.0.
             * @requires_webgl20 Uniform buffers are not available in WebGL
             *      1.0.
             */
            UniformBuffers = 1 << 4,

            /**
             * Enable multidraw functionality. Implies @ref Flag::UniformBuffers
             * and adds the value from @ref setDrawOffset() with the
             * @glsl gl_DrawID @ce builtin, which makes draws submitted via
             * @ref GL::AbstractShaderProgram::draw(const Containers::Iterable<MeshView>&)
             * and related APIs pick up per-draw parameters directly, without
             * having to rebind the uniform buffers or specify
             * @ref setDrawOffset() before each draw. In a non-multidraw
             * scenario, @glsl gl_DrawID @ce is @cpp 0 @ce, which means a
             * shader with this flag enabled can be used for regular draws as
             * well.
             * @requires_gl46 Extension @gl_extension{ARB,uniform_buffer_object}
             *      and @gl_extension{ARB,shader_draw_parameters}
             * @requires_es_extension OpenGL ES 3.0 and extension
             *      @m_class{m-doc-external} [ANGLE_multi_draw](https://chromium.googlesource.com/angle/angle/+/master/extensions/ANGLE_multi_draw.txt)
             *      (unlisted). While the extension alone needs only OpenGL ES
             *      2.0, the shader implementation relies on uniform buffers,
             *      which require OpenGL ES 3.0.
             * @requires_webgl_extension WebGL 2.0 and extension
             *      @webgl_extension{ANGLE,multi_draw}. While the extension
             *      alone needs only WebGL 1.0, the shader implementation
             *      relies on uniform buffers, which require WebGL 2.0.
             * @m_since_latest
             */
            MultiDraw = UniformBuffers|(1 << 5)
            #endif
        };

        /**
         * @brief Flags
         *
         * @see @ref flags(), @ref Configuration::setFlags()
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Cap style
         *
         * @see @ref capStyle(), @ref Configuration::setCapStyle()
         */
        enum class CapStyle: UnsignedByte {
            // TODO document, say that Butt will make zero-sized lines (points)
            //  disappear
            Butt,
            Square,
            Round,
            Triangle
        };

        /**
         * @brief Join style
         *
         * @see @ref joinStyle(), @ref Configuration::setJoinStyle()
         */
        enum class JoinStyle: UnsignedByte {
            Miter,
            Bevel,
            Round
        };
        #else
        /* Done this way to be prepared for possible future diversion of 2D
           and 3D flags (e.g. introducing 3D-specific features) */
        typedef Implementation::LineGLFlag Flag;
        typedef Implementation::LineGLFlags Flags;

        // TODO have just a standalone LineCapStyle? in Line.h??
        typedef Implementation::LineGLCapStyle CapStyle;
        typedef Implementation::LineGLJoinStyle JoinStyle;
        #endif

        /**
         * @brief Compile asynchronously
         * @m_since_latest
         *
         * Compared to @ref LineGL(const Configuration&) can perform an
         * asynchronous compilation and linking. See @ref shaders-async for
         * more information.
         * @see @ref LineGL(CompileState&&)
         */
        /* Compared to the non-templated shaders like PhongGL or
           MeshVisualizerGL2D using a forward declaration is fine here. Huh. */
        static CompileState compile(const Configuration& configuration = Configuration{});

        /* Compared to the non-templated shaders like PhongGL or
           MeshVisualizerGL2D using a forward declaration is fine here. Huh. */
        explicit LineGL(const Configuration& configuration = Configuration{});

        /**
         * @brief Finalize an asynchronous compilation
         * @m_since_latest
         *
         * Takes an asynchronous compilation state returned by @ref compile()
         * and forms a ready-to-use shader object. See @ref shaders-async for
         * more information.
         */
        explicit LineGL(CompileState&& state);

        /**
         * @brief Construct without creating the underlying OpenGL object
         *
         * The constructed instance is equivalent to a moved-from state. Useful
         * in cases where you will overwrite the instance later anyway. Move
         * another object over it to make it useful.
         *
         * This function can be safely used for constructing (and later
         * destructing) objects even without any OpenGL context being active.
         * However note that this is a low-level and a potentially dangerous
         * API, see the documentation of @ref NoCreate for alternatives.
         */
        explicit LineGL(NoCreateT) noexcept: GL::AbstractShaderProgram{NoCreate} {}

        /** @brief Copying is not allowed */
        LineGL(const LineGL<dimensions>&) = delete;

        /** @brief Move constructor */
        LineGL(LineGL<dimensions>&&) noexcept = default;

        /** @brief Copying is not allowed */
        LineGL<dimensions>& operator=(const LineGL<dimensions>&) = delete;

        /** @brief Move assignment */
        LineGL<dimensions>& operator=(LineGL<dimensions>&&) noexcept = default;

        /**
         * @brief Flags
         *
         * @see @ref Configuration::setFlags()
         */
        Flags flags() const { return _flags; }

        /**
         * @brief Cap style
         *
         * @see @ref Configuration::setCapStyle()
         */
        CapStyle capStyle() const { return _capStyle; }

        /**
         * @brief Join style
         *
         * @see @ref Configuration::setJoinStyle()
         */
        JoinStyle joinStyle() const { return _joinStyle; }

        #ifndef MAGNUM_TARGET_GLES2
        UnsignedInt materialCount() const { return _materialCount; }

        UnsignedInt drawCount() const { return _drawCount; }
        #endif

        // TODO split projection for 3D? would that mean a dedicated 3D class? uhhhhhhhhh
        LineGL<dimensions>& setTransformationProjectionMatrix(const MatrixTypeFor<dimensions, Float>& matrix);

        LineGL<dimensions>& setViewportSize(const Vector2& size);

        LineGL<dimensions>& setWidth(Float width);

        LineGL<dimensions>& setSmoothness(Float smoothness);

        LineGL<dimensions>& setMiterLimit(Float smoothness);

        LineGL<dimensions>& setBackgroundColor(const Magnum::Color4& color);

        LineGL<dimensions>& setColor(const Magnum::Color4& color);

        #ifndef MAGNUM_TARGET_GLES2
        LineGL<dimensions>& setObjectId(UnsignedInt id);
        #endif

        #ifndef MAGNUM_TARGET_GLES2
        LineGL<dimensions>& setDrawOffset(UnsignedInt offset);

        LineGL<dimensions>& bindTransformationProjectionBuffer(GL::Buffer& buffer);
        LineGL<dimensions>& bindTransformationProjectionBuffer(GL::Buffer& buffer, GLintptr offset, GLsizeiptr size);

        LineGL<dimensions>& bindDrawBuffer(GL::Buffer& buffer);
        LineGL<dimensions>& bindDrawBuffer(GL::Buffer& buffer, GLintptr offset, GLsizeiptr size);

        LineGL<dimensions>& bindMaterialBuffer(GL::Buffer& buffer);
        LineGL<dimensions>& bindMaterialBuffer(GL::Buffer& buffer, GLintptr offset, GLsizeiptr size);
        #endif

        MAGNUM_GL_ABSTRACTSHADERPROGRAM_SUBCLASS_DRAW_IMPLEMENTATION(LineGL<dimensions>)

    private:
        /* Creates the GL shader program object but does nothing else.
           Internal, used by compile(). */
        explicit LineGL(NoInitT);

        Flags _flags;
        CapStyle _capStyle;
        JoinStyle _joinStyle;
        #ifndef MAGNUM_TARGET_GLES2
        UnsignedInt _materialCount{}, _drawCount{};
        #endif
        Int _viewportSizeUniform{0},
            _transformationProjectionMatrixUniform{1},
            _widthUniform{2},
            _smoothnessUniform{3},
            _miterLimitUniform{4},
            _backgroundColorUniform{5},
            _colorUniform{6};
        #ifndef MAGNUM_TARGET_GLES2
        Int _objectIdUniform{7};
        /* Used instead of all other uniforms except viewportSize when
           Flag::UniformBuffers is set, so it can alias them */
        Int _drawOffsetUniform{1};
        #endif
};

/**
@brief Configuration

*/
template<UnsignedInt dimensions> class LineGL<dimensions>::Configuration {
    public:
        explicit Configuration() = default;

        Configuration& setFlags(Flags flags) {
            _flags = flags;
            return *this;
        }

        Flags flags() const { return _flags; }

        // TODO document the default and say that it's different from SVG
        //  because it'd make points disappear
        Configuration& setCapStyle(CapStyle style) {
            _capStyle = style;
            return *this;
        }

        CapStyle capStyle() const { return _capStyle; }

        Configuration& setJoinStyle(JoinStyle style) {
            _joinStyle = style;
            return *this;
        }

        JoinStyle joinStyle() const { return _joinStyle; }

        #ifndef MAGNUM_TARGET_GLES2
        Configuration& setMaterialCount(UnsignedInt count) {
            _materialCount = count;
            return *this;
        }

        UnsignedInt materialCount() const { return _materialCount; }

        Configuration& setDrawCount(UnsignedInt count) {
            _drawCount = count;
            return *this;
        }

        UnsignedInt drawCount() const { return _drawCount; }
        #endif

    private:
        Flags _flags;
        CapStyle _capStyle = CapStyle::Square;
        JoinStyle _joinStyle = JoinStyle::Miter;
        #ifndef MAGNUM_TARGET_GLES2
        UnsignedInt _materialCount{1};
        UnsignedInt _drawCount{1};
        #endif
};

/**
@brief Asynchronous compilation state

Returned by @ref compile(). See @ref shaders-async for more information.
*/
template<UnsignedInt dimensions> class LineGL<dimensions>::CompileState: public LineGL<dimensions> {
    /* Everything deliberately private except for the inheritance */
    friend class LineGL;

    explicit CompileState(NoCreateT): LineGL{NoCreate}, _vert{NoCreate}, _frag{NoCreate} {}

    explicit CompileState(LineGL<dimensions>&& shader, GL::Shader&& vert, GL::Shader&& frag, GL::Version version): LineGL<dimensions>{std::move(shader)}, _vert{std::move(vert)}, _frag{std::move(frag)}, _version{version} {}

    Implementation::GLShaderWrapper _vert, _frag;
    GL::Version _version;
};

/**
@brief 2D line OpenGL shader
@m_since_latest
*/
typedef LineGL<2> LineGL2D;

/**
@brief 3D LineGL OpenGL shader
@m_since_latest
*/
typedef LineGL<3> LineGL3D;

#ifdef DOXYGEN_GENERATING_OUTPUT
/** @debugoperatorclassenum{LineGL,LineGL::Flag} */
template<UnsignedInt dimensions> Debug& operator<<(Debug& debug, LineGL<dimensions>::Flag value);

/** @debugoperatorclassenum{LineGL,LineGL::Flags} */
template<UnsignedInt dimensions> Debug& operator<<(Debug& debug, LineGL<dimensions>::Flags value);

/** @debugoperatorclassenum{LineGL,LineGL::CapStyle} */
template<UnsignedInt dimensions> Debug& operator<<(Debug& debug, LineGL<dimensions>::CapStyle value);

/** @debugoperatorclassenum{LineGL,LineGL::JoinStyle} */
template<UnsignedInt dimensions> Debug& operator<<(Debug& debug, LineGL<dimensions>::JoinStyle value);
#else
namespace Implementation {
    MAGNUM_SHADERS_EXPORT Debug& operator<<(Debug& debug, LineGLFlag value);
    MAGNUM_SHADERS_EXPORT Debug& operator<<(Debug& debug, LineGLFlags value);
    MAGNUM_SHADERS_EXPORT Debug& operator<<(Debug& debug, LineGLCapStyle value);
    MAGNUM_SHADERS_EXPORT Debug& operator<<(Debug& debug, LineGLJoinStyle value);
}
#endif

}}

#endif
