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

#include <sstream>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Path.h>
#include <Corrade/Utility/System.h>

#include "Magnum/Image.h"
#include "Magnum/PixelFormat.h"
#include "Magnum/DebugTools/CompareImage.h"
#include "Magnum/GL/Buffer.h"
#include "Magnum/GL/Extensions.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/Mesh.h"
#include "Magnum/GL/OpenGLTester.h"
#include "Magnum/GL/Renderbuffer.h"
#include "Magnum/GL/RenderbufferFormat.h"
#include "Magnum/Math/Color.h"
#include "Magnum/Math/Complex.h"
#include "Magnum/Math/FunctionsBatch.h"
#include "Magnum/Math/Matrix3.h"
#include "Magnum/Math/Matrix4.h"
#include "Magnum/Shaders/Generic.h"
#include "Magnum/Shaders/Line.h"
#include "Magnum/Shaders/LineGL.h"
#include "Magnum/Trade/AbstractImporter.h"

#include "configure.h"

namespace Magnum { namespace Shaders { namespace Test { namespace {

struct LineGLTest: GL::OpenGLTester {
    explicit LineGLTest();

    template<UnsignedInt dimensions> void construct();
    template<UnsignedInt dimensions> void constructAsync();
    #ifndef MAGNUM_TARGET_GLES2
    template<UnsignedInt dimensions> void constructUniformBuffers();
    template<UnsignedInt dimensions> void constructUniformBuffersAsync();
    #endif

    template<UnsignedInt dimensions> void constructMove();
    #ifndef MAGNUM_TARGET_GLES2
    template<UnsignedInt dimensions> void constructMoveUniformBuffers();
    #endif

    // template<UnsignedInt dimensions> void constructInvalid();
    #ifndef MAGNUM_TARGET_GLES2
    template<UnsignedInt dimensions> void constructUniformBuffersInvalid();
    #endif

    #ifndef MAGNUM_TARGET_GLES2
    template<UnsignedInt dimensions> void setUniformUniformBuffersEnabled();
    template<UnsignedInt dimensions> void bindBufferUniformBuffersNotEnabled();
    #endif
    template<UnsignedInt dimensions> void setMiterLengthLimitInvalid();
    template<UnsignedInt dimensions> void setMiterAngleLimitInvalid();
    #ifndef MAGNUM_TARGET_GLES2
    template<UnsignedInt dimensions> void setObjectIdNotEnabled();
    #endif
    #ifndef MAGNUM_TARGET_GLES2
    template<UnsignedInt dimensions> void setWrongDrawOffset();
    #endif

    void renderSetup();
    void renderTeardown();

    template<LineGL2D::Flag flag = LineGL2D::Flag{}> void renderDefaults2D();
    template<LineGL3D::Flag flag = LineGL3D::Flag{}> void renderDefaults3D();

    template<LineGL2D::Flag flag = LineGL2D::Flag{}> void renderLineCapsJoins2D();
    void renderLineCapsJoins2DReversed();
    void renderLineCapsJoins2DTransformed();

    template<LineGL3D::Flag flag = LineGL3D::Flag{}> void render3D();

    template<class T, LineGL2D::Flag flag = LineGL2D::Flag{}> void renderVertexColor2D();
    template<class T, LineGL3D::Flag flag = LineGL3D::Flag{}> void renderVertexColor3D();

    #ifndef MAGNUM_TARGET_GLES2
    void renderObjectIdSetup();
    void renderObjectIdTeardown();

    template<LineGL2D::Flag flag = LineGL2D::Flag{}> void renderObjectId2D();
    template<LineGL3D::Flag flag = LineGL3D::Flag{}> void renderObjectId3D();
    #endif

    template<LineGL2D::Flag flag = LineGL2D::Flag{}> void renderInstanced2D();
    template<LineGL3D::Flag flag = LineGL3D::Flag{}> void renderInstanced3D();

    #ifndef MAGNUM_TARGET_GLES2
    void renderMulti2D();
    void renderMulti3D();
    #endif

    private:
        PluginManager::Manager<Trade::AbstractImporter> _manager{"nonexistent"};

