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
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/Format.h>

#include "Magnum/Shaders/LineGL.h"

namespace Magnum { namespace Shaders { namespace Test { namespace {

/* There's an underscore between GL and Test to disambiguate from GLTest, which
   is a common suffix used to mark tests that need a GL context. Ugly, I know. */
struct LineGL_Test: TestSuite::Tester {
    explicit LineGL_Test();

    template<UnsignedInt dimensions> void constructConfigurationDefault();
    template<UnsignedInt dimensions> void constructConfigurationSetters();

    template<UnsignedInt dimensions> void constructNoCreate();
    template<UnsignedInt dimensions> void constructCopy();

    void debugFlag();
    void debugFlags();
    void debugFlagsSupersets();

    void debugCapStyle();
    void debugJoinStyle();
};

LineGL_Test::LineGL_Test() {
    addTests({&LineGL_Test::constructConfigurationDefault<2>,
              &LineGL_Test::constructConfigurationDefault<3>,
              &LineGL_Test::constructConfigurationSetters<2>,
              &LineGL_Test::constructConfigurationSetters<3>,

              &LineGL_Test::constructNoCreate<2>,
              &LineGL_Test::constructNoCreate<3>,
              &LineGL_Test::constructCopy<2>,
              &LineGL_Test::constructCopy<3>,

              &LineGL_Test::debugFlag,
              &LineGL_Test::debugFlags,
              &LineGL_Test::debugFlagsSupersets,

              &LineGL_Test::debugCapStyle,
              &LineGL_Test::debugJoinStyle});
}

template<UnsignedInt dimensions> void LineGL_Test::constructConfigurationDefault() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    typename LineGL<dimensions>::Configuration configuration;
    CORRADE_COMPARE(configuration.flags(), typename LineGL<dimensions>::Flags{});
    #ifndef MAGNUM_TARGET_GLES2
    CORRADE_COMPARE(configuration.materialCount(), 1);
    CORRADE_COMPARE(configuration.drawCount(), 1);
    #endif
}

template<UnsignedInt dimensions> void LineGL_Test::constructConfigurationSetters() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    typename LineGL<dimensions>::Configuration configuration = typename LineGL<dimensions>::Configuration{}
        .setFlags(LineGL<dimensions>::Flag::VertexColor)
        #ifndef MAGNUM_TARGET_GLES2
        .setMaterialCount(17)
        .setDrawCount(266)
        #endif
        ;
    CORRADE_COMPARE(configuration.flags(), LineGL<dimensions>::Flag::VertexColor);
    #ifndef MAGNUM_TARGET_GLES2
    CORRADE_COMPARE(configuration.materialCount(), 17);
    CORRADE_COMPARE(configuration.drawCount(), 266);
    #endif
}

template<UnsignedInt dimensions> void LineGL_Test::constructNoCreate() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    {
        LineGL<dimensions> shader{NoCreate};
        CORRADE_COMPARE(shader.id(), 0);
        CORRADE_COMPARE(shader.flags(), typename LineGL<dimensions>::Flags{});
    }

    CORRADE_VERIFY(true);
}

template<UnsignedInt dimensions> void LineGL_Test::constructCopy() {
    setTestCaseTemplateName(Utility::format("{}", dimensions));

    CORRADE_VERIFY(!std::is_copy_constructible<LineGL<dimensions>>{});
    CORRADE_VERIFY(!std::is_copy_assignable<LineGL<dimensions>>{});
}

void LineGL_Test::debugFlag() {
    std::ostringstream out;
    Debug{&out} << LineGL3D::Flag::VertexColor << LineGL3D::Flag(0xf00d);
    CORRADE_COMPARE(out.str(), "Shaders::LineGL::Flag::VertexColor Shaders::LineGL::Flag(0xf00d)\n");
}

void LineGL_Test::debugFlags() {
    std::ostringstream out;
    Debug{&out} << (LineGL3D::Flag::VertexColor|LineGL3D::Flag::InstancedTransformation) << LineGL3D::Flags{};
    CORRADE_COMPARE(out.str(), "Shaders::LineGL::Flag::VertexColor|Shaders::LineGL::Flag::InstancedTransformation Shaders::LineGL::Flags{}\n");
}

void LineGL_Test::debugFlagsSupersets() {
    #ifndef MAGNUM_TARGET_GLES2
    /* InstancedObjectId is a superset of ObjectId so only one should be
       printed */
    {
        std::ostringstream out;
        Debug{&out} << (LineGL3D::Flag::ObjectId|LineGL3D::Flag::InstancedObjectId);
        CORRADE_COMPARE(out.str(), "Shaders::LineGL::Flag::InstancedObjectId\n");
    }
    #endif

    #ifndef MAGNUM_TARGET_GLES2
    /* MultiDraw is a superset of UniformBuffers so only one should be printed */
    {
        std::ostringstream out;
        Debug{&out} << (LineGL3D::Flag::MultiDraw|LineGL3D::Flag::UniformBuffers);
        CORRADE_COMPARE(out.str(), "Shaders::LineGL::Flag::MultiDraw\n");
    }
    #endif
}

void LineGL_Test::debugCapStyle() {
    std::ostringstream out;
    Debug{&out} << LineGL3D::CapStyle::Square << LineGL3D::CapStyle(0xb0);
    CORRADE_COMPARE(out.str(), "Shaders::LineGL::CapStyle::Square Shaders::LineGL::CapStyle(0xb0)\n");
}

void LineGL_Test::debugJoinStyle() {
    std::ostringstream out;
    Debug{&out} << LineGL3D::JoinStyle::Round << LineGL3D::JoinStyle(0xb0);
    CORRADE_COMPARE(out.str(), "Shaders::LineGL::JoinStyle::Round Shaders::LineGL::JoinStyle(0xb0)\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Shaders::Test::LineGL_Test)
