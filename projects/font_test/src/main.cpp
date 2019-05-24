#include "AzCore/log_stream.hpp"
#include "AzCore/memory.hpp"
#include "AzCore/font.hpp"

#define pow(v, e) pow((double)v, (double)e)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#undef pow

io::logStream cout("main.log");

i32 main(i32 argumentCount, char **argumentValues) {

    if (argumentCount == 1) {
        cout << "In order to use this program, you must pass the name of a font file as an argument." << std::endl;
        return 0;
    }
    ClockTime start = Clock::now();
    font::Font font;
    font.filename = argumentValues[1];
    if (!font.Load()) {
        cout << "Failed to load font: " << font::error << std::endl;
        return 1;
    }
    WString string = ToWString("私ñÑēÈèéîêâô∵…ėȯȧıëäöïü学元気出区電話番号이작품희망");
    // for (u32 i = 0; i < 256; i++) {
    //     for (char32 c : string) {
    //         font.PrintGlyph(c);
    //     }
    // }
    // for (u32 i = 0; i < 64; i++) {
    //     for (char32 c = 32; c < 128; c++) {
    //         font.PrintGlyph(c);
    //     }
    // }
    // font.PrintGlyph((char32)'2');
    font::FontBuilder fontBuilder;
    fontBuilder.font = &font;
    cout << "AddRange" << std::endl;
    if (!fontBuilder.AddRange(0, 65535)) {
        cout << "Failed fontBuilder.AddRange: " << font::error << std::endl;
        return 1;
    }
    // cout << "AddString" << std::endl;
    // if (!fontBuilder.AddString(string)) {
    //     cout << "Failed fontBuilder.AddString: " << font::error << std::endl;
    //     return 1;
    // }
    cout << "Build" << std::endl;
    if (!fontBuilder.Build()) {
        cout << "Failed fontBuilder.Build: " << font::error << std::endl;
        return 1;
    }
    cout << "Total time: " << std::chrono::duration_cast<Milliseconds>(Clock::now()-start).count() << "ms" << std::endl;
    stbi_write_png("data/atlas.png", fontBuilder.dimensions.x, fontBuilder.dimensions.y, 1, fontBuilder.pixels.data, fontBuilder.dimensions.x);
    return 0;
}
