#ifndef Magnum_Shaders_Line_h
#define Magnum_Shaders_Line_h
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
 * @brief Struct @ref Magnum::Shaders::LineDrawUniform, @ref Magnum::Shaders::LineMaterialUniform, enum @ref Magnum::Shaders::LineCapStyle, @ref Magnum::Shaders::LineJoinStyle
 * @m_since_latest
 */

#include "Magnum/Magnum.h"
#include "Magnum/Math/Color.h"
#include "Magnum/Shaders/visibility.h"

namespace Magnum { namespace Shaders {

/**
@brief Line cap style
@m_since_latest

@see @ref LineGL::capStyle(), @ref LineGL::Configuration::setCapStyle()
*/
enum class LineCapStyle: UnsignedByte {
    /**
     * [Butt cap](https://en.wikipedia.org/wiki/Butt_joint). The line is cut
     * off right at the endpoint. Lines of zero length will be invisible.
     *
     * @ref TODOTODO image
     */
    Butt,

    /**
     * Square cap. The line is extended by half of its width past the endpoint.
     * Lines of zero length will be shown as squares.
     *
     * @ref TODOTODO image
     */
    Square,

    /**
     * Round cap. The line is extended by half of its width past the endpoint.
     * It's still rendered as a quad but pixels outside of the half-circle have
     * the background color. Lines of zero length will be shown as circles.
     *
     * @ref TODOTODO image
     *
     * @see @ref LineMaterialUniform::backgroundColor,
     *      @ref LineGL::setBackgroundColor()
     */
    Round,

    /**
     * Triangle cap. The line is extended by half of its width past the
     * endpoint. It's still rendered as a quad but pixels outside of the
     * triangle have the background color. Lines of zero length will be shown
     * as squares rotated by 45°.
     *
     * @ref TODOTODO image
     *
     * @see @ref LineMaterialUniform::backgroundColor,
     *      @ref LineGL::setBackgroundColor(),
     *
     */
    Triangle
};

/**
@brief Line join style
@m_since_latest

@see @ref LineGL::joinStyle(), @ref LineGL::Configuration::setJoinStyle()
*/
enum class LineJoinStyle: UnsignedByte {
    /**
     * [Miter join](https://en.wikipedia.org/wiki/Miter_joint). The outer edges
     * of both line segments extend until they intersect. If the miter would be
     * longer than what the limit set in
     * @ref LineMaterialUniform::setMiterLengthLimit() /
     * @ref LineMaterialUniform::setMiterAngleLimit() or
     * @ref LineGL::setMiterLengthLimit() / @ref LineGL::setMiterAngleLimit()
     * allows, it switches to @ref LineJoinStyle::Bevel instead.
     *
     * @ref TODOTODO image
     */
    Miter,

    /**
     * [Bevel join](https://en.wikipedia.org/wiki/Bevel). Outer edges of both
     * line segments are cut off at a right angle at their endpoints and the
     * area in between is filled with an extra triangle.
     *
     * @ref TODOTODO image
     */
    Bevel
};

/**
 * @debugoperatorenum{LineCapStyle}
 * @m_since_latest
 */
MAGNUM_SHADERS_EXPORT Debug& operator<<(Debug& debug, LineCapStyle value);

/**
 * @debugoperatorenum{LineJoinStyle}
 * @m_since_latest
 */
MAGNUM_SHADERS_EXPORT Debug& operator<<(Debug& debug, LineJoinStyle value);

struct LineDrawUniform {
    /** @brief Construct with default parameters */
    constexpr explicit LineDrawUniform(DefaultInitT = DefaultInit) noexcept: materialId{0}, objectId{0} {}

    /** @brief Construct without initializing the contents */
    explicit LineDrawUniform(NoInitT) noexcept {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref materialId field
     * @return Reference to self (for method chaining)
     */
    LineDrawUniform& setMaterialId(UnsignedInt id) {
        materialId = id;
        return *this;
    }

    /**
     * @brief Set the @ref objectId field
     * @return Reference to self (for method chaining)
     */
    LineDrawUniform& setObjectId(UnsignedInt id) {
        objectId = id;
        return *this;
    }

    /**
     * @}
     */

    /** @var materialId
     * @brief Material ID
     *
     * References a particular material from a @ref LineMaterialUniform array.
     * Useful when an UBO with more than one material is supplied or in a
     * multi-draw scenario. Should be less than the material count passed to
     * @ref LineGL::Configuration::setMaterialCount(), if material count is
     * @cpp 1 @ce, this field is assumed to be @cpp 0 @ce and isn't even read
     * by the shader. Default value is @cpp 0 @ce, meaning the first material
     * gets used.
     */

    /* This field is an UnsignedInt in the shader and materialId is extracted
       as (value & 0xffff), so the order has to be different on BE */
    #ifndef CORRADE_TARGET_BIG_ENDIAN
    alignas(4) UnsignedShort materialId;
    /* warning: Member __pad0__ is not documented. FFS DOXYGEN WHY DO YOU THINK
       I MADE THOSE UNNAMED, YOU DUMB FOOL */
    #ifndef DOXYGEN_GENERATING_OUTPUT
    UnsignedShort:16; /* reserved */
    #endif
    #else
    alignas(4) UnsignedShort:16; /* reserved */
    UnsignedShort materialId;
    #endif

    /**
     * @brief Object ID
     *
     * Used only for the object ID framebuffer output, not to access any other
     * uniform data. Default value is @cpp 0 @ce.
     *
     * Used only if @ref LineGL::Flag::ObjectId is enabled, ignored otherwise.
     * If @ref LineGL::Flag::InstancedObjectId is enabled as well, this value
     * is added to the ID coming from the @ref LineGL::ObjectId attribute.
     * @see @ref LineGL::setObjectId()
     */
    UnsignedInt objectId;

    /* warning: Member __pad1__ is not documented. FFS DOXYGEN WHY DO YOU THINK
       I MADE THOSE UNNAMED, YOU DUMB FOOL */
    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    Int:32;
    #endif
};

/**
@brief Material uniform for line shaders
@m_since_latest

Describes material properties referenced from
@ref LineDrawUniform::materialId.
@see @ref LineGL::bindMaterialBuffer()
*/
struct MAGNUM_SHADERS_EXPORT LineMaterialUniform {
    /** @brief Construct with default parameters */
    constexpr explicit LineMaterialUniform(DefaultInitT = DefaultInit) noexcept: backgroundColor{0.0f, 0.0f, 0.0f, 0.0f}, color{1.0f, 1.0f, 1.0f, 1.0f}, width{1.0f}, smoothness{0.0f}, miterLimit{0.875f} {}