        GL::Renderbuffer _color{NoCreate};
        #ifndef MAGNUM_TARGET_GLES2
        GL::Renderbuffer _objectId{NoCreate};
        #endif
        GL::Framebuffer _framebuffer{NoCreate};
};

using namespace Containers::Literals;
using namespace Math::Literals;

const struct {
    const char* name;
    LineGL2D::Flags flags;
    // TODO cap/join style
} ConstructData[]{
    {"", {}},
    {"vertex colors", LineGL2D::Flag::VertexColor},
    #ifndef MAGNUM_TARGET_GLES2
    {"object ID", LineGL2D::Flag::ObjectId},
    {"instanced object ID", LineGL2D::Flag::InstancedObjectId},
    #endif
    {"instanced transformation", LineGL2D::Flag::InstancedTransformation},
};

#ifndef MAGNUM_TARGET_GLES2
const struct {
    const char* name;
    LineGL2D::Flags flags;
    // TODO cap/join style
    UnsignedInt materialCount, drawCount;
} ConstructUniformBuffersData[]{
    {"classic fallback", {}, 1, 1},
    {"", LineGL2D::Flag::UniformBuffers, 1, 1},
    /* SwiftShader has 256 uniform vectors at most, per-draw is 4+1 in 3D case
       and 3+1 in 2D, per-material 1 */
    {"multiple materials, draws", LineGL2D::Flag::UniformBuffers, 16, 48},
    {"object ID", LineGL2D::Flag::UniformBuffers|LineGL2D::Flag::ObjectId, 1, 1},
    {"instanced object ID", LineGL2D::Flag::UniformBuffers|LineGL2D::Flag::InstancedObjectId, 1, 1},
    {"multidraw with all the things", LineGL2D::Flag::MultiDraw|LineGL2D::Flag::ObjectId|LineGL2D::Flag::InstancedTransformation|LineGL2D::Flag::InstancedObjectId, 16, 48}
};
#endif

// const struct {
//     const char* name;
//     LineGL2D::Flags flags;
//     const char* message;
// } ConstructInvalidData[]{
// };

#ifndef MAGNUM_TARGET_GLES2
const struct {
    const char* name;
    LineGL2D::Flags flags;
    UnsignedInt materialCount, drawCount;
    const char* message;
} ConstructUniformBuffersInvalidData[]{
    {"zero draws", LineGL2D::Flag::UniformBuffers, 1, 0,
        "draw count can't be zero"},
    {"zero materials", LineGL2D::Flag::UniformBuffers, 0, 1,
        "material count can't be zero"}
};
#endif

const struct {
    const char* name;
    LineGL2D::JoinStyle joinStyle;
    Float limit;
    const char* message;
} SetMiterLengthLimitInvalidData[]{
    {"wrong join style", LineGL2D::JoinStyle::Bevel, 1.0f,
        "the shader was created with Shaders::LineGL::JoinStyle::Bevel"},
    {"too short", LineGL2D::JoinStyle::Miter, 0.9997f,
        "expected a finite value greater than or equal to 1, got 0.9997"},
    {"too long", LineGL2D::JoinStyle::Miter, Constants::inf(),
        "expected a finite value greater than or equal to 1, got inf"},
};

const struct {
    const char* name;
    LineGL2D::JoinStyle joinStyle;
    Rad limit;
    const char* message;
} SetMiterAngleLimitInvalidData[]{
    {"wrong join style", LineGL2D::JoinStyle::Bevel, 90.0_degf,
        "the shader was created with Shaders::LineGL::JoinStyle::Bevel"},
    {"too small", LineGL2D::JoinStyle::Miter, 0.0_degf,
        "expected a value greater than 0, got Rad(0)"}
};

const struct {
    const char* name;
    Float width;
    Float smoothness;
    Containers::Optional<Float> miterLengthLimit;
    Containers::Optional<Deg> miterAngleLimit;
    Containers::Optional<LineGL2D::CapStyle> capStyle;
    Containers::Optional<LineGL2D::JoinStyle> joinStyle;
    const char* expected;
} RenderLineCapsJoins2DData[]{
    {"caps & joints default, flat",
        16.0f, 0.0f, {}, {}, {}, {},
        "caps-square-joins-miter-flat.tga"},
    {"caps butt, joins default, flat",
        16.0f, 0.0f, {}, {},
        LineGL2D::CapStyle::Butt, {},
        "caps-butt-joins-miter-flat.tga"},
    {"caps butt, joins bevel",
        16.0f, 1.0f, {}, {},
        LineGL2D::CapStyle::Butt, LineGL2D::JoinStyle::Bevel,
        "caps-butt-joins-bevel.tga"}, // TODO the smoothing is off for 30°
    {"caps square, joins miter",
        16.0f, 1.0f, {}, {},
        LineGL2D::CapStyle::Square, LineGL2D::JoinStyle::Miter,
        "caps-square-joins-miter.tga"},
    {"caps square, joins bevel",
        16.0f, 1.0f, {}, {},
        LineGL2D::CapStyle::Square, LineGL2D::JoinStyle::Bevel,
        "caps-square-joins-bevel.tga"}, // TODO the smoothing is off for 30°
    {"caps square, joins miter, miter limit 3.95",
        16.0f, 1.0f, 3.95f, {},
        LineGL2D::CapStyle::Square, LineGL2D::JoinStyle::Miter,
        /* Same as default */
        "caps-square-joins-miter.tga"},
    {"caps square, joins miter, miter limit 3.6",
        16.0f, 1.0f, 3.6f, {}, /** @todo 3.85 should work but it doesn't */
        LineGL2D::CapStyle::Square, LineGL2D::JoinStyle::Miter,
        /* The 30° join should get a bevel here */
        "caps-square-joins-miter-limit-36.tga"},
    {"caps square, joins miter, miter limit 59°",
        16.0f, 1.0f, {}, 59.0_degf,
        LineGL2D::CapStyle::Square, LineGL2D::JoinStyle::Miter,
        /* Same as limit 3.6, the 30° join gets a bevel */
        "caps-square-joins-miter-limit-36.tga"},
    {"caps square, joins miter, miter limit 70°",
        16.0f, 1.0f, {}, 70.0_degf, /** @todo 61° should work but it doesn't */
        LineGL2D::CapStyle::Square, LineGL2D::JoinStyle::Miter,
        /* The 30° and 60° join should get a bevel here, 90° and 120° should
           stay */
        "caps-square-joins-miter-limit-70deg.tga"},
    {"caps square, joins miter, miter limit 89°",
        16.0f, 1.0f, {}, 89.0_degf,
        LineGL2D::CapStyle::Square, LineGL2D::JoinStyle::Miter,
        /* Same as limit 61°, the 30° and 60° joins get a bevel, 90° and 120°
           not */
        "caps-square-joins-miter-limit-70deg.tga"},
    {"caps square, joins miter, miter limit 91°",
        16.0f, 1.0f, {}, 91.0_degf,
        LineGL2D::CapStyle::Square, LineGL2D::JoinStyle::Miter,
        /* The 30°, 60° and 90° join should get a bevel here, 120° should
           stay */
        "caps-square-joins-miter-limit-91deg.tga"},
    {"caps round, joins miter",
        /** @todo use round joins instead once implemented */
        16.0f, 1.0f, {}, {},
        LineGL2D::CapStyle::Round, LineGL2D::JoinStyle::Miter,
        "caps-round-joins-miter.tga"},
    {"caps triangle, joins bevel",
        16.0f, 1.0f, {}, {},
        LineGL2D::CapStyle::Triangle, LineGL2D::JoinStyle::Bevel,
        "caps-triangle-joins-bevel.tga"},
};

LineGLTest::LineGLTest() {
    addInstancedTests<LineGLTest>({
        &LineGLTest::construct<2>,
        // &LineGLTest::construct<3>

    },
        Containers::arraySize(ConstructData));

    addTests<LineGLTest>({
        &LineGLTest::constructAsync<2>,
        // &LineGLTest::constructAsync<3>

    });

    #ifndef MAGNUM_TARGET_GLES2
    // addInstancedTests<LineGLTest>({
    //     &LineGLTest::constructUniformBuffers<2>,
    //     &LineGLTest::constructUniformBuffers<3>},
    //     Containers::arraySize(ConstructUniformBuffersData));
    //
    // addTests<LineGLTest>({
    //     &LineGLTest::constructUniformBuffersAsync<2>,
    //     &LineGLTest::constructUniformBuffersAsync<3>});
    #endif

    addTests<LineGLTest>({
        &LineGLTest::constructMove<2>,
        // &LineGLTest::constructMove<3>,

        // #ifndef MAGNUM_TARGET_GLES2
        // &LineGLTest::constructMoveUniformBuffers<2>,
        // &LineGLTest::constructMoveUniformBuffers<3>,
        // #endif
        });

    // addInstancedTests<LineGLTest>({
    //     &LineGLTest::constructInvalid<2>,
    //     &LineGLTest::constructInvalid<3>},
    //     Containers::arraySize(ConstructInvalidData));

    #ifndef MAGNUM_TARGET_GLES2
    addInstancedTests<LineGLTest>({
        &LineGLTest::constructUniformBuffersInvalid<2>,
        // &LineGLTest::constructUniformBuffersInvalid<3>

    },
        Containers::arraySize(ConstructUniformBuffersInvalidData));
    #endif

    #ifndef MAGNUM_TARGET_GLES2
    addTests<LineGLTest>({
        // &LineGLTest::setUniformUniformBuffersEnabled<2>,
        // &LineGLTest::setUniformUniformBuffersEnabled<3>,
        &LineGLTest::bindBufferUniformBuffersNotEnabled<2>,
        // &LineGLTest::bindBufferUniformBuffersNotEnabled<3>,
    });
    #endif

    addInstancedTests<LineGLTest>({
        &LineGLTest::setMiterLengthLimitInvalid<2>,
        // &LineGLTest::setMiterLengthLimitInvalid<3>

    },
        Containers::arraySize(SetMiterLengthLimitInvalidData));

    addInstancedTests<LineGLTest>({
        &LineGLTest::setMiterAngleLimitInvalid<2>,
        // &LineGLTest::setMiterAngleLimitInvalid<3>

    },
        Containers::arraySize(SetMiterAngleLimitInvalidData));

    #ifndef MAGNUM_TARGET_GLES2
    addTests<LineGLTest>({&LineGLTest::setObjectIdNotEnabled<2>,
        // &LineGLTest::setObjectIdNotEnabled<3>,
        // &LineGLTest::setWrongDrawOffset<2>,
        // &LineGLTest::setWrongDrawOffset<3>

    });
    #endif

    /* MSVC needs explicit type due to default template args */
    addTests<LineGLTest>({
        &LineGLTest::renderDefaults2D,
        #ifndef MAGNUM_TARGET_GLES2
        // &LineGLTest::renderDefaults2D<LineGL2D::Flag::UniformBuffers>,
        #endif
        // &LineGLTest::renderDefaults3D,
        // #ifndef MAGNUM_TARGET_GLES2
        // &LineGLTest::renderDefaults3D<LineGL3D::Flag::UniformBuffers>,
        // #endif
        },
        &LineGLTest::renderSetup,
        &LineGLTest::renderTeardown);

    /* MSVC needs explicit type due to default template args */
    addInstancedTests<LineGLTest>({
        &LineGLTest::renderLineCapsJoins2D,
        #ifndef MAGNUM_TARGET_GLES2
        // &LineGLTest::renderLineCapsJoins2D<LineGL2D::Flag::UniformBuffers>,
        #endif
        &LineGLTest::renderLineCapsJoins2DReversed,
        &LineGLTest::renderLineCapsJoins2DTransformed,
        },
        Containers::arraySize(RenderLineCapsJoins2DData),
        &LineGLTest::renderSetup,
        &LineGLTest::renderTeardown);

    /* Load the plugins directly from the build tree. Otherwise they're either
       static and already loaded or not present in the build tree */
    #ifdef ANYIMAGEIMPORTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT_OUTPUT(_manager.load(ANYIMAGEIMPORTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif
    #ifdef TGAIMPORTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT_OUTPUT(_manager.load(TGAIMPORTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif
}

template<UnsignedInt dimensions> void LineGLTest::construct() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    #ifndef MAGNUM_TARGET_GLES
    if((data.flags & LineGL2D::Flag::ObjectId) && !GL::Context::current().isExtensionSupported<GL::Extensions::EXT::gpu_shader4>())
        CORRADE_SKIP(GL::Extensions::EXT::gpu_shader4::string() << "is not supported.");
    #endif

