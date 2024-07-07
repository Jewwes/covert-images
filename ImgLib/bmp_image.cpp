#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>
#include <vector>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    char signature[2];    
    uint32_t file_size;     
    uint32_t reserved;       
    uint32_t data_offset;    
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t header_size;   
    int32_t width;           
    int32_t height;             
    uint16_t planes;      
    uint16_t bits_per_pixel;      
    uint32_t compression;      
    uint32_t image_size;      
    int32_t horizontal_resolution; 
    int32_t vertical_resolution;  
    uint32_t colors_used;       
    uint32_t colors_important;  
}
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    if (!out.is_open()) {
        return false;
    }

    const int w = image.GetWidth();
    const int h = image.GetHeight();
    vector<char> buffer(GetBMPStride(w));

    BitmapFileHeader file_header = {
        .signature{'B', 'M'},
        .file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + static_cast<int>(buffer.size()) * h,
        .reserved = 0,
        .data_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader)
    };

    BitmapInfoHeader info_header = {
        .header_size = sizeof(BitmapInfoHeader),
        .width = w,
        .height = h,
        .planes = 1,
        .bits_per_pixel = 24,
        .compression = 0,
        .image_size = buffer.size() * h,
        .horizontal_resolution = 11811,
        .vertical_resolution = 11811,
        .colors_used = 0,
        .colors_important = 0x1000000
    };

    out.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < w; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buffer.data(), buffer.size());
    }

    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);
    if (!ifs.is_open()) {
        return {}; 
    }

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    ifs.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    ifs.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    if (string_view(file_header.signature, 2) != "BM" || info_header.bits_per_pixel != 24) {
        return {}; 
    }

    const int w = info_header.width;
    const int h = info_header.height;
    Image result(w, h, Color::Black());
    vector<char> buffer(GetBMPStride(w));

    for (int y = h - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buffer.data(), buffer.size());

        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]);
        }
    }

    return result;
}
}
