#include "AzCore/log_stream.hpp"
#include "AzCore/memory.hpp"
#include "AzCore/font.hpp"

io::logStream cout("main.log");

i32 main(i32 argumentCount, char **argumentValues) {

    if (argumentCount == 1) {
        cout << "In order to use this program, you must pass the name of a font file as an argument." << std::endl;
        return 0;
    }
    font::Font font;
    font.filename = argumentValues[1];
    if (!font.Load()) {
        cout << "Failed to load font: " << font::error << std::endl;
        return 1;
    }
    ClockTime start = Clock::now();
    WString string = ToWString("私ñÑēÈèéîêâô∵…ėȯȧıëäöïü学元気出区電話番号이작품희망");
    for (char32 c : string) {
        font.PrintGlyph(c);
    }
    // for (u32 i = 0; i < 64; i++) {
        // for (char32 c = 32; c < 256; c++) {
        //     font.PrintGlyph(c);
        // }
    // }
    // font.PrintGlyph((char32)'2');
    cout << "Total time: " << std::chrono::duration_cast<Milliseconds>(Clock::now()-start).count() << "ms" << std::endl;
    return 0;
}
