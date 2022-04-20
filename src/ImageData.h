#pragma once

#include <vector>

class ImageData {
private:
    unsigned int width = 0;
    unsigned int height = 0;
    std::vector<unsigned char> pixels;

public:
    [[nodiscard]] unsigned int getWidth() const {
        return width;
    }

    void setWidth(unsigned int const &aWidth) {
        ImageData::width = aWidth;
    }

    [[nodiscard]] unsigned int getHeight() const {
        return height;
    }

    void setHeight(unsigned int const &aHeight) {
        ImageData::height = aHeight;
    }

    [[nodiscard]] const std::vector<unsigned char> &getPixels() const {
        return const_cast<std::vector<unsigned char> &>(pixels);
    }

    void setPixels(void *src, unsigned long long const &size) {
        pixels.resize(size);
        std::memcpy(pixels.data(), src, size);
    }

    void setPixels(std::vector<unsigned char> const &src) {
        pixels.resize(src.size());
        std::memcpy(pixels.data(), src.data(), src.size());
    }

    [[nodiscard]] const unsigned char *getPixelsPointer() const {
        return const_cast<unsigned char *>(pixels.data());
    }

    [[nodiscard]] unsigned long long getPixelsSize() const {
        return pixels.size();
    }
};
