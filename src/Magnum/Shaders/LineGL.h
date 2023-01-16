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

#ifndef MAGNUM_TARGET_GLES2
/** @file
 * @brief Class @ref Magnum::Shaders::LineGL, typedef @ref Magnum::Shaders::LineGL2D, @ref Magnum::Shaders::LineGL3D
 * @m_since_latest
 */
#endif

#include "Magnum/DimensionTraits.h"
#include "Magnum/GL/AbstractShaderProgram.h"
#include "Magnum/Shaders/GenericGL.h"
#include "Magnum/Shaders/glShaderWrapper.h"
#include "Magnum/Shaders/visibility.h"

#ifndef MAGNUM_TARGET_GLES2
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
}

/**
@brief Line GL shader
@m_since_latest

Compared to builtin GPU line rendering, the shader implements support for lines
of arbitrary width, antialiasing and custom cap styles.

@requires_gl30 Extension @gl_extension{EXT,gpu_shader4}
@requires_gles30 Requires integer support in shaders which is not available in
    OpenGL ES 2.0.
@requires_webgl20 Requires integer support in shaders which is not available in
    WebGL 1.0.

TODO document UBOs below the above requirements so it's clear that the exts are
    globally required

@ref TODOTODO document attributes

TODO mention also how points can be drawn with this
*/
template<UnsignedInt dimensions> class MAGNUM_SHADERS_EXPORT LineGL: public GL::AbstractShaderProgram {
    public:
        class Configuration;
        class CompileState;

        // TODO actually move the last attribute elsewhere
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

        /**
         * @brief (Instanced) object ID
         *
         * @ref shaders-generic "Generic attribute",
         * @relativeref{Magnum,UnsignedInt}. Used only if
         * @ref Flag::InstancedObjectId is set.
         */
        typedef typename GenericGL<dimensions>::ObjectId ObjectId;

        /**
         * @brief (Instanced) transformation matrix
         *
         * @ref shaders-generic "Generic attribute",
         * @relativeref{Magnum,Matrix3} in 2D, @relativeref{Magnum,Matrix4} in
         * 3D. Used only if @ref Flag::InstancedTransformation is set.
         * @requires_gl33 Extension @gl_extension{ARB,instanced_arrays}
         */
        typedef typename GenericGL<dimensions>::TransformationMatrix TransformationMatrix;

        enum: UnsignedInt {
            /**
             * Color shader output. Present always, expects three- or
             * four-component floating-point or normalized buffer attachment.
             */
            ColorOutput = GenericGL<dimensions>::ColorOutput,

            /**
             * Object ID shader output. @ref shaders-generic "Generic output",
             * present only if @ref Flag::ObjectId is set. Expects a
             * single-component unsigned integral attachment. Writes the value
             * set in @ref setObjectId() and possibly also a per-vertex ID and
             * an ID fetched from a texture, see @ref Shaders-LineGL-object-id
             * for more information.
             * @requires_gl30 Extension @gl_extension{EXT,texture_integer}
             */
            ObjectIdOutput = GenericGL<dimensions>::ObjectIdOutput
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

            /**
             * Enable object ID output. See @ref Shaders-LineGL-object-id for
             * more information.
             */
            ObjectId = 1 << 1,

            /**
             * Instanced object ID. Retrieves a per-instance / per-vertex
             * object ID from the @ref ObjectId attribute, outputting a sum of
             * the per-vertex ID and ID coming from @ref setObjectId() or
             * @ref LineDrawUniform::objectId. Implicitly enables
             * @ref Flag::ObjectId. See @ref Shaders-LineGL-object-id for more
             * information.
             */
            InstancedObjectId = (1 << 2)|ObjectId,

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
             */
            InstancedTransformation = 1 << 3,

            /**
             * Use uniform buffers. Expects that uniform data are supplied via
             * @ref bindTransformationProjectionBuffer(),
             * @ref bindDrawBuffer(), @ref bindTextureTransformationBuffer()
             * and @ref bindMaterialBuffer() instead of direct uniform setters.
             * @ref TODOTODO or is it separate projection? update docs
             * @requires_gl31 Extension @gl_extension{ARB,uniform_buffer_object}
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
             */
            MultiDraw = UniformBuffers|(1 << 5)
        };

        /**
         * @brief Flags
         *
         * @see @ref flags(), @ref Configuration::setFlags()
         */
        typedef Containers::EnumSet<Flag> Flags;
        #else
        /* Done this way to be prepared for possible future diversion of 2D
           and 3D flags (e.g. introducing 3D-specific features) */
        typedef Implementation::LineGLFlag Flag;
        typedef Implementation::LineGLFlags Flags;
        #endif

        /**
         * @brief Compile asynchronously
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
        LineCapStyle capStyle() const { return _capStyle; }

        /**
         * @brief Join style
         *
         * @see @ref Configuration::setJoinStyle()
         */
        LineJoinStyle joinStyle() const { return _joinStyle; }

        /**
         * @brief Material count
         *
         * Statically defined size of the @ref LineMaterialUniform uniform
         * buffer bound with @ref bindMaterialBuffer(). Has use only if
         * @ref Flag::UniformBuffers is set.
         * @see @ref Configuration::setMaterialCount()
         */
        UnsignedInt materialCount() const { return _materialCount; }

        /**
         * @brief Draw count
         *
         * Statically defined size of each of the
         * @ref TransformationProjectionUniform2D /
         * @ref TransformationProjectionUniform3D and @ref LineDrawUniform
         * uniform buffers bound with @ref bindTransformationProjectionBuffer()
         * and @ref bindDrawBuffer(). Has use only if @ref Flag::UniformBuffers
         * is set.
         * @see @ref Configuration::setDrawCount()
         */
        UnsignedInt drawCount() const { return _drawCount; }

        /**
         * @brief Set viewport size
         * @return Reference to self (for method chaining)
         *
         * Line width and smoothness set in either @ref setWidth() /
         * @ref setSmoothness() or @ref LineMaterialUniform::width /
         * @ref LineMaterialUniform::smoothness depends on this value --- i.e.,
         * a value of @cpp 1.0f @ce is one pixel only if @ref setViewportSize()
         * is called with the actual pixel size of the viewport. Initial value
         * is a zero vector.
         */
        LineGL<dimensions>& setViewportSize(const Vector2& size);

        /** @{
         * @name Uniform setters
         *
         * Used only if @ref Flag::UniformBuffers is not set.
         */

        /**
         * @brief Set transformation and projection matrix
         * @return Reference to self (for method chaining)
         *
         * Initial value is an identity matrix. If
         * @ref Flag::InstancedTransformation is set, the per-instance
         * transformation matrix coming from the @ref TransformationMatrix
         * attribute is applied first, before this one.
         *
         * Expects that @ref Flag::UniformBuffers is not set, in that case fill
         * @ref TransformationProjectionUniform2D::transformationProjectionMatrix /
         * @ref TransformationProjectionUniform3D::transformationProjectionMatrix
         * and call @ref bindTransformationProjectionBuffer() instead.
         */
        // TODO split projection for 3D? would that mean a dedicated 3D class? uhhhhhhhhh
        LineGL<dimensions>& setTransformationProjectionMatrix(const MatrixTypeFor<dimensions, Float>& matrix);

        /**
         * @brief Set background color
         * @return Reference to self (for method chaining)
         *
         * Initial value is @cpp 0x00000000_rgbaf @ce. Used for edge smoothing
         * if smoothness is non-zero, and for background areas if
         * @ref CapStyle::Round or @ref CapStyle::Triangle is used. If
         * smoothness is zero and @ref CapStyle::Butt or @ref CapStyle::Square
         * is used, only the foreground color is used.
         *
         * Expects that @ref Flag::UniformBuffers is not set, in that case fill
         * @ref LineMaterialUniform::backgroundColor and call
         * @ref bindMaterialBuffer() instead.
         * @see @ref setColor(), @ref setSmoothness(),
         *      @ref Configuration::setCapStyle()
         */
        LineGL<dimensions>& setBackgroundColor(const Magnum::Color4& color);

        /**
         * @brief Set color
         * @return Reference to self (for method chaining)
         *
         * Initial value is @cpp 0xffffffff_rgbaf @ce.
         *
         * Expects that @ref Flag::UniformBuffers is not set, in that case fill
         * @ref LineMaterialUniform::color and call @ref bindMaterialBuffer()
         * instead.
         * @see @ref setBackgroundColor()
         */
        LineGL<dimensions>& setColor(const Magnum::Color4& color);

        /**
         * @brief Set line width
         * @return Reference to self (for method chaining)
         *
         * Screen-space, interpreted depending on the viewport size --- i.e.,
         * a value of @cpp 1.0f @ce is one pixel only if @ref setViewportSize()
         * is called with the actual pixel size of the viewport. Initial value
         * is @cpp 1.0f @ce.
         *
         * Expects that @ref Flag::UniformBuffers is not set, in that case fill
         * @ref LineMaterialUniform::width and call @ref bindMaterialBuffer()
         * instead.
         */
        LineGL<dimensions>& setWidth(Float width);

        /**
         * @brief Set line smoothness
         * @return Reference to self (for method chaining)
         *
         * Larger values will make edges look less aliased (but blurry),
         * smaller values will make them more crisp (but possibly aliased).
         * Screen-space, interpreted depending on the viewport size --- i.e.,
         * a value of @cpp 1.0f @ce is one pixel only if @ref setViewportSize()
         * is called with the actual pixel size of the viewport. Initial value
         * is @cpp 0.0f @ce.
         *
         * Expects that @ref Flag::UniformBuffers is not set, in that case fill
         * @ref LineMaterialUniform::smoothness and call @ref bindMaterialBuffer()
         * instead.
         */
        LineGL<dimensions>& setSmoothness(Float smoothness);

        /**
         * @brief Set miter length limit
         * @return Reference to self (for method chaining)
         *
         * Maximum length (relative to line width) over which a
         * @ref JoinStyle::Miter join is converted to a @ref JoinStyle::Bevel
         * in order to avoid sharp corners extending too much. Default value is
         * @cpp 4.0f @ce, which corresponds to approximately 29 degrees.
         * Alternatively you can set the limit as an angle using
         * @ref setMiterAngleLimit(). Miter length is calculated using the
         * following formula, where @f$ w @f$ is line half-width, @f$ l @f$ is
         * miter length and @f$ \theta @f$ is angle between two line segments:
         * @f[
         *      \frac{w}{l} = \sin(\frac{\theta}{2})
         * @f]
         *
         * Expects that @ref joinStyle() is @ref JoinStyle::Miter and @p limit
         * is greater or equal to @cpp 1.0f @ce and finite. Expects that
         * @ref Flag::UniformBuffers is not set, in that case fill
         * @ref LineMaterialUniform::miterLimit using
         * @ref LineMaterialUniform::setMiterLengthLimit() and call
         * @ref bindMaterialBuffer() instead.
         */
        LineGL<dimensions>& setMiterLengthLimit(Float limit);

        /**
         * @brief Set miter angle limit
         * @return Reference to self (for method chaining)
         *
         * Like @ref setMiterLengthLimit(), but specified as a minimum angle
         * below which a @ref JoinStyle::Miter join is converted to a
         * @ref JoinStyle::Bevel in order to avoid sharp corners extending too
         * much. Default value is approximately @cpp 28.955_degf @ce, see the
         * above function for more information.
         *
         * Expects that @ref joinStyle() is @ref JoinStyle::Miter and @p limit
         * is greater than @cpp 0.0_radf @ce. Expects that
         * @ref Flag::UniformBuffers is not set, in that case fill
         * @ref LineMaterialUniform::miterLimit using
         * @ref LineMaterialUniform::setMiterAngleLimit() and call
         * @ref bindMaterialBuffer() instead.
         */
        LineGL<dimensions>& setMiterAngleLimit(Rad limit);

        /**
         * @brief Set object ID
         * @return Reference to self (for method chaining)
         *
         * Expects that the shader was created with @ref Flag::ObjectId
         * enabled. Value set here is written to the @ref ObjectIdOutput, see
         * @ref Shaders-LineGL-object-id for more information. Initial value
         * is @cpp 0 @ce. If @ref Flag::InstancedObjectId is enabled as well,
         * this value is added to the ID coming from the @ref ObjectId
         * attribute.
         *
         * Expects that @ref Flag::UniformBuffers is not set, in that case fill
         * @ref LineDrawUniform::objectId and call @ref bindDrawBuffer()
         * instead.
         */
        LineGL<dimensions>& setObjectId(UnsignedInt id);

        /**
         * @}
         */

        /** @{
         * @name Uniform buffer binding and related uniform setters
         *
         * Used if @ref Flag::UniformBuffers is set.
         */

        /**
         * @brief Bind a draw offset
         * @return Reference to self (for method chaining)
         *
         * Specifies which item in the @ref TransformationProjectionUniform2D /
         * @ref TransformationProjectionUniform3D and @ref LineDrawUniform
         * buffers bound with @ref bindTransformationProjectionBuffer() and
         * @ref bindDrawBuffer() should be used for current draw. Expects that
         * @ref Flag::UniformBuffers is set and @p offset is less than
         * @ref drawCount(). Initial value is @cpp 0 @ce, if @ref drawCount()
         * is @cpp 1 @ce, the function is a no-op as the shader assumes draw
         * offset to be always zero.
         *
         * If @ref Flag::MultiDraw is set, @glsl gl_DrawID @ce is added to this
         * value, which makes each draw submitted via
         * @ref GL::AbstractShaderProgram::draw(const Containers::Iterable<MeshView>&)
         * pick up its own per-draw parameters.
         * @requires_gl31 Extension @gl_extension{ARB,uniform_buffer_object}
         * @requires_gles30 Uniform buffers are not available in OpenGL ES 2.0.
         * @requires_webgl20 Uniform buffers are not available in WebGL 1.0.
         */
        LineGL<dimensions>& setDrawOffset(UnsignedInt offset);

        /**
         * @brief Bind a transformation and projection uniform buffer
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref Flag::UniformBuffers is set. The buffer is
         * expected to contain @ref drawCount() instances of
         * @ref TransformationProjectionUniform2D /
         * @ref TransformationProjectionUniform3D. At the very least you need
         * to call also @ref bindDrawBuffer() and @ref bindMaterialBuffer().
         * @requires_gl31 Extension @gl_extension{ARB,uniform_buffer_object}
         */
        LineGL<dimensions>& bindTransformationProjectionBuffer(GL::Buffer& buffer);
        LineGL<dimensions>& bindTransformationProjectionBuffer(GL::Buffer& buffer, GLintptr offset, GLsizeiptr size); /**< @overload */

        /**
         * @brief Bind a draw uniform buffer
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref Flag::UniformBuffers is set. The buffer is
         * expected to contain @ref drawCount() instances of
         * @ref LineDrawUniform. At the very least you need to call also
         * @ref bindTransformationProjectionBuffer() and
         * @ref bindMaterialBuffer().
         * @requires_gl31 Extension @gl_extension{ARB,uniform_buffer_object}
         */
        LineGL<dimensions>& bindDrawBuffer(GL::Buffer& buffer);
        LineGL<dimensions>& bindDrawBuffer(GL::Buffer& buffer, GLintptr offset, GLsizeiptr size); /**< @overload */

        /**
         * @brief Bind a material uniform buffer
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref Flag::UniformBuffers is set. The buffer is
         * expected to contain @ref materialCount() instances of
         * @ref LineMaterialUniform. At the very least you need to call also
         * @ref bindTransformationProjectionBuffer() and @ref bindDrawBuffer().
         * @requires_gl31 Extension @gl_extension{ARB,uniform_buffer_object}
         */
        LineGL<dimensions>& bindMaterialBuffer(GL::Buffer& buffer);
        LineGL<dimensions>& bindMaterialBuffer(GL::Buffer& buffer, GLintptr offset, GLsizeiptr size); /**< @overload */

        /**
         * @}
         */

        MAGNUM_GL_ABSTRACTSHADERPROGRAM_SUBCLASS_DRAW_IMPLEMENTATION(LineGL<dimensions>)

    private:
        /* Creates the GL shader program object but does nothing else.
           Internal, used by compile(). */
        explicit LineGL(NoInitT);

        Flags _flags;
        LineCapStyle _capStyle;
        LineJoinStyle _joinStyle;
        UnsignedInt _materialCount{},
            _drawCount{};
        Int _viewportSizeUniform{0},
            _transformationProjectionMatrixUniform{1},
            _backgroundColorUniform{2},
            _colorUniform{3},
            _widthUniform{4},
            _smoothnessUniform{5},
            _miterLimitUniform{6},
            _objectIdUniform{7},
            /* Used instead of all other uniforms except viewportSize when
               Flag::UniformBuffers is set, so it can alias them */
            _drawOffsetUniform{1};
};

