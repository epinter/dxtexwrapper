//
// Created by epinter on 18/04/2022.
//

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

#include <cstdio>
#include <cstring>
#include <DirectXTex.h>
#include "DxTexWrapper.h"
#include "ImageData.h"

#ifdef DXTWRAPPER_USE_LIBSPNG
#pragma message("building with SPNG")

#include <spng.h>

#else
#ifdef DXTWRAPPER_USE_LIBLODEPNG
#pragma message("building with LODEPNG")
#include <lodepng.h>
#endif
#endif

/**
 * Convert PNG image to a DDS texture
 *
 * @param pngSize The size(in bytes) of data parameter
 * @param pngData The PNG image
 * @param dx10ext Flag to toggle generation of DX10 header. True to generate DX10 header extension. False will force DirectX9
 *            legacy (DDS_FLAGS_FORCE_DX9_LEGACY), this will return a failure if it can't encode the metadata using legacy DX9 headers.
 * @param bgra When true, the output will be DXGI_FORMAT_B8G8R8A8_UNORM. When false, the output will be DXGI_FORMAT_R8G8B8A8_UNORM.
 * @return Returns the 'Data' containing size and a pointer to DDS texture
 */
[[maybe_unused]] Content convertPNGtoDDS(unsigned int const pngSize, void *const pngData, bool const dx10ext, bool const bgra) {
    std::vector<unsigned char> data(pngSize);
    std::memcpy(data.data(), pngData, pngSize);
    Content ret{};

    ImageData imageData = decodePng(data);

    DirectX::Blob output{};
    DirectX::Image rawImage{};
    DirectX::ScratchImage image{};
    DirectX::ScratchImage mipChain{};

    //create struct with raw image data
    rawImage.width = imageData.getWidth();
    rawImage.height = imageData.getHeight();

    if (bgra) {
        rawImage.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        //convert byte order, PNG is big endian and uses RGBA
        std::vector<unsigned char> bgraBuffer = convertRGBAtoBGRA(imageData.getPixels());

        rawImage.pixels = static_cast<uint8_t *>(std::malloc(bgraBuffer.size()));
        std::memcpy(rawImage.pixels, bgraBuffer.data(), bgraBuffer.size());
        bgraBuffer.clear();
    } else {
        rawImage.format = DXGI_FORMAT_R8G8B8A8_UNORM;

        //PNG is already in the correct byte order for R8G8B8A8_UNORM
        rawImage.pixels = static_cast<uint8_t *>(std::malloc(imageData.getPixelsSize()));
        std::memcpy(rawImage.pixels, imageData.getPixelsPointer(), imageData.getPixelsSize());
    }

    //calculate pitch, needed to keep image in the correct position and aspect
    DirectX::ComputePitch(rawImage.format, rawImage.width, rawImage.height, rawImage.rowPitch, rawImage.slicePitch,
                          DirectX::CP_FLAGS_NONE);

    //generate all mipmaps in the ScratchImage, unlimited levels
    HRESULT hrMipmap = DirectX::GenerateMipMaps(rawImage, DirectX::TEX_FILTER_FORCE_NON_WIC, 0, mipChain);
    if (FAILED(hrMipmap)) {
        std::fprintf(stderr, "Error generating mipmaps: ");
        printErrorDescription(hrMipmap);
        return ret;
    }
    std::free(rawImage.pixels);

    //generate a DDS from the mipchain
    HRESULT hr = DirectX::SaveToDDSMemory(mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(),
                                          dx10ext ? DirectX::DDS_FLAGS_FORCE_DX10_EXT : DirectX::DDS_FLAGS_FORCE_DX9_LEGACY
                                                                                        | DirectX::DDS_FLAGS_FORCE_RGB, output);
    if (FAILED(hr)) {
        std::fprintf(stderr, "Error writing DDS to buffer: ");
        printErrorDescription(hr);
        return ret;
    }

    ret.size = static_cast<unsigned int>(output.GetBufferSize());
    ret.content = std::malloc(ret.size);
    std::memcpy(ret.content, output.GetBufferPointer(), ret.size);
    output.Release();
    return ret;
}

/**
 * Convert a DDS texture to PNG image
 * @param size The size(in bytes) of data parameter
 * @param data The DDS texture
 * @return Returns the 'Data' containing size and a pointer to PNG
 */
[[maybe_unused]] Content convertDDStoPNG(unsigned int const size, void *const data) {
    Content ret{};
    ret.size = 0;

    DirectX::ScratchImage image{};
    DirectX::TexMetadata metadata{};
    try {
        HRESULT hr = DirectX::LoadFromDDSMemory(data, size, DirectX::DDS_FLAGS_FORCE_RGB, &metadata, image);
        if (FAILED(hr)) {
            std::fprintf(stderr, "Error reading DDS from buffer: ");
            printErrorDescription(hr);
            return ret;
        }
    } catch (...) {
        std::fprintf(stderr, "Error reading DDS from buffer");
    }

    try {
        if (DirectX::IsCompressed(image.GetMetadata().format)) {
            DirectX::ScratchImage uncompressed;
            HRESULT resultDec = DirectX::Decompress(image.GetImages(), image.GetImageCount(),
                                                    image.GetMetadata(),
                                                    DXGI_FORMAT_UNKNOWN, uncompressed);
            if (FAILED(resultDec)) {
                std::fprintf(stderr, "Error decompressing texture: ");
                printErrorDescription(resultDec);
                return ret;
            }
            std::swap(uncompressed, image);
            uncompressed.Release();
        }
    } catch (...) {
        std::fprintf(stderr, "Error uncompressing texture\n");
    }

    ImageData ddsData;
    ddsData.setWidth(static_cast<unsigned int>(image.GetMetadata().width));
    ddsData.setHeight(static_cast<unsigned int>(image.GetMetadata().height));
    ddsData.setPixels(image.GetImage(0, 0, 0)->pixels, image.GetImage(0, 0, 0)->slicePitch);
    try {
        ImageData imageData = encodePng(ddsData);

        image.Release();
        ret.size = static_cast<unsigned int>(imageData.getPixelsSize());
        ret.content = std::malloc(ret.size);
        std::memcpy(ret.content, imageData.getPixelsPointer(), ret.size);
    } catch (...) {
        std::fprintf(stderr, "Error writing to WIC buffer");
    }
    return ret;
}