    LineGL<dimensions> shader{typename LineGL<dimensions>::Configuration{}
        .setFlags(data.flags)};
    CORRADE_COMPARE(shader.flags(), data.flags);
    CORRADE_VERIFY(shader.id());
    {
        #if defined(CORRADE_TARGET_APPLE) && !defined(MAGNUM_TARGET_GLES)
        CORRADE_EXPECT_FAIL("macOS drivers need insane amount of state to validate properly.");
        #endif
        CORRADE_VERIFY(shader.validate().first());
    }

    MAGNUM_VERIFY_NO_GL_ERROR();
}

template<UnsignedInt dimensions> void LineGLTest::constructAsync() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    typename LineGL<dimensions>::CompileState state = LineGL<dimensions>::compile(typename LineGL<dimensions>::Configuration{}
        .setFlags(LineGL2D::Flag::VertexColor));
    CORRADE_COMPARE(state.flags(),  LineGL2D::Flag::VertexColor);

    while(!state.isLinkFinished())
        Utility::System::sleep(100);

    LineGL<dimensions> shader{std::move(state)};
    CORRADE_COMPARE(shader.flags(), LineGL2D::Flag::VertexColor);

    CORRADE_VERIFY(shader.id());
    {
        #if defined(CORRADE_TARGET_APPLE) && !defined(MAGNUM_TARGET_GLES)
        CORRADE_EXPECT_FAIL("macOS drivers need insane amount of state to validate properly.");
        #endif
        CORRADE_VERIFY(shader.validate().first());
    }

    MAGNUM_VERIFY_NO_GL_ERROR();
}

#ifndef MAGNUM_TARGET_GLES2
template<UnsignedInt dimensions> void LineGLTest::constructUniformBuffers() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    auto&& data = ConstructUniformBuffersData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    #ifndef MAGNUM_TARGET_GLES
    if((data.flags & LineGL2D::Flag::UniformBuffers) && !GL::Context::current().isExtensionSupported<GL::Extensions::ARB::uniform_buffer_object>())
        CORRADE_SKIP(GL::Extensions::ARB::uniform_buffer_object::string() << "is not supported.");
    if((data.flags & LineGL2D::Flag::ObjectId) && !GL::Context::current().isExtensionSupported<GL::Extensions::EXT::gpu_shader4>())
        CORRADE_SKIP(GL::Extensions::EXT::gpu_shader4::string() << "is not supported.");
    #endif

