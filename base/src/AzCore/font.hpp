/*
    File: font.hpp
    Author: Philip Haynes
    Utilities to load data from font files.
    TODOs:
        1) Support loading of data from fonts.
        2) Organize glyph data, expose combination glyphs to user.
        3) Add basic rendering of vector-based glyphs.
        4) Add support for bitmapped glyphs.
*/

#ifndef AZCORE_FONT_HPP
#define AZCORE_FONT_HPP

#include "common.hpp"
#include "IO/LogStream.hpp"

#include "Font/FontTables.hpp"

#include <fstream>

namespace AzCore {

namespace font {

    extern String error;
    extern io::LogStream cout;

    extern const f32 sdfDistance;

    /*  struct: Curve2
        Author: Philip Haynes
        Defines a single cubic bezier curve.          */
    struct Curve2 {
        vec2 p1, p2, p3, p4;
        // Intersection assumes the ray is traveling in the x-positive direction from the point.
        // Returns:
        //      1 for clockwise-winding intersection
        //      -1 for counter-clockwise-winding intersection
        //      0 for no intersection
        i32 Intersection(const vec2 &point) const;
        inline vec2 Point(const f32& t) const {
            const f32 tInv = 1.0f - t;
            const f32 t2 = t*t;
            const f32 tInv2 = tInv*tInv;
            return p1 * tInv2*tInv + (p2 * tInv2*t + p3 * t2*tInv) * 3.0f + p4 * t2*t;
        }
        f32 DistanceLess(const vec2 &point, f32 distSquared) const;
        void Scale(const mat2& scale);
        void Offset(const vec2& offset);
        void Print(io::LogStream &cout);
    };
    static_assert(sizeof(Curve2) == 32);

    /*  struct: Curve
        Author: Philip Haynes
        Defines a single quadtratic bezier curve.          */
    struct Curve {
        vec2 p1, p2, p3;
        // Intersection assumes the ray is traveling in the x-positive direction from the point.
        // Returns:
        //      1 for clockwise-winding intersection
        //      -1 for counter-clockwise-winding intersection
        //      0 for no intersection
        i32 Intersection(const vec2 &point) const;
        inline vec2 Point(const f32& t) const {
            const f32 tInv = 1.0f - t;
            return p1 * square(tInv) + p2 * (2.0f * t * tInv) + p3 * square(t);
        }
        f32 DistanceLess(const vec2 &point, f32 distSquared) const;
        void Scale(const mat2& scale);
        void Offset(const vec2& offset);
        void Print(io::LogStream &cout);
    };
    static_assert(sizeof(Curve) == 24);

    /*  struct: Line
        Author: Philip Haynes
        Defines a single line segment.          */
    struct Line {
        vec2 p1, p2;
        i32 Intersection(const vec2 &point) const;
        f32 DistanceLess(const vec2 &point, f32 distSquared) const;
        void Scale(const mat2& scale);
        void Offset(const vec2& offset);
        void Print(io::LogStream &cout);
    };
    static_assert(sizeof(Line) == 16);


    /*  struct: Component
        Author: Philip Haynes
        Defines how a glyph should use a single component.  */
    struct Component {
        u16 glyphIndex;
        vec2 offset; // Added to Glyph.offset for total offset
        mat2 transform;
    };

    /*  struct: GlyphInfo
        Author: Philip Haynes
        Defines some attributes of a glyph, like those necessary to draw text.  */
    struct GlyphInfo {
        vec2 pos = vec2(0.0f); // Position in atlas
        vec2 size = vec2(0.0f); // Total dimensions of the contours
        vec2 offset = vec2(0.0f); // Origin point relative to contours
        vec2 advance = vec2(0.0f); // How far to advance
    };

