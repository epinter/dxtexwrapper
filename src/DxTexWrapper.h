#pragma once

/*
    MIT License

    Copyright (c) 2022 Emerson Pinter

     https://github.com/epinter/dxtexwrapper

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 */

#include <DirectXTex.h>
#include "Content.h"
#include "ImageData.h"

#ifdef WIN32
#define LIBEXPORT __declspec(dllexport)
#else
#define LIBEXPORT __attribute__((visibility("default")))
#endif

std::vector<unsigned char> convertRGBAtoBGRA(std::vector<unsigned char> const &rgba);
ImageData encodePng([[maybe_unused]] ImageData const &ddsData);
ImageData decodePng(std::vector<unsigned char> const &png);
extern "C" {
[[maybe_unused]] LIBEXPORT Content convertDDStoPNG(unsigned int size, void * data);
[[maybe_unused]] LIBEXPORT Content convertPNGtoDDS(unsigned int size, void * data, bool dx10ext, bool bgra);
}

#ifdef WIN32

void printErrorDescription(HRESULT hr);

#else

void printErrorDescription(long hr);

#endif