    if(data.flags >= LineGL2D::Flag::MultiDraw) {
        #ifndef MAGNUM_TARGET_GLES
        if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::shader_draw_parameters>())
            CORRADE_SKIP(GL::Extensions::ARB::shader_draw_parameters::string() << "is not supported.");
        #elif !defined(MAGNUM_TARGET_WEBGL)
        if(!GL::Context::current().isExtensionSupported<GL::Extensions::ANGLE::multi_draw>())
            CORRADE_SKIP(GL::Extensions::ANGLE::multi_draw::string() << "is not supported.");
        #else
        if(!GL::Context::current().isExtensionSupported<GL::Extensions::WEBGL::multi_draw>())
            CORRADE_SKIP(GL::Extensions::WEBGL::multi_draw::string() << "is not supported.");
        #endif
    }

    LineGL<dimensions> shader{typename LineGL<dimensions>::Configuration{}
        .setFlags(data.flags)
        .setMaterialCount(data.materialCount)
        .setDrawCount(data.drawCount)};
    CORRADE_COMPARE(shader.flags(), data.flags);
    CORRADE_COMPARE(shader.materialCount(), data.materialCount);
    CORRADE_COMPARE(shader.drawCount(), data.drawCount);
    CORRADE_VERIFY(shader.id());
    {
        #if defined(CORRADE_TARGET_APPLE) && !defined(MAGNUM_TARGET_GLES)
        CORRADE_EXPECT_FAIL("macOS drivers need insane amount of state to validate properly.");
        #endif
        CORRADE_VERIFY(shader.validate().first);
    }

    MAGNUM_VERIFY_NO_GL_ERROR();
}

template<UnsignedInt dimensions> void LineGLTest::constructUniformBuffersAsync() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::uniform_buffer_object>())
        CORRADE_SKIP(GL::Extensions::ARB::uniform_buffer_object::string() << "is not supported.");
    #endif

    typename LineGL<dimensions>::CompileState state = LineGL<dimensions>::compile(typename LineGL<dimensions>::Configuration{}
        .setFlags(LineGL2D::Flag::UniformBuffers|LineGL2D::Flag::VertexColor)
        .setMaterialCount(16)
        .setDrawCount(48));
    CORRADE_COMPARE(state.flags(), LineGL2D::Flag::UniformBuffers|LineGL2D::Flag::VertexColor);
    CORRADE_COMPARE(state.materialCount(), 16);
    CORRADE_COMPARE(state.drawCount(), 48);

    while(!state.isLinkFinished())
        Utility::System::sleep(100);

    LineGL<dimensions> shader{std::move(state)};
    CORRADE_COMPARE(shader.flags(), LineGL2D::Flag::UniformBuffers|LineGL2D::Flag::VertexColor);
    CORRADE_COMPARE(shader.materialCount(), 16);
    CORRADE_COMPARE(shader.drawCount(), 48);
    CORRADE_VERIFY(shader.id());
    {
        #if defined(CORRADE_TARGET_APPLE) && !defined(MAGNUM_TARGET_GLES)
        CORRADE_EXPECT_FAIL("macOS drivers need insane amount of state to validate properly.");
        #endif
        CORRADE_VERIFY(shader.validate().first);
    }

    MAGNUM_VERIFY_NO_GL_ERROR();
}

#endif

template<UnsignedInt dimensions> void LineGLTest::constructMove() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    LineGL<dimensions> a{typename LineGL<dimensions>::Configuration{}
        .setFlags(LineGL<dimensions>::Flag::VertexColor)};
    const GLuint id = a.id();
    CORRADE_VERIFY(id);

    MAGNUM_VERIFY_NO_GL_ERROR();

    LineGL<dimensions> b{std::move(a)};
    CORRADE_COMPARE(b.id(), id);
    CORRADE_COMPARE(b.flags(), LineGL<dimensions>::Flag::VertexColor);
    CORRADE_VERIFY(!a.id());

    LineGL<dimensions> c{NoCreate};
    c = std::move(b);
    CORRADE_COMPARE(c.id(), id);
    CORRADE_COMPARE(c.flags(), LineGL<dimensions>::Flag::VertexColor);
    CORRADE_VERIFY(!b.id());
}

#ifndef MAGNUM_TARGET_GLES2
template<UnsignedInt dimensions> void LineGLTest::constructMoveUniformBuffers() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::uniform_buffer_object>())
        CORRADE_SKIP(GL::Extensions::ARB::uniform_buffer_object::string() << "is not supported.");
    #endif

    LineGL<dimensions> a{typename LineGL<dimensions>::Configuration{}
        .setFlags(LineGL<dimensions>::Flag::UniformBuffers)
        .setMaterialCount(2)
        .setDrawCount(5)};
    const GLuint id = a.id();
    CORRADE_VERIFY(id);

    MAGNUM_VERIFY_NO_GL_ERROR();

    LineGL<dimensions> b{std::move(a)};
    CORRADE_COMPARE(b.id(), id);
    CORRADE_COMPARE(b.flags(), LineGL<dimensions>::Flag::UniformBuffers);
    CORRADE_COMPARE(b.materialCount(), 2);
    CORRADE_COMPARE(b.drawCount(), 5);
    CORRADE_VERIFY(!a.id());

    LineGL<dimensions> c{NoCreate};
    c = std::move(b);
    CORRADE_COMPARE(c.id(), id);
    CORRADE_COMPARE(c.flags(), LineGL<dimensions>::Flag::UniformBuffers);
    CORRADE_COMPARE(c.materialCount(), 2);
    CORRADE_COMPARE(c.drawCount(), 5);
    CORRADE_VERIFY(!b.id());
}
#endif