/**
@brief Configuration

@see @ref LineGL(const Configuration&), @ref compile(const Configuration&)
*/
template<UnsignedInt dimensions> class MAGNUM_SHADERS_EXPORT LineGL<dimensions>::Configuration {
    public:
        explicit Configuration();

        /** @brief Flags */
        Flags flags() const { return _flags; }

        /**
         * @brief Set flags
         *
         * No flags are set by default.
         * @see @ref LineGL::flags()
         */
        Configuration& setFlags(Flags flags) {
            _flags = flags;
            return *this;
        }

        /** @brief Cap style */
        LineCapStyle capStyle() const { return _capStyle; }

        /**
         * @brief Set cap style
         *
         * Unlike for example the SVG specification that uses
         * @ref LineCapStyle::Butt by default, the default value is
         * @ref LineCapStyle::Square, in order to make zero-length lines visible.
         * @see @ref LineGL::capStyle()
         */
        Configuration& setCapStyle(LineCapStyle style) {
            _capStyle = style;
            return *this;
        }

        /** @brief Join style */
        LineJoinStyle joinStyle() const { return _joinStyle; }

        /**
         * @brief Set join style
         *
         * Default value is @ref LineJoinStyle::Miter, consistently with the
         * SVG specification.
         * @see @ref LineGL::joinStyle()
         */
        Configuration& setJoinStyle(LineJoinStyle style) {
            _joinStyle = style;
            return *this;
        }

        /** @brief Material count */
        UnsignedInt materialCount() const { return _materialCount; }

        /**
         * @brief Set material count
         *
         * If @ref Flag::UniformBuffers is set, describes size of a
         * @ref LineMaterialUniform buffer bound with
         * @ref bindMaterialBuffer(); as uniform buffers are required to have a
         * statically defined size. The per-draw materials are then specified
         * via @ref LineDrawUniform::materialId. Default value is @cpp 1 @ce.
         *
         * If @ref Flag::UniformBuffers isn't set, this value is ignored.
         * @see @ref setFlags(), @ref setDrawCount(),
         *      @ref LineGL::materialCount()
         * @requires_gl31 Extension @gl_extension{ARB,uniform_buffer_object}
         */
        Configuration& setMaterialCount(UnsignedInt count) {
            _materialCount = count;
            return *this;
        }

        /** @brief Draw count */
        UnsignedInt drawCount() const { return _drawCount; }

        /**
         * @brief Set draw count
         *
         * If @ref Flag::UniformBuffers is set, describes size of a
         * @ref TransformationProjectionUniform2D /
         * @ref TransformationProjectionUniform3D /
         * @ref LineDrawUniform buffer bound with
         * @ref bindTransformationProjectionBuffer() and @ref bindDrawBuffer();
         * as uniform buffers are required to have a statically defined size.
         * The draw offset is then set via @ref setDrawOffset(). Default value
         * is @cpp 1 @ce.
         *
         * If @ref Flag::UniformBuffers isn't set, this value is ignored.
         * @see @ref setFlags(), @ref setMaterialCount(),
         *      @ref LineGL::drawCount()
         * @requires_gl31 Extension @gl_extension{ARB,uniform_buffer_object}
         */
        Configuration& setDrawCount(UnsignedInt count) {
            _drawCount = count;
            return *this;
        }

    private:
        Flags _flags;
        LineCapStyle _capStyle;
        LineJoinStyle _joinStyle;
        UnsignedInt _materialCount{1};
        UnsignedInt _drawCount{1};
};

