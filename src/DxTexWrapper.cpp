//
// Created by epinter on 18/04/2022.
//
#include "DirectXTex.h"
#include "wincodec.h"
#include <tchar.h>
#include "DxTexWrapper.h"

[[maybe_unused]] Data convertDDSToTIFF(unsigned int size, const void *data) {
    Data ret{};
    ret.size = 0;

    DirectX::ScratchImage image{};
    try {
        DirectX::TexMetadata metadata{};
        HRESULT hr = DirectX::LoadFromDDSMemory(data, size, DirectX::DDS_FLAGS_NONE, &metadata, image);
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

    try {
        CoInitialize(nullptr);
        DirectX::Blob output;
        HRESULT hr = DirectX::SaveToWICMemory(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE,
                                              DirectX::GetWICCodec(DirectX::WIC_CODEC_TIFF),
                                              output, &GUID_WICPixelFormat32bppBGRA);

        if (FAILED(hr)) {
            std::fprintf(stderr, "Error writing to WIC buffer: ");
            printErrorDescription(hr);
            return ret;
        }

        ret.content = output.GetBufferPointer();
        ret.size = static_cast<unsigned int>(output.GetBufferSize());
        ret.content = malloc(ret.size);
        std::memcpy(ret.content, output.GetBufferPointer(), ret.size);
        image.Release();
        output.Release();
    } catch (...) {
        std::fprintf(stderr, "Error writing to WIC buffer");
    }
    CoUninitialize();
    return ret;
}

void printErrorDescription(HRESULT hr) {
    if (FACILITY_WINDOWS == HRESULT_FACILITY(hr))
        hr = HRESULT_CODE(hr);
    TCHAR *szErrMsg;

    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&szErrMsg), 0, nullptr) != 0) {
        _ftprintf(stderr, TEXT("%s"), szErrMsg);
        LocalFree(szErrMsg);
    } else
        _ftprintf(stderr, TEXT("[Could not find a description for error # %#lx.]\n"), hr);
}