// template<UnsignedInt dimensions> void LineGLTest::constructInvalid() {
//     auto&& data = ConstructInvalidData[testCaseInstanceId()];
//     setTestCaseTemplateName(Utility::format("{}", dimensions));
//     setTestCaseDescription(data.name);
//
//     CORRADE_SKIP_IF_NO_ASSERT();
//
//     std::ostringstream out;
//     Error redirectError{&out};
//     LineGL<dimensions>{data.flags};
//     CORRADE_COMPARE(out.str(), Utility::formatString(
//         "Shaders::LineGL: {}\n", data.message));
// }

#ifndef MAGNUM_TARGET_GLES2
template<UnsignedInt dimensions> void LineGLTest::constructUniformBuffersInvalid() {
    auto&& data = ConstructUniformBuffersInvalidData[testCaseInstanceId()];
    setTestCaseTemplateName(Utility::format("{}", dimensions));
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::uniform_buffer_object>())
        CORRADE_SKIP(GL::Extensions::ARB::uniform_buffer_object::string() << "is not supported.");
    #endif

    std::ostringstream out;
    Error redirectError{&out};
    LineGL<dimensions>{typename LineGL<dimensions>::Configuration{}
        .setFlags(data.flags)
        .setMaterialCount(data.materialCount)
        .setDrawCount(data.drawCount)};
    CORRADE_COMPARE(out.str(), Utility::formatString(
        "Shaders::LineGL: {}\n", data.message));
}
#endif

#ifndef MAGNUM_TARGET_GLES2
template<UnsignedInt dimensions> void LineGLTest::setUniformUniformBuffersEnabled() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    CORRADE_SKIP_IF_NO_ASSERT();

    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::uniform_buffer_object>())
        CORRADE_SKIP(GL::Extensions::ARB::uniform_buffer_object::string() << "is not supported.");
    #endif

    LineGL<dimensions> shader{typename LineGL<dimensions>::Configuration{}
        .setFlags(LineGL<dimensions>::Flag::UniformBuffers)};

    /* This should work fine */
    shader.setViewportSize({});

    std::ostringstream out;
    Error redirectError{&out};
    shader.setTransformationProjectionMatrix({})
        .setWidth({})
        .setSmoothness({})
        .setMiterLimit({})
        .setBackgroundColor({})
        .setColor({})
        .setObjectId({});
    CORRADE_COMPARE(out.str(),
        "Shaders::LineGL::setTransformationProjectionMatrix(): the shader was created with uniform buffers enabled\n"
        "Shaders::LineGL::setWidth(): the shader was created with uniform buffers enabled\n"
        "Shaders::LineGL::setSmoothness(): the shader was created with uniform buffers enabled\n"
        "Shaders::LineGL::setMiterLimit(): the shader was created with uniform buffers enabled\n"
        "Shaders::LineGL::setBackgroundColor(): the shader was created with uniform buffers enabled\n"
        "Shaders::LineGL::setColor(): the shader was created with uniform buffers enabled\n"
        "Shaders::LineGL::setObjectId(): the shader was created with uniform buffers enabled\n");
}

template<UnsignedInt dimensions> void LineGLTest::bindBufferUniformBuffersNotEnabled() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    CORRADE_SKIP_IF_NO_ASSERT();

    GL::Buffer buffer;
    LineGL<dimensions> shader;

    std::ostringstream out;
    Error redirectError{&out};
    shader.bindTransformationProjectionBuffer(buffer)
          .bindTransformationProjectionBuffer(buffer, 0, 16)
          .bindDrawBuffer(buffer)
          .bindDrawBuffer(buffer, 0, 16)
          .bindMaterialBuffer(buffer)
          .bindMaterialBuffer(buffer, 0, 16)
          .setDrawOffset(0);
    CORRADE_COMPARE(out.str(),
        "Shaders::LineGL::bindTransformationProjectionBuffer(): the shader was not created with uniform buffers enabled\n"
        "Shaders::LineGL::bindTransformationProjectionBuffer(): the shader was not created with uniform buffers enabled\n"
        "Shaders::LineGL::bindDrawBuffer(): the shader was not created with uniform buffers enabled\n"
        "Shaders::LineGL::bindDrawBuffer(): the shader was not created with uniform buffers enabled\n"
        "Shaders::LineGL::bindMaterialBuffer(): the shader was not created with uniform buffers enabled\n"
        "Shaders::LineGL::bindMaterialBuffer(): the shader was not created with uniform buffers enabled\n"
        "Shaders::LineGL::setDrawOffset(): the shader was not created with uniform buffers enabled\n");
}
#endif

template<UnsignedInt dimensions> void LineGLTest::setMiterLengthLimitInvalid() {
    auto&& data = SetMiterLengthLimitInvalidData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    CORRADE_SKIP_IF_NO_ASSERT();

    GL::Buffer buffer;
    LineGL<dimensions> shader{typename LineGL<dimensions>::Configuration{}
        .setJoinStyle(data.joinStyle)
    };

    std::ostringstream out;
    Error redirectError{&out};
    shader.setMiterLengthLimit(data.limit);
    CORRADE_COMPARE(out.str(), Utility::formatString(
        "Shaders::LineGL::setMiterLengthLimit(): {}\n", data.message));
}

template<UnsignedInt dimensions> void LineGLTest::setMiterAngleLimitInvalid() {
    auto&& data = SetMiterAngleLimitInvalidData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    CORRADE_SKIP_IF_NO_ASSERT();

    GL::Buffer buffer;
    LineGL<dimensions> shader{typename LineGL<dimensions>::Configuration{}
        .setJoinStyle(data.joinStyle)
    };

    std::ostringstream out;
    Error redirectError{&out};
    shader.setMiterAngleLimit(data.limit);
    CORRADE_COMPARE(out.str(), Utility::formatString(
        "Shaders::LineGL::setMiterAngleLimit(): {}\n", data.message));
}

#ifndef MAGNUM_TARGET_GLES2
template<UnsignedInt dimensions> void LineGLTest::setObjectIdNotEnabled() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    CORRADE_SKIP_IF_NO_ASSERT();

    LineGL<dimensions> shader;

    std::ostringstream out;
    Error redirectError{&out};
    shader.setObjectId(33376);
    CORRADE_COMPARE(out.str(),
        "Shaders::LineGL::setObjectId(): the shader was not created with object ID enabled\n");
}
#endif