    /** @brief Construct without initializing the contents */
    explicit LineMaterialUniform(NoInitT) noexcept: color{NoInit} {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref color field
     * @return Reference to self (for method chaining)
     */
    LineMaterialUniform& setColor(const Color4& color) {
        this->color = color;
        return *this;
    }

    /**
     * @brief Set the @ref backgroundColor field
     * @return Reference to self (for method chaining)
     */
    LineMaterialUniform& setBackgroundColor(const Color4& color) {
        backgroundColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref width field
     * @return Reference to self (for method chaining)
     */
    LineMaterialUniform& setWidth(Float width) {
        this->width = width;
        return *this;
    }

    /**
     * @brief Set the @ref smoothness field
     * @return Reference to self (for method chaining)
     */
    LineMaterialUniform& setSmoothness(Float smoothness) {
        this->smoothness = smoothness;
        return *this;
    }

    /**
     * @brief Set the @ref miterLimit field to a length value
     * @return Reference to self (for method chaining)
     *
     * @ref TODO document asserts
     */
    LineMaterialUniform& setMiterLengthLimit(Float limit);

    /**
     * @brief Set the @ref miterLimit field to an angle value
     * @return Reference to self (for method chaining)
     *
     * @ref TODO document asserts
     */
    LineMaterialUniform& setMiterAngleLimit(Rad limit);

    /**
     * @}
     */

    /**
     * @brief Background color
     *
     * Default value is @cpp 0x00000000_rgbaf @ce. Used for edge smoothing if
     * smoothness is non-zero, and for background areas if
     * @ref LineCapStyle::Round or @ref LineCapStyle::Triangle is used. If
     * smoothness is zero and @ref LineCapStyle::Butt or
     * @ref LineCapStyle::Square is used, only the foreground color is used.
     * @see @ref LineGL::setBackgroundColor(), @ref LineGL::setSmoothness(),
     *      @ref LineGL::Configuration::setCapStyle()
     */
    Color4 backgroundColor;

    /**
     * @brief Color
     *
     * Default value is @cpp 0xffffffff_rgbaf @ce.
     *
     * If @ref LineGL::Flag::VertexColor is enabled, the color is multiplied
     * with a color coming from the @ref LineGL::Color3 / @ref LineGL::Color4
     * attribute.
     * @see @ref LineGL::setColor()
     */
    Color4 color;

    /**
     * @brief Line width
     *
     * Screen-space, interpreted depending on the viewport size --- i.e., a
     * value of @cpp 1.0f @ce is one pixel only if @ref LineGL::setViewportSize()
     * is called with the actual pixel size of the viewport. Default value is
     * @cpp 1.0f @ce.
     * @see @ref LineGL::setWidth()
     */
    Float width;

    /**
     * @brief Line smoothness
     *
     * Larger values will make edges look less aliased (but blurry), smaller
     * values will make them more crisp (but possibly aliased). Screen-space,
     * interpreted depending on the viewport size --- i.e., a value of
     * @cpp 1.0f @ce is one pixel only if @ref LineGL::setViewportSize() is
     * called with the actual pixel size of the viewport. Initial value is
     * @cpp 0.0f @ce.
     * @see @ref LineGL::setSmoothness()
     */
    Float smoothness;

    /**
     * @brief Miter limit
     *
     * @ref TODOTODO document what is this (cosine of an angle), use a common
     * helper in the setters
     *
     * @see @ref LineGL::setMiterLengthLimit(),
     *      @ref LineGL::setMiterLengthLimit()
     */
    Float miterLimit;

    /* warning: Member __pad0__ is not documented. FFS DOXYGEN WHY DO YOU THINK
       I MADE THOSE UNNAMED, YOU DUMB FOOL */
    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32; /* reserved for dynamic cap/join style */
    #endif
};

}}

#endif