/**
@brief Asynchronous compilation state

Returned by @ref compile(). See @ref shaders-async for more information.
*/
template<UnsignedInt dimensions> class LineGL<dimensions>::CompileState: public LineGL<dimensions> {
    /* Everything deliberately private except for the inheritance */
    friend class LineGL;

    explicit CompileState(NoCreateT): LineGL{NoCreate}, _vert{NoCreate}, _frag{NoCreate} {}

    explicit CompileState(LineGL<dimensions>&& shader, GL::Shader&& vert, GL::Shader&& frag
        #ifndef MAGNUM_TARGET_GLES
        , GL::Version version
        #endif
    ): LineGL<dimensions>{std::move(shader)}, _vert{std::move(vert)}, _frag{std::move(frag)}
        #ifndef MAGNUM_TARGET_GLES
        , _version{version}
        #endif
        {}

    Implementation::GLShaderWrapper _vert, _frag;
    #ifndef MAGNUM_TARGET_GLES
    GL::Version _version;
    #endif
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
/**
 * @debugoperatorclassenum{LineGL,LineGL::Flag}
 * @m_since_latest
 */
template<UnsignedInt dimensions> Debug& operator<<(Debug& debug, LineGL<dimensions>::Flag value);

/**
 * @debugoperatorclassenum{LineGL,LineGL::Flags}
 * @m_since_latest
 */
template<UnsignedInt dimensions> Debug& operator<<(Debug& debug, LineGL<dimensions>::Flags value);
#else
namespace Implementation {
    MAGNUM_SHADERS_EXPORT Debug& operator<<(Debug& debug, LineGLFlag value);
    MAGNUM_SHADERS_EXPORT Debug& operator<<(Debug& debug, LineGLFlags value);
}
#endif

}}
#else
#error this header is not available in the OpenGL ES 2.0 / WebGL 1.0 build
#endif

#endif