#ifndef MAGNUM_TARGET_GLES2
template<UnsignedInt dimensions> void LineGLTest::setWrongDrawOffset() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    CORRADE_SKIP_IF_NO_ASSERT();

    #ifndef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::uniform_buffer_object>())
        CORRADE_SKIP(GL::Extensions::ARB::uniform_buffer_object::string() << "is not supported.");
    #endif

    LineGL<dimensions> shader{typename LineGL<dimensions>::Configuration{}
        .setFlags(LineGL<dimensions>::Flag::UniformBuffers)
        .setMaterialCount(2)
        .setDrawCount(5)};

    std::ostringstream out;
    Error redirectError{&out};
    shader.setDrawOffset(5);
    CORRADE_COMPARE(out.str(),
        "Shaders::LineGL::setDrawOffset(): draw offset 5 is out of bounds for 5 draws\n");
}
#endif

constexpr Vector2i RenderSize{128, 128};

void LineGLTest::renderSetup() {
    /* Pick a color that's directly representable on RGBA4 as well to reduce
       artifacts */
    GL::Renderer::setClearColor(0x111111_rgbf);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    _color = GL::Renderbuffer{};
    _color.setStorage(
        #if !defined(MAGNUM_TARGET_GLES2) || !defined(MAGNUM_TARGET_WEBGL)
        GL::RenderbufferFormat::RGBA8,
        #else
        GL::RenderbufferFormat::RGBA4,
        #endif
        RenderSize);
    _framebuffer = GL::Framebuffer{{{}, RenderSize}};
    _framebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _color)
        .clear(GL::FramebufferClear::Color)
        .bind();
}

void LineGLTest::renderTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Renderbuffer{NoCreate};
}

/* A barebones utility for generating a line mesh. Embedded directly in the
   test (as opposed to using something from MeshTools) to have it easier to
   modify, debug and iterate on. */
template<UnsignedInt dimensions> GL::Mesh generateLineMesh(Containers::StridedArrayView1D<const VectorTypeFor<dimensions, Float>> lineSegments) {
    struct Vertex {
        VectorTypeFor<dimensions, Float> previousPosition;
        VectorTypeFor<dimensions + 1, Float> position;
        VectorTypeFor<dimensions, Float> nextPosition;
    };

    enum: Int {
        LineUp = 1,
        LineBegin = 2,
        LineCap = 4
    };

    CORRADE_INTERNAL_ASSERT(lineSegments.size() % 2 == 0);
    /* Not NoInit, because we're subsequently checking for NaNs */
    Containers::Array<Vertex> vertices{ValueInit, lineSegments.size()*2};
    for(std::size_t i = 0; i != lineSegments.size(); ++i) {
        vertices[i*2 + 0].position = {lineSegments[i], Float(LineUp|(i % 2 ? 0 : LineBegin))};
        vertices[i*2 + 1].position = {lineSegments[i], Float(i % 2 ? 0 : LineBegin)};
    }

    /* Mark caps if it's the beginning, the end or the segments are disjoint */
    for(std::size_t i: {std::size_t{0}, std::size_t{1}, vertices.size() - 2, vertices.size() - 1}) {
        vertices[i].position[dimensions] = Int(vertices[i].position[dimensions])|LineCap;
    }
    for(std::size_t i = 4; i < vertices.size(); i += 4) {
        /* Compare everything except the last component, which is always
           different */
        if(VectorTypeFor<dimensions, Float>::pad(vertices[i - 2].position) ==
           VectorTypeFor<dimensions, Float>::pad(vertices[i].position))
            continue;
        for(std::size_t j: {i - 2, i - 1, i + 0, i + 1}) {
            vertices[j].position[dimensions] = Int(vertices[j].position[dimensions])|LineCap;
        }
    }

    /* Prev positions for segment last vertices -- the other segment point */
    for(std::size_t i = 2; i < Containers::arraySize(vertices); i += 4) {
        vertices[i + 0].previousPosition = vertices[i + 1].previousPosition =
            VectorTypeFor<dimensions, Float>::pad(vertices[i - 2].position);
    }
    /* Prev positions for segment first vertices -- a neighbor segment, if any */
    for(std::size_t i = 4; i < Containers::arraySize(vertices); i += 4) {
        if(Int(vertices[i].position[dimensions]) & LineCap)
            continue;
        vertices[i + 0].previousPosition = vertices[i + 1].previousPosition =
            VectorTypeFor<dimensions, Float>::pad(vertices[i - 4].position);
    }
    /* Next positions for segment first vertices -- the other segment point */
    for(std::size_t i = 0; i < Containers::arraySize(vertices) - 2; i += 4) {
        vertices[i + 0].nextPosition = vertices[i + 1].nextPosition =
            VectorTypeFor<dimensions, Float>::pad(vertices[i + 2].position);
    }
    /* Next positions for last vertices -- a neighbor segment, if any */
    for(std::size_t i = 2; i < Containers::arraySize(vertices) - 4; i += 4) {
        if(Int(vertices[i].position[dimensions]) & LineCap)
            continue;
        vertices[i + 0].nextPosition = vertices[i + 1].nextPosition =
            VectorTypeFor<dimensions, Float>::pad(vertices[i + 4].position);
    }

    Containers::Array<UnsignedInt> indices;
    for(UnsignedInt i = 0; i != lineSegments.size()/2; ++i) {
        arrayAppend(indices, {
            i*4 + 0,
            i*4 + 1,
            i*4 + 2,
            i*4 + 2,
            i*4 + 1,
            i*4 + 3
        });

        // TODO enable FaceCulling
        /* Add also indices for the bevel in both orientations (one will always
           degenerate) */
        if(!(Int(Math::abs(vertices[i*4 + 3].position[dimensions])) & LineCap)) {
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

    GL::Mesh mesh;
    mesh.addVertexBuffer(GL::Buffer{vertices}, 0,
            LineGL2D::PreviousPosition{},
            LineGL2D::Position{},
            LineGL2D::NextPosition{})
        .setIndexBuffer(GL::Buffer{indices}, 0, GL::MeshIndexType::UnsignedInt)
        .setCount(indices.size());

    return mesh;
}

GL::Mesh generateLineMesh(std::initializer_list<Vector2> lineSegments) {
    return generateLineMesh<2>(Containers::arrayView(lineSegments));
}

// GL::Mesh generateLineMesh(std::initializer_list<Vector3> lineSegments) {
//     return generateLineMesh<3>(Containers::arrayView(lineSegments));
// }

template<LineGL2D::Flag flag> void LineGLTest::renderDefaults2D() {
    #ifndef MAGNUM_TARGET_GLES2
    if(flag == LineGL2D::Flag::UniformBuffers) {
        setTestCaseTemplateName("Flag::UniformBuffers");

        #ifndef MAGNUM_TARGET_GLES
        if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::uniform_buffer_object>())
            CORRADE_SKIP(GL::Extensions::ARB::uniform_buffer_object::string() << "is not supported.");
        #endif
    }
    #endif

    GL::Mesh lines = generateLineMesh({
        /* A / line from the top to bottom */
        {-0.0f, 0.5f}, {-0.5f, -0.5f},
        /* A / line from the bottom to top */
        {-0.5f, -0.5f}, {0.5f, -0.25f},
        /* A | line from the bottom to top */
        {-0.75f, -0.25f}, {-0.75f, 0.75f},
        /* A _ line from the left to right */
        {-0.25f, -0.75f}, {0.75f, -0.75f},
        /* A zero-size line that should be visible as a point */
        {0.5f, 0.5f}, {0.5f, 0.5f}
    });

    LineGL2D shader{LineGL2D::Configuration{}
        .setFlags(flag)};
    shader.setViewportSize(Vector2{RenderSize});

    /* Enabling blending and a half-transparent color -- there should be no
       overlaps */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::One,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    if(flag == LineGL2D::Flag{}) {
        shader.draw(lines);
    }
    #ifndef MAGNUM_TARGET_GLES2
    else if(flag == LineGL2D::Flag::UniformBuffers) {
        GL::Buffer transformationProjectionUniform{GL::Buffer::TargetHint::Uniform, {
            TransformationProjectionUniform2D{}
        }};
        GL::Buffer drawUniform{GL::Buffer::TargetHint::Uniform, {
            LineDrawUniform{}
        }};
        GL::Buffer materialUniform{GL::Buffer::TargetHint::Uniform, {
            LineMaterialUniform{}
        }};
        shader
            .bindTransformationProjectionBuffer(transformationProjectionUniform)
            .bindDrawBuffer(drawUniform)
            .bindMaterialBuffer(materialUniform)
            .draw(lines);
    }
    #endif
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImporter plugins not found.");

    CORRADE_COMPARE_WITH(
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(_framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm}).pixels<Color4ub>()),
        Utility::Path::join(SHADERS_TEST_DIR, "LineTestFiles/defaults2D.tga"),
        (DebugTools::CompareImageToFile{_manager}));
}

