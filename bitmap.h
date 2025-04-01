#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t type_id;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
};

struct BMPInfoHeader {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
};

struct Pixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};
#pragma pack(pop)

class Bitmap {
public:
    Bitmap();
    Bitmap(const Bitmap& other);
    Bitmap& operator=(const Bitmap& other);
    ~Bitmap();

    bool load_from_file(const std::string& path);
    bool write_to_file(const std::string& path) const;

    void rotate_clockwise();
    void rotate_counterclockwise();
    void apply_gaussian_blur();

    int32_t get_width() const;
    int32_t get_height() const;

private:
    BMPFileHeader file_header_;
    BMPInfoHeader info_header_;
    std::vector<Pixel> pixels_;

    int32_t width_;
    int32_t height_;
    int32_t row_stride_;
    int32_t padding_;

    void clear();
    void calculate_row_stride();
};

#endif // BITMAP_HPP
