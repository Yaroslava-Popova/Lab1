#include "bitmap.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

Bitmap::Bitmap()
    : width_(0), height_(0), row_stride_(0), padding_(0) {}

Bitmap::~Bitmap() {
    clear();
}

Bitmap::Bitmap(const Bitmap& other)
    : file_header_(other.file_header_),
      info_header_(other.info_header_),
      pixels_(other.pixels_),
      width_(other.width_),
      height_(other.height_),
      row_stride_(other.row_stride_),
      padding_(other.padding_) {}

Bitmap& Bitmap::operator=(const Bitmap& other) {
    if (this != &other) {
        file_header_ = other.file_header_;
        info_header_ = other.info_header_;
        pixels_ = other.pixels_;
        width_ = other.width_;
        height_ = other.height_;
        row_stride_ = other.row_stride_;
        padding_ = other.padding_;
    }
    return *this;
}

void Bitmap::clear() {
    pixels_.clear();
    width_ = 0;
    height_ = 0;
    row_stride_ = 0;
    padding_ = 0;
}

void Bitmap::calculate_row_stride() {
    const int bytes_per_pixel = 3; // 24-bit bitmap
    row_stride_ = width_ * bytes_per_pixel;
    padding_ = (4 - (row_stride_ % 4)) % 4;
    row_stride_ += padding_;
}

bool Bitmap::load_from_file(const std::string& path) {
    clear();
    std::ifstream infile(path, std::ios::binary);
    if (!infile) {
        std::cerr << "Failed to open file: " << path << "\n";
        return false;
    }

    infile.read(reinterpret_cast<char*>(&file_header_), sizeof(file_header_));
    if (file_header_.type_id != 0x4D42) {
        std::cerr << "Not a BMP file: " << path << "\n";
        return false;
    }

    infile.read(reinterpret_cast<char*>(&info_header_), sizeof(info_header_));
    if (info_header_.bit_count != 24) {
        std::cerr << "Only 24-bit BMP files are supported: " << path << "\n";
        return false;
    }

    width_ = info_header_.width;
    height_ = std::abs(info_header_.height);
    calculate_row_stride();

    infile.seekg(file_header_.offset, std::ios::beg);

    size_t pixel_data_size = row_stride_ * height_;
    std::vector<uint8_t> pixel_data(pixel_data_size);
    infile.read(reinterpret_cast<char*>(pixel_data.data()), pixel_data_size);
    if (!infile) {
        std::cerr << "Error reading pixel data from file: " << path << "\n";
        return false;
    }

    pixels_.resize(width_ * height_);

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            size_t pixel_offset = y * row_stride_ + x * 3;
            Pixel pixel;
            pixel.blue = pixel_data[pixel_offset];
            pixel.green = pixel_data[pixel_offset + 1];
            pixel.red = pixel_data[pixel_offset + 2];
            pixels_[(height_ - y - 1) * width_ + x] = pixel;
        }
    }

    return true;
}

bool Bitmap::write_to_file(const std::string& path) const {
    if (pixels_.empty()) {
        std::cerr << "No image data to write.\n";
        return false;
    }

    BMPFileHeader file_header = file_header_;
    BMPInfoHeader info_header = info_header_;

    file_header.size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + row_stride_ * height_;
    info_header.size = sizeof(BMPInfoHeader);
    info_header.width = width_;
    info_header.height = info_header_.height < 0 ? -height_ : height_;
    info_header.image_size = row_stride_ * height_;

    std::ofstream outfile(path, std::ios::binary);
    if (!outfile) {
        std::cerr << "Failed to create file: " << path << "\n";
        return false;
    }

    outfile.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    outfile.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    const uint8_t padding_bytes[3] = {0, 0, 0};

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const Pixel& pixel = pixels_[(height_ - y - 1) * width_ + x];
            outfile.write(reinterpret_cast<const char*>(&pixel.blue), 1);
            outfile.write(reinterpret_cast<const char*>(&pixel.green), 1);
            outfile.write(reinterpret_cast<const char*>(&pixel.red), 1);
        }
        outfile.write(reinterpret_cast<const char*>(padding_bytes), padding_);
    }

    return true;
}

void Bitmap::rotate_clockwise() {
    if (pixels_.empty()) return;

    std::vector<Pixel> rotated_pixels(width_ * height_);
    int new_width = height_;
    int new_height = width_;

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            rotated_pixels[x * new_width + (new_width - y - 1)] = pixels_[y * width_ + x];
        }
    }

    std::swap(width_, height_);
    pixels_ = std::move(rotated_pixels);
    calculate_row_stride();

    info_header_.width = width_;
    info_header_.height = info_header_.height < 0 ? -height_ : height_;
}

void Bitmap::rotate_counterclockwise() {
    if (pixels_.empty()) return;

    std::vector<Pixel> rotated_pixels(width_ * height_);
    int new_width = height_;
    int new_height = width_;

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            rotated_pixels[(new_height - x - 1) * new_width + y] = pixels_[y * width_ + x];
        }
    }

    std::swap(width_, height_);
    pixels_ = std::move(rotated_pixels);
    calculate_row_stride();

    info_header_.width = width_;
    info_header_.height = info_header_.height < 0 ? -height_ : height_;
}

void Bitmap::apply_gaussian_blur() {
    if (pixels_.empty()) return;

    constexpr int K_SIZE = 3;
    constexpr float kernel[K_SIZE][K_SIZE] = {
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f},
        {2 / 16.0f, 4 / 16.0f, 2 / 16.0f},
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f}
    };

    std::vector<Pixel> original_pixels = pixels_;

    for (int y = 1; y < height_ - 1; ++y) {
        for (int x = 1; x < width_ - 1; ++x) {
            float red = 0.0f, green = 0.0f, blue = 0.0f;

            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int idx = (y + ky) * width_ + (x + kx);
                    const Pixel& p = original_pixels[idx];
                    float weight = kernel[ky + 1][kx + 1];

                    red += p.red * weight;
                    green += p.green * weight;
                    blue += p.blue * weight;
                }
            }

            Pixel& current_pixel = pixels_[y * width_ + x];
            current_pixel.red = static_cast<uint8_t>(std::round(red));
            current_pixel.green = static_cast<uint8_t>(std::round(green));
            current_pixel.blue = static_cast<uint8_t>(std::round(blue));
        }
    }
    // Edge pixels remain unchanged
}

int32_t Bitmap::get_width() const {
    return width_;
}

int32_t Bitmap::get_height() const {
    return height_;
}