const Vector2 RenderLineCapsJoins2DLineData[]{
    /* A single point (a zero-length line) */
    {0.2f, 0.8f}, {0.2f, 0.8f},
    /* A rotated point (i.e., a line of a very small length). Should ideally be
       symmetric. */
    /** @todo it isn't, why? */
    {-0.4f, 0.15f}, {-0.4f + Math::TypeTraits<Float>::epsilon(), 0.15f + Math::TypeTraits<Float>::epsilon()},
    /* A 90° join with a large length ratio. Caps should look the same on both
       ends, independently on the length */
    {-0.8f, 0.7f}, {-0.8f, -0.25f},
    {-0.8f, -0.25f}, {-0.6f, -0.25f},
    /* A four-segment line with a 60°, 120° and a 30° join. All should be
       miters in the default setup. */
    Vector2{0.25f, 0.3f} + Complex::rotation(-60.0_degf).transformVector(Vector2::yAxis(0.6f)) + Vector2::yAxis(-0.3f),
        Vector2{0.25f, 0.3f} + Complex::rotation(-60.0_degf).transformVector(Vector2::yAxis(0.6f)),
    Vector2{0.25f, 0.3f} + Complex::rotation(-60.0_degf).transformVector(Vector2::yAxis(0.6f)),
        {0.2f, 0.35f},
    {0.2f, 0.35f},
        Vector2{0.25f, 0.3f} + Complex::rotation(60.0_degf).transformVector(Vector2::yAxis(0.6f)),
    Vector2{0.25f, 0.3f} + Complex::rotation(60.0_degf).transformVector(Vector2::yAxis(0.6f)),
        Vector2{0.25f, 0.3f} + Complex::rotation(60.0_degf).transformVector(Vector2::yAxis(0.6f)) +
        Complex::rotation(30.0_degf).transformVector(Vector2::yAxis(-0.5f)),
    /* A completely ordinary line segment, to test the case when everything
       goes wrong */
    {0.4f, -0.05f}, {0.8f, -0.05f},
    /* A 180° join, with one part shorter. Should be always beveled, should not
       overlap and should not disappear. */
    /** @todo the other end disappears, fix */
    {0.8f, -0.4f}, {0.0f, -0.4f},
    {0.0f, -0.4f}, {0.8f, -0.40001f}, /** @todo otherwise disappears, fix */
    /* A join where the other line touches the edge (either slightly above for
       square/round caps, or slightly below for butt caps). The caps should not
       get distorted in any way. */
    /** @todo they do, fix */
    {-0.45f, -0.8f}, {-0.7f, -0.8f},
    {-0.7f, -0.8f},
        Vector2{-0.7f, -0.8f} + Complex::rotation(60.0_degf).transformVector(Vector2::xAxis(0.2f)),
    /* A join where the other line endpoint is inside the line. The caps should
       not get distorted and it shouldn't overlap. */
    /** @todo it's distorted heavily, fix */
    {0.25f, -0.8f}, {0.0f, -0.8f},
    {0.0f, -0.8f},
        Vector2{0.0f, -0.8f} + Complex::rotation(60.0_degf).transformVector(Vector2::xAxis(0.01f)),
    /* Like above, but with the first line short as well */
    /** @todo here's both a heavy distortion and an overlap, fix */
    {0.725f, -0.8f}, {0.7f, -0.8f},
    {0.7f, -0.8f},
        Vector2{0.7f, -0.8f} + Complex::rotation(60.0_degf).transformVector(Vector2::xAxis(0.01f))
};

