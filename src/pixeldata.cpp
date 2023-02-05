#include <stdexcept>

#include "pixeldata.h"
#include "image.h"
#include "color.h"

// Accepts empty matrices
template <typename T>
bool is_rectangular_vector_matrix(std::vector<std::vector<T>> vector2D) {
    if (vector2D.size() == 0) {
        return true;
    }
    int width = vector2D[0].size();

    for (int row = 0; row < vector2D.size(); row++) {
        if (vector2D[row].size() != width) {
            return false;
        }
    }

    return true;
}


PixelData::PixelData(std::vector<std::vector<RGBColor> >& rawData) {
    if (!is_rectangular_vector_matrix(rawData)) {
        throw std::runtime_error("Cannot initialize pixel data with non-rectangular matrix");
    }

    for (int row = 0; row < rawData.size(); row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < rawData[row].size(); col++) {
            this->pixels[row].push_back(rawData[row][col]);
        }
    }
}

bool PixelData::in_bounds(int row, int col) const {
    return row >= 0 && col >= 0 && row < this->get_height() && col < this->get_width();
}

PixelData::PixelData(std::vector<std::vector<uint8_t> >& rawGrayscaleData) {
    if (!is_rectangular_vector_matrix(rawGrayscaleData)) {
        throw std::runtime_error("Cannot initialize pixel data with non-rectangular matrix");
    }

    for (int row = 0; row < rawGrayscaleData.size(); row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < rawGrayscaleData[row].size(); col++) {
            this->pixels[row].push_back(RGBColor(rawGrayscaleData[row][col]));
        }
    }
}

PixelData::PixelData(AVFrame* videoFrame) {
        
    for (int row = 0; row < videoFrame->height; row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < videoFrame->width; col++) {

            if ((AVPixelFormat)videoFrame->format == AV_PIX_FMT_GRAY8) {
                this->pixels[row].push_back(RGBColor(videoFrame->data[0][row * videoFrame->width + col]));
            } else if ((AVPixelFormat)videoFrame->format == AV_PIX_FMT_RGB24) {
                int data_start = row * videoFrame->width * 3 + col * 3;
                this->pixels[row].push_back(RGBColor( videoFrame->data[0][data_start], videoFrame->data[0][data_start + 1], videoFrame->data[0][data_start + 2] ));
            } else {
                throw std::runtime_error("Passed in AVFrame with unimplemeted format, only supported formats for initializing from AVFrame are AV_PIX_FMT_GRAY8 and AV_PIX_FMT_RGB24");
            }

        }
    }

}


RGBColor PixelData::get_avg_color_from_area(int x, int y, int width, int height) const {
    if (width * height <= 0) {
        throw std::runtime_error("Cannot get average color from an area with negative dimensions: " + std::to_string(width) + " x " + std::to_string(height));
    }
    if (width * height == 0) {
        throw std::runtime_error("Cannot get average color from an area with dimension of 0: " + std::to_string(width) + " x " + std::to_string(height));
    }

    std::vector<RGBColor> colors;
    for (int row = y; row < y + height; row++) {
        for (int col = x; col < x + width; col++) {
            if (this->in_bounds(row, col)) {
                colors.push_back(this->at(row, col));
            }
        }
    }

    if (colors.size() > 0) {
        return get_average_color(colors);
    }
    throw std::runtime_error("Cannot get average color of out of bounds area");
}

RGBColor PixelData::get_avg_color_from_area(double x, double y, double width, double height) const {
    return this->get_avg_color_from_area(std::floor(x), std::floor(y), std::ceil(width), std::ceil(height));

}

PixelData::PixelData(const PixelData& other) {
    for (int row = 0; row < other.get_height(); row++) {
        this->pixels.push_back(std::vector<RGBColor>());
        for (int col = 0; col < other.get_width(); col++) {
            this->pixels[row].push_back(other.at(row, col));
        }
    }
}


int PixelData::get_width() const {
    if (this->pixels.size() == 0) {
        return 0;
    }
    return this->pixels[0].size();
}

int PixelData::get_height() const {
    return this->pixels.size();
}

PixelData PixelData::scale(double amount) const {
    int new_width = this->get_width() * amount;
    int new_height = this->get_height() * amount;
    std::vector<std::vector<RGBColor>> new_pixels;

    for (double row = 0; row < new_height; row++) {
        new_pixels.push_back(std::vector<RGBColor>());
        for (double col = 0; col < new_width; col++) {
            new_pixels[row].push_back( this->get_avg_color_from_area( row / amount, col / amount, amount, amount ) );
        }
    }
    return PixelData(new_pixels);
}

PixelData PixelData::bound(int width, int height) const {
    if (width < this->get_width() && height < this->get_height()) {
        return PixelData(*this);
    }
    int scale_factor = get_scale_factor(this->get_width(), this->get_height(), width, height);
    return this->scale(scale_factor);
}

bool PixelData::equals(const PixelData& other) const {
    if (this->get_width() != other.get_width() || this->get_height() != other.get_height()) {
        return false;
    }

    for (int row = 0; row < this->get_width(); row++) {
        for (int col = 0; col < this->get_height(); col++) {
            RGBColor other_color = other.at(row, col);
            if (!this->at(row, col).equals(other_color)) {
                return false;
            }
        }
    }
    return true;
}

RGBColor PixelData::at(int row, int col) const {
    return this->pixels[row][col];
}