    /*  struct: Glyph
        Author: Philip Haynes
        Defines a single glyph, including all the contours.     */
    struct Glyph {
        // All coordinates are in Em units
        Array<Curve2> curve2s{};
        Array<Curve> curves{};
        Array<Line> lines{};
        Array<Component> components{};
        GlyphInfo info{};
        // Returns whether a point is inside the glyph.
        bool Inside(const vec2 &point) const;
        f32 MinDistance(vec2 point, const f32& startingDist) const;
        void AddFromGlyfPoints(glyfPoint *glyfPoints, i32 count);
        void Scale(const mat2& scale);
        void Offset(const vec2& offset);
        void Print(io::LogStream &cout);
        // Converts curves into lines if they're actually linear
        Glyph& Simplify();
    };

    /*  struct: Font
        Author: Philip Haynes
        Allows loading a single font file, and getting useful information from them.    */
    struct Font {
        struct {
            std::ifstream file;
            tables::TTCHeader ttcHeader;
            Array<tables::Offset> offsetTables;
            // In the case of font collections, we should determine which tables
            // are unique since some of them will be shared between fonts.
            Array<tables::Record> uniqueTables;
            Array<u32> cmaps; // Offset from beginning of tableData for chosen encodings
            tables::cffParsed cffParsed;
            tables::glyfParsed glyfParsed;
            u32 offsetMin = UINT32_MAX;
            u32 offsetMax = 0;
            // Holds all of the tables, which can then be read by referencing
            // any of the offsetTables and finding the desired ones using tags
            Array<char> tableData;
        } data;
        String filename{};

        bool Load();

        u16 GetGlyphIndex(char32 unicode) const;
        Glyph GetGlyphByIndex(u16 index) const;
        Glyph GetGlyph(char32 unicode) const;
        GlyphInfo GetGlyphInfoByIndex(u16 index) const;
        GlyphInfo GetGlyphInfo(char32 unicode) const;
        // Rasterizes a single glyph in the console
        void PrintGlyph(char32 unicode) const;
    };

    struct Box {
        vec2 min, max;
    };

    struct BoxListXNode {
        Array<Box> boxes;

        bool Intersects(Box box);
    };

    struct BoxListX {
        Array<BoxListXNode> nodes;

        void AddBox(Box box);
        bool Intersects(Box box);
    };

    struct BoxListY {
        Array<BoxListX> lists;

        void AddBox(Box box);
        bool Intersects(Box box);
    };

    /*  struct: FontBuilder
        Author: Philip Haynes
        Automatically packs an atlas of signed distance field glyphs from a font file.   */
    struct FontBuilder {
        Font *font = nullptr;
        i32 renderThreadCount = 0; // <1 means hardware concurrency
        vec2i dimensions = vec2i(0); // Size of our image
        Array<u8> pixels; // Actual image data
        ArrayList<i32> indexToId; // Our mapping from glyph index to indices of the below Arrays
        Array<Glyph> glyphs; // Actual glyph data, referenced by id
        // Boxes are used to construct the atlas.
        // We hold on to these in case we want to dynamically add more.
        BoxListY boxes;
        // Potential good places to put new boxes
        Array<vec2> corners;
        f32 area;
        f32 boundSquare;
        // How big our current atlas is
        vec2 bounding;
        f32 scale;
        f32 edge;
        // Which glyphs we need from the font. Not including ones already in the atlas.
        Array<u16> indicesToAdd = {0};
        // Which glyphs are already accounted for and don't need to be added?
        Set<u16> allIndices = {0};
        // Sets the scaling of the actual image
        // HIGH is good for titles and gives the crispest result.
        // MEDIUM is good for subtitles/titles and gives a fairly crisp result.
        // LOW is good for text/subtitles and gives rounded corners only noticed at large scales.
        enum {
            HIGH=64,
            MEDIUM=48,
            LOW=32
        } resolution = MEDIUM;

        // Resizes the image, keeping the pixels in the right spots.
        void ResizeImage(i32 w, i32 h);

        // Adds every available glyph in the range specified.
        bool AddRange(char32 min, char32 max);
        // Adds any glyphs in the string that aren't already included
        bool AddString(WString string);
        // Assembles the atlas and renders the glyphs into it.
        bool Build();
    };

} // namespace font

} // namespace AzCore

#endif // AZCORE_FONT_HPP
