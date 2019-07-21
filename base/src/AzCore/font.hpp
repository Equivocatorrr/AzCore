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

#ifndef FONT_HPP
#define FONT_HPP

#include "common.hpp"
#include "log_stream.hpp"

#include "font_tables.hpp"

#include <fstream>

namespace font {

    extern String error;
    extern io::logStream cout;

    extern const f32 sdfDistance;

    /*  struct: Curve
        Author: Philip Haynes
        Defines a single bezier curve.          */
    struct Curve {
        vec2 p1, p2, p3;
        // Intersection assumes the ray is traveling in the x-positive direction from the point.
        // Returns:
        //      1 for clockwise-winding intersection
        //      -1 for counter-clockwise-winding intersection
        //      0 for no intersection
        i32 Intersection(const vec2& point) const;
        vec2 Point(const f32& t) const;
        void DistanceLess(const vec2& point, f32& distSquared) const;
        void Scale(const mat2& scale);
        void Offset(const vec2& offset);
    };
    static_assert(sizeof(Curve) == 24);

    /*  struct: Line
        Author: Philip Haynes
        Defines a single line segment.          */
    struct Line {
        vec2 p1, p2;
        i32 Intersection(const vec2& point) const;
        void DistanceLess(const vec2& point, f32& distSquared) const;
        void Scale(const mat2& scale);
        void Offset(const vec2& offset);
    };
    static_assert(sizeof(Line) == 16);

    /*  struct: Contour
        Author: Philip Haynes
        Defines a single glyph contour.      */
    // struct Contour {
    //     Array<Curve> curves;
    //     Array<Line> lines;
    //     // Finds all intersections and returns the total winding number
    //     i32 Intersection(const vec2& point) const;
    //     // Finds the minimum distance between the contour and the point, and replaces distSquared if it's less.
    //     void DistanceLess(const vec2& point, f32& distSquared) const;
    //     // Expands an array of glyfPoints into always having the control point available
    //     void FromGlyfPoints(glyfPoint *glyfPoints, i32 count);
    //     void Scale(const mat2& scale);
    //     void Offset(const vec2& offset);
    // };

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
        vec2 pos = vec2(0.0); // Position in atlas
        vec2 size = vec2(0.0); // Total dimensions of the contours
        vec2 offset = vec2(0.0); // Origin point relative to contours
        vec2 advance = vec2(0.0); // How far to advance
    };

    /*  struct: Glyph
        Author: Philip Haynes
        Defines a single glyph, including all the contours.     */
    struct Glyph {
        // All coordinates are in Em units
        Array<Curve> curves{};
        Array<Line> lines{};
        Array<Component> components{};
        // Array<Contour> contours;
        GlyphInfo info{};
        // Returns whether a point is inside the glyph.
        bool Inside(const vec2& point) const;
        f32 MinDistance(const vec2& point, const f32& startingDist) const;
        void AddFromGlyfPoints(glyfPoint *glyfPoints, i32 count);
        void Scale(const mat2& scale);
        void Offset(const vec2& offset);
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

#endif // FONT_HPP