std::vector<unsigned char> convertRGBAtoBGRA(std::vector<unsigned char> const &rgba) {
    std::vector<unsigned char> bgra;

    for (auto it = rgba.begin(); it != rgba.end(); it += 4) {
        unsigned char r = *(it);
        unsigned char g = *(it + 1);
        unsigned char b = *(it + 2);
        unsigned char a = *(it + 3);
        bgra.emplace_back(b);
        bgra.emplace_back(g);
        bgra.emplace_back(r);
        bgra.emplace_back(a);
    }

    return bgra;
}

#ifdef DXTWRAPPER_USE_LIBSPNG

ImageData encodePng(ImageData const &ddsData) {
    ImageData imageData;

    spng_ihdr ihdr{}; /* zero-initialize to set valid defaults */

    /* Creating an encoder context requires a flag */
    spng_ctx *ctx = spng_ctx_new(SPNG_CTX_ENCODER);

    /* Encode to internal buffer managed by the library */
    spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER, 1);

    /* Set image properties, this determines the destination image format */
    ihdr.width = ddsData.getWidth();
    ihdr.height = ddsData.getHeight();
    ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
    ihdr.bit_depth = 8;

    spng_set_ihdr(ctx, &ihdr);

    /* SPNG_ENCODE_FINALIZE will finalize the PNG with the end-of-file marker */
    int error = spng_encode_image(ctx, ddsData.getPixelsPointer(), ddsData.getPixelsSize(), SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);

    if (error) {
        std::fprintf(stderr, "spng_encode_image() error: %s\n", spng_strerror(error));
        spng_ctx_free(ctx);
        return imageData;
    }

    size_t pngSize;
    void *pngBuf;

    int encodeError = 0;
    /* Get the internal buffer of the finished PNG */
    pngBuf = spng_get_png_buffer(ctx, &pngSize, &encodeError);

    if (encodeError || pngBuf == nullptr) {
        std::fprintf(stderr, "spng_get_png_buffer() error: %s\n", spng_strerror(encodeError));
        spng_ctx_free(ctx);
        return imageData;
    }

    imageData.setPixels(pngBuf, pngSize);
    spng_ctx_free(ctx);
    free(pngBuf);

    return imageData;
}

ImageData decodePng(std::vector<unsigned char> const &png) {
    ImageData imageData;

    unsigned long long outputBufferSize;
    /* Create a context */
    spng_ctx *ctx = spng_ctx_new(0);

    /* Set an input buffer */
    spng_set_png_buffer(ctx, png.data(), png.size());

    /* Determine output image size */
    spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &outputBufferSize);

    std::vector<unsigned char> outputBuffer(outputBufferSize);

    /* Decode to 8-bit RGBA */
    int error = spng_decode_image(ctx, outputBuffer.data(), outputBuffer.size(), SPNG_FMT_RGBA8, 0);
    if (error) {
        printf("progressive spng_decode_image() error: %s\n", spng_strerror(error));
        return imageData;
    }

    spng_ihdr ihdr{};
    spng_get_ihdr(ctx, &ihdr);
    imageData.setWidth(ihdr.width);
    imageData.setHeight(ihdr.height);

    /* Free context memory */
    spng_ctx_free(ctx);

    imageData.setPixels(outputBuffer);

    return imageData;
}

#else
#ifdef DXTWRAPPER_USE_LIBLODEPNG

ImageData encodePng(ImageData const &ddsData) {
    ImageData imageData{};
    lodepng::State state;

    if (lodepng::encode(imageData.getPixels(), ddsData.getPixelsPointer(), ddsData.getWidth(), ddsData.getHeight())) {
        std::fprintf(stderr, "Error converting to PNG");
    }

    return imageData;
}

ImageData decodePng(std::vector<unsigned char> const &png) {
    ImageData imageData{};
    unsigned int width;
    unsigned int height;

    lodepng::State state;

    if (lodepng::decode(imageData.getPixels(), width, height, state, png.data(), png.size())) {
        std::fprintf(stderr, "Error decoding PNG\n");
    }

    imageData.setWidth(width);
    imageData.setHeight(height);

    return imageData;
}

#endif
#endif

#ifdef WIN32

/**
 * Print the WIN32 error detail generated in the HRESULT
 *
 * @param hr Error
 */
void printErrorDescription(HRESULT hr) {
    if (FACILITY_WINDOWS == HRESULT_FACILITY(hr))
        hr = HRESULT_CODE(hr);
    TCHAR *szErrMsg;

    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&szErrMsg), 0, nullptr) != 0) {
        std::fprintf(stderr, TEXT("%s"), szErrMsg);
        LocalFree(szErrMsg);
    } else
        std::fprintf(stderr, TEXT("[Could not find a description for error # %#lx.]\n"), hr);
}

#else
void printErrorDescription(long hr) {
    std::fprintf(stderr, ("Error code # %#lx.\n"), hr);
}
#endif
