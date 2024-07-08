#include "bmp_image.h" 
#include "pack_defines.h" 

#include <array> 
#include <fstream> 
#include <string_view> 
#include <vector> 

using namespace std; 

namespace img_lib { 

constexpr int BMP_COLOR_DEPTH = 24;
constexpr int BMP_ALIGNMENT = 4;
constexpr int BMP_COLOR_CHANNELS = 3;

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

static int GetBMPStride(int width) { 
    return BMP_ALIGNMENT * ((width * BMP_COLOR_CHANNELS + BMP_ALIGNMENT - 1) / BMP_ALIGNMENT); 
} 

bool SaveBMP(const Path& file, const Image& image) { 
    ofstream out(file, ios::binary); 
    if (!out.is_open()) { 
        return false; 
    } 

    const int width = image.GetWidth(); 
    const int height = image.GetHeight(); 
    vector<char> buffer(GetBMPStride(width)); 

    BitmapFileHeader file_header = { 
        .signature{'B', 'M'}, 
        .file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + static_cast<int>(buffer.size()) * height, 
        .reserved = 0, 
        .data_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) 
    }; 

    BitmapInfoHeader info_header = { 
        .header_size = sizeof(BitmapInfoHeader), 
        .width = width, 
        .height = height, 
        .planes = 1, 
        .bits_per_pixel = BMP_COLOR_DEPTH, 
        .compression = 0, 
        .image_size = buffer.size() * height, 
        .horizontal_resolution = 11811, 
        .vertical_resolution = 11811, 
        .colors_used = 0, 
        .colors_important = 0x1000000 
    }; 

    out.write(reinterpret_cast<char*>(&file_header), sizeof(file_header)); 
    out.write(reinterpret_cast<char*>(&info_header), sizeof(info_header)); 

    for (int y = height - 1; y >= 0; --y) { 
        const Color* line = image.GetLine(y); 
        for (int x = 0; x < width; ++x) { 
            buffer[x * BMP_COLOR_CHANNELS + 0] = static_cast<char>(line[x].b); 
            buffer[x * BMP_COLOR_CHANNELS + 1] = static_cast<char>(line[x].g); 
            buffer[x * BMP_COLOR_CHANNELS + 2] = static_cast<char>(line[x].r); 
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
    if (!ifs) {
        return {};
    }
    
    ifs.read(reinterpret_cast<char*>(&info_header), sizeof(info_header)); 
    if (!ifs) {
        return {};
    }

    if (string_view(file_header.signature, 2) != "BM" || info_header.bits_per_pixel != BMP_COLOR_DEPTH) { 
        return {};  
    } 

    const int width = info_header.width; 
    const int height = info_header.height; 
    Image result(width, height, Color::Black()); 
    vector<char> buffer(GetBMPStride(width)); 

    for (int y = height - 1; y >= 0; --y) { 
        Color* line = result.GetLine(y); 
        ifs.read(buffer.data(), buffer.size()); 
        if (!ifs) {
            return {};
        }

        for (int x = 0; x < width; ++x) { 
            line[x].b = static_cast<byte>(buffer[x * BMP_COLOR_CHANNELS + 0]); 
            line[x].g = static_cast<byte>(buffer[x * BMP_COLOR_CHANNELS + 1]); 
            line[x].r = static_cast<byte>(buffer[x * BMP_COLOR_CHANNELS + 2]); 
        } 
    } 

    return result; 
} 
} 
