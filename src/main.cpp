#include <iostream>
#include "DirectXTex.h"
#include "DDS.h"
#include <fstream>
#include <wincodec.h>

#include <windows.h>
#include <tchar.h>
#include <algorithm>

void ErrorDescription(HRESULT hr) {
    if (FACILITY_WINDOWS == HRESULT_FACILITY(hr))
        hr = HRESULT_CODE(hr);
    TCHAR *szErrMsg;

    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &szErrMsg, 0, NULL) != 0) {
        _tprintf(TEXT("%s"), szErrMsg);
        LocalFree(szErrMsg);
    } else
        _tprintf(TEXT("[Could not find a description for error # %#x.]\n"), hr);
}

unsigned int byteSwap(unsigned int x) {
    return (x & 0xff000000) >> 24 | ((x & 0x00ff0000) >> 8) | ((x & 0x0000ff00) << 8) | ((x & 0xff) << 24);
}

int test1() {
    printf("test1!\n");

    //HRESULT LoadFromDDSMemory(const void *pSource, size_t size, DDS_FLAGS flags, TexMetadata *metadata, ScratchImage &image) noexcept
//    std::ifstream texfile("D:/temp/tq/tex-xp4/quest/questmapchina.tex", std::ios::binary | std::ios::ate);
//    std::ifstream texfile("D:/temp/tq/tex-xp3/quest/questmapatlantis01.tex", std::ios::binary | std::ios::ate);
//    std::ifstream texfile("D:/temp/tq/tex-xp2/quest/questmapnorth01.tex", std::ios::binary | std::ios::ate);
//    std::ifstream texfile("D:/temp/tq/tex-xp4/quest/a.tex", std::ios::binary | std::ios::ate);
//    std::ifstream texfile("D:/temp/a.dds", std::ios::binary | std::ios::ate);
//    std::ifstream texfile("D:/temp/rocksgrass01.dds", std::ios::binary | std::ios::ate);
    std::ifstream texfile;
    std::string filename("D:/temp/tq/tex-xp1/quest/questmaphades01.tex");
    try {
        texfile.open(filename, std::ios::binary | std::ios::ate);
        if (!texfile.is_open()) {
            std::fprintf(stderr, "Error opening file %s\n", filename.c_str());
            return 1;
        }
    } catch (const std::ios::failure &e) {
        fprintf(stderr, "Error: %s", e.what());
        return 1;
    }

    std::streamsize texsize = texfile.tellg();
    texfile.seekg(0, std::ios::beg);

    std::vector<unsigned char> buf(texsize);
    texfile.read(reinterpret_cast<char *>(buf.data()), texsize);

    if((unsigned int) buf.size() != texsize) {
        fprintf(stderr, "Error reading file");
        return 1;
    }

    unsigned int containerMagic = byteSwap(*reinterpret_cast<const int *>(&buf[0]));
    int texVersion = 0;
    int textureLength = 0;
    if (containerMagic == 0x54455802 || containerMagic == 0x54455801) {
        std::printf("TEX FOUND, magic: %08x\n", containerMagic);
        if ((containerMagic & 0xff) == 0x01) {
            //version 1
            texVersion = 1;
            textureLength = *reinterpret_cast<const int *>(&buf[8]);
            buf.erase(buf.begin(), buf.begin() + 12);
        } else if ((containerMagic & 0xff) == 0x02) {
            //version 2
            texVersion = 2;
            textureLength = *reinterpret_cast<const int *>(&buf[9]);
            buf.erase(buf.begin(), buf.begin() + 13);
        } else {
            throw std::exception("unsupported tex");
        }
        std::printf("TEXTURELENGTH: %d\n", textureLength);
        //remove 'R' from magic DDSR
        buf[3] = 0x20;

        std::printf("DDS HEADER STRUCTURE SIZE: %d\n", *reinterpret_cast<const int *>(&buf[4]));
        std::printf("DDS HEADER FLAGS: %d\n", *reinterpret_cast<const int *>(&buf[8]));

        int dwSize = *reinterpret_cast<const int *>(&buf[76]);
        int dwFlags = *reinterpret_cast<const int *>(&buf[80]);
        int dwFourCC = *reinterpret_cast<const int *>(&buf[84]);
        int dwRGBBitCount = *reinterpret_cast<const int *>(&buf[88]);
        int dwRBitMask = *reinterpret_cast<const int *>(&buf[92]);
        int dwGBitMask = *reinterpret_cast<const int *>(&buf[96]);
        int dwBBitMask = *reinterpret_cast<const int *>(&buf[100]);
        int dwABitMask = *reinterpret_cast<const int *>(&buf[104]);

        std::printf("DDS_PIXELFORMAT dwSize: %d\n", dwSize);
        std::printf("DDS_PIXELFORMAT dwFlags: %d\n", dwFlags);
        std::printf("DDS_PIXELFORMAT dwFourCC: %d\n", dwFourCC);
        std::printf("DDS_PIXELFORMAT dwRGBBitCount: %d\n", dwRGBBitCount);
        std::printf("DDS_PIXELFORMAT dwRBitMask: %d\n", dwRBitMask);
        std::printf("DDS_PIXELFORMAT dwGBitMask: %d\n", dwGBitMask);
        std::printf("DDS_PIXELFORMAT dwBBitMask: %d\n", dwBBitMask);
        std::printf("DDS_PIXELFORMAT dwABitMask: %d\n", dwABitMask);

        if (dwFlags == 0x40 && (dwRBitMask == 0 || dwGBitMask == 0 || dwBBitMask == 0)) {
            //texture must have bitmask
            if (dwRGBBitCount >= 24) {
                //set rgb masks for A8R8G8B8
                buf.erase(buf.begin() + 92, buf.begin() + 95);
                buf.insert(buf.begin() + 92, {0x00, 0x00, 0xff, 0x00});
                buf.erase(buf.begin() + 96, buf.begin() + 99);
                buf.insert(buf.begin() + 96, {0x00, 0xff, 0x00, 0x00});
                buf.erase(buf.begin() + 100, buf.begin() + 103);
                buf.insert(buf.begin() + 100, {0xff, 0x00, 0x00, 0x00});
            }
        }
        if (dwRGBBitCount == 32 && (dwABitMask == 0 || (buf[80] & 1) == 0)) {
            //only 32bit depth can have alpha channel bit mask
            //enable
            buf[80] |= DDS_ALPHAPIXELS;
            //set alpha mask for A8R8G8B8
            buf.erase(buf.begin() + 104, buf.begin() + 107);
            buf.insert(buf.begin() + 104, {0x00, 0x00, 0x00, 0xff});
        }
        //
        buf[109] |= DDS_SURFACE_FLAGS_TEXTURE;

        /**
         struct DDS_PIXELFORMAT {
          DWORD dwSize;
          DWORD dwFlags;
          DWORD dwFourCC;
          DWORD dwRGBBitCount;
          DWORD dwRBitMask;
          DWORD dwGBitMask;
          DWORD dwBBitMask;
          DWORD dwABitMask;
        };
         */
        printf("Red pixel mask: %02X %02x %02x %02x\n", buf[92], buf[93], buf[94], buf[95]);
        printf("Green pixel mask: %02X %02x %02x %02x\n", buf[96], buf[97], buf[98], buf[99]);
        printf("Blue pixel mask: %02X %02x %02x %02x\n", buf[100], buf[101], buf[102], buf[103]);
        printf("Alpha pixel mask: %02X %02x %02x %02x\n", buf[104], buf[105], buf[106], buf[107]);
    }

    unsigned int magic = byteSwap(*reinterpret_cast<const int *>(&buf[0]));
    std::printf("DDS magic: %08x\n", magic);

    std::printf("filesize: %lld; buffer: %llu; sizeof(buf): %llu\n", texsize, buf.size(), sizeof(buf.data()));

    DirectX::ScratchImage image;
    DirectX::TexMetadata metadata{};

    try {
        printf("Loading DDS\n");
        HRESULT resultDds = DirectX::LoadFromDDSMemory(buf.data(), buf.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
        if (FAILED(resultDds)) {
            fprintf(stderr, "ERROR READING DDS\n");
            HRESULT resultWic0 = DirectX::LoadFromWICMemory(buf.data(), buf.size(), DirectX::WIC_FLAGS_NONE, &metadata, image);
            if (FAILED(resultWic0)) {
                fprintf(stderr, "ERROR READING DDS (wic)\n");
            }
            return 1;
        }
    } catch (...) {
        fprintf(stderr, "Error loading dds\n");
    }

    printf("IMAGE: %zu x %zu\n", image.GetMetadata().width, image.GetMetadata().height);

    //HRESULT SaveToWICFile(const Image &image, WIC_FLAGS flags, const _GUID &guidContainerFormat, const wchar_t *szFile, const _GUID *targetFormat = nullptr, std::function<void(IPropertyBag2 *)> setCustomProps = nullptr)
    //HRESULT SaveToWICMemory(const Image &image, WIC_FLAGS flags, const _GUID &guidContainerFormat, Blob &blob, const _GUID *targetFormat = nullptr, std::function<void(IPropertyBag2 *)> setCustomProps = nullptr)
//    DirectX::SaveToWICMemory(*(image->GetImages()), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG, );

    try {
        if (DirectX::IsCompressed(image.GetMetadata().format)) {
            DirectX::ScratchImage uncompressed;
            HRESULT resultDec = DirectX::Decompress(image.GetImages(), image.GetImageCount(),
                                                    image.GetMetadata(),
                                                    DXGI_FORMAT_UNKNOWN, uncompressed);
            if (FAILED(resultDec)) {
                ErrorDescription(resultDec);
                fprintf(stderr, "ERROR DECOMPRESSING TEXTURE\n");
                return 1;
            }
            image.Release();
            std::swap(uncompressed, image);
        }
    } catch (...) {
        fprintf(stderr, "Error uncompressing texture\n");
    }

    CoInitialize(nullptr);
    printf("Writing WIC\n");

    HRESULT resultWicFile = DirectX::SaveToWICFile(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE,
                                            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), L"d:\\temp\\a.png");
    if (FAILED(resultWicFile)) {
        fprintf(stderr, "ERROR WRITING WIC: ");
        ErrorDescription(resultWicFile);
        CoUninitialize();
        return 1;
    }

    DirectX::Blob output;
    HRESULT resultWicMem = DirectX::SaveToWICMemory(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE,
                                            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), output, &GUID_WICPixelFormat32bppBGRA);
    if (FAILED(resultWicMem)) {
        fprintf(stderr, "ERROR WRITING WIC (mem): ");
        ErrorDescription(resultWicMem);
        CoUninitialize();
        return 1;
    }
    CoUninitialize();
    image.Release();

    return 0;
}

int main() {
    printf("Hello, World!\n");

}