#include "bitmap.h"
#include <iostream>

int main() {
    Bitmap original_image;

    if (!original_image.load_from_file("input.bmp")) {
        return 1;
    }

    Bitmap cw_image = original_image;
    cw_image.rotate_clockwise();
    if (!cw_image.write_to_file("output1.bmp")) {
        std::cerr << "Error saving output1.bmp\n";
    }

    Bitmap ccw_image = original_image;
    ccw_image.rotate_counterclockwise();
    if (!ccw_image.write_to_file("output2.bmp")) {
        std::cerr << "Error saving output2.bmp\n";
    }

    Bitmap blurred_image = original_image;
    blurred_image.apply_gaussian_blur();
    if (!blurred_image.write_to_file("output3.bmp")) {
        std::cerr << "Error saving output3.bmp\n";
    }

    return 0;
}
