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

    /*  struct: Glyph
        Author: Philip Haynes
        Defines a single glyph, including all the contours.     */
    struct Glyph {
        // All coordinates are in Em units
        Array<Curve> curves;
        Array<Line> lines;
        // Array<Contour> contours;
        vec2 size; // Total dimensions of the contours
        vec2 offset; // Origin point relative to contours
        vec2 advance; // How far to advance
        // Returns whether a point is inside the glyph.
        bool Inside(const vec2& point) const;
        f32 MinDistance(const vec2& point) const;
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

        // Rasterizes a single glyph in the console
        void PrintGlyph(char32 glyph);
    };

} // namespace font

#endif // FONT_HPP
