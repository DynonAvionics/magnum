/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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
#include <Corrade/Containers/StringView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h>

#include "Magnum/Text/Feature.h"

namespace Magnum { namespace Text { namespace Test { namespace {

struct FeatureTest: TestSuite::Tester {
    explicit FeatureTest();

    void debug();

    void fromFourCC();
    void fromString();
    void fromStringInvalid();
};

FeatureTest::FeatureTest() {
    addTests({&FeatureTest::debug,

              &FeatureTest::fromFourCC,
              &FeatureTest::fromString,
              &FeatureTest::fromStringInvalid});
}

void FeatureTest::debug() {
    std::ostringstream out;
    Debug{&out} << Feature::StandardLigatures << Text::feature('m', 'a', '\xab', 'g');
    CORRADE_COMPARE(out.str(), "Text::Feature::StandardLigatures Text::Feature('m', 'a', 0xab, 'g')\n");
}

void FeatureTest::fromFourCC() {
    Feature s = Text::feature('z', 'e', 'r', 'o');
    CORRADE_COMPARE(s, Feature::SlashedZero);

    constexpr Feature cs = Text::feature('z', 'e', 'r', 'o');
    CORRADE_COMPARE(cs, Feature::SlashedZero);
}

void FeatureTest::fromString() {
    Feature s = Text::feature("zero");
    CORRADE_COMPARE(s, Feature::SlashedZero);
}

void FeatureTest::fromStringInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    std::ostringstream out;
    Error redirectError{&out};
    Text::feature("");
    Text::feature("hahah");
    /* Non-ASCII values are allowed, as the constexpr feature() allows them
       too */
    CORRADE_COMPARE(out.str(),
        "Text::feature(): expected a four-character code, got \n"
        "Text::feature(): expected a four-character code, got hahah\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Text::Test::FeatureTest)