template<LineGL2D::Flag flag> void LineGLTest::renderLineCapsJoins2D() {
    auto&& data = RenderLineCapsJoins2DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    #ifndef MAGNUM_TARGET_GLES2
    if(flag == LineGL2D::Flag::UniformBuffers) {
        setTestCaseTemplateName("Flag::UniformBuffers");

        #ifndef MAGNUM_TARGET_GLES
        if(!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::uniform_buffer_object>())
            CORRADE_SKIP(GL::Extensions::ARB::uniform_buffer_object::string() << "is not supported.");
        #endif
    }
    #endif

    GL::Mesh lines = generateLineMesh<2>(RenderLineCapsJoins2DLineData);

    LineGL2D::Configuration configuration;
    configuration.setFlags(flag);
    if(data.capStyle) configuration.setCapStyle(*data.capStyle);
    if(data.joinStyle) configuration.setJoinStyle(*data.joinStyle);
    LineGL2D shader{configuration};
    shader
        .setViewportSize(Vector2{RenderSize})
        .setWidth(data.width)
        .setSmoothness(data.smoothness)
        .setColor(0x80808080_rgbaf);
    if(data.miterLengthLimit) shader.setMiterLengthLimit(*data.miterLengthLimit);
    if(data.miterAngleLimit) shader.setMiterAngleLimit(*data.miterAngleLimit);

    /* Enabling blending and a half-transparent color -- there should be no
       overlaps */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::One,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    if(flag == LineGL2D::Flag{}) {
        shader.draw(lines);
    }
    #ifndef MAGNUM_TARGET_GLES2
    else if(flag == LineGL2D::Flag::UniformBuffers) {
        GL::Buffer transformationProjectionUniform{GL::Buffer::TargetHint::Uniform, {
            TransformationProjectionUniform2D{}
        }};
        GL::Buffer drawUniform{GL::Buffer::TargetHint::Uniform, {
            LineDrawUniform{}
        }};
        GL::Buffer materialUniform{GL::Buffer::TargetHint::Uniform, {
            LineMaterialUniform{}
        }};
        shader
            .bindTransformationProjectionBuffer(transformationProjectionUniform)
            .bindDrawBuffer(drawUniform)
            .bindMaterialBuffer(materialUniform)
            .draw(lines);
    }
    #endif
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImporter plugins not found.");

    Image2D image = _framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm});

    CORRADE_COMPARE_WITH(
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(image.pixels<Color4ub>()),
        Utility::Path::join({SHADERS_TEST_DIR, "LineTestFiles", data.expected}),
        (DebugTools::CompareImageToFile{_manager}));

    {
        /** @todo drop this when fixed */
        CORRADE_EXPECT_FAIL("Rendered with overlapping geometry at the moment.");
        CORRADE_COMPARE(Math::max(image.pixels<Color4ub>().asContiguous()), 0x888888ff_rgba);
    }
}

void LineGLTest::renderLineCapsJoins2DReversed() {
    auto&& data = RenderLineCapsJoins2DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* As this verifies mainly the algorithm, there's no variant with UBOs --
       those are sufficiently tested elsewhere */

    GL::Mesh lines = generateLineMesh<2>(Containers::stridedArrayView(RenderLineCapsJoins2DLineData).flipped<0>());

    /* Enabling blending and a half-transparent color -- there should be no
       overlaps */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::One,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    LineGL2D::Configuration configuration;
    if(data.capStyle) configuration.setCapStyle(*data.capStyle);
    if(data.joinStyle) configuration.setJoinStyle(*data.joinStyle);
    LineGL2D shader{configuration};
    shader
        .setViewportSize(Vector2{RenderSize})
        .setWidth(data.width)
        .setSmoothness(data.smoothness)
        .setColor(0x80808080_rgbaf);
    if(data.miterLengthLimit) shader.setMiterLengthLimit(*data.miterLengthLimit);
    if(data.miterAngleLimit) shader.setMiterAngleLimit(*data.miterAngleLimit);

    /* Enabling blending and a half-transparent color -- there should be no
       overlaps */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::One,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    shader.draw(lines);

    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImporter plugins not found.");

    Image2D image = _framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm});

    CORRADE_COMPARE_WITH(
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(image.pixels<Color4ub>()),
        Utility::Path::join({SHADERS_TEST_DIR, "LineTestFiles", data.expected}),
        /** @todo sync this with render2D() once the overlaps are fixed */
        (DebugTools::CompareImageToFile{_manager, 1.0f, 0.0005f}));
}

void LineGLTest::renderLineCapsJoins2DTransformed() {
    auto&& data = RenderLineCapsJoins2DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* As this verifies mainly the algorithm, there's no variant with UBOs --
       those are sufficiently tested elsewhere */

    const Matrix3 transformation = Matrix3::scaling({100.0f, 2.0f})*Matrix3::rotation(45.0_degf);

    Containers::Array<Vector2> transformedLineSegments{NoInit, Containers::arraySize(RenderLineCapsJoins2DLineData)};
    for(std::size_t i = 0; i != transformedLineSegments.size(); ++i)
        transformedLineSegments[i] = transformation.transformPoint(RenderLineCapsJoins2DLineData[i]);

    GL::Mesh lines = generateLineMesh<2>(transformedLineSegments);

    LineGL2D::Configuration configuration;
    if(data.capStyle) configuration.setCapStyle(*data.capStyle);
    if(data.joinStyle) configuration.setJoinStyle(*data.joinStyle);
    LineGL2D shader{configuration};
    shader
        .setViewportSize(Vector2{RenderSize})
        .setWidth(data.width)
        .setSmoothness(data.smoothness)
        .setTransformationProjectionMatrix(transformation.inverted())
        .setColor(0x80808080_rgbaf);

    if(data.miterLengthLimit) shader.setMiterLengthLimit(*data.miterLengthLimit);
    if(data.miterAngleLimit) shader.setMiterAngleLimit(*data.miterAngleLimit);

    /* Enabling blending and a half-transparent color -- there should be no
       overlaps */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::One,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    shader.draw(lines);

    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImporter plugins not found.");

    Image2D image = _framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm});

    CORRADE_COMPARE_WITH(
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(image.pixels<Color4ub>()),
        Utility::Path::join({SHADERS_TEST_DIR, "LineTestFiles", data.expected}),
        (DebugTools::CompareImageToFile{_manager}));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Shaders::Test::LineGLTest)
