const char *stdStrings[391] = {
".notdef",              "space",                "exclam",               "quotedbl",             "numbersign",       // 0
"dollar",               "percent",              "ampersand",            "quoteright",           "parenleft",
"parenright",           "asterisk",             "plus",                 "comma",                "hyphen",           // 10
"period",               "slash",                "zero",                 "one",                  "two",
"three",                "four",                 "five",                 "six",                  "seven",            // 20
"eight",                "nine",                 "colon",                "semicolon",            "less",
"equal",                "greater",              "question",             "at",                   "A",                // 30
"B",                    "C",                    "D",                    "E",                    "F",
"G",                    "H",                    "I",                    "J",                    "K",                // 40
"L",                    "M",                    "N",                    "O",                    "P",
"Q",                    "R",                    "S",                    "T",                    "U",                // 50
"V",                    "W",                    "X",                    "Y",                    "Z",
"bracketleft",          "backslash",            "bracketright",         "asciicircum",          "underscore",       // 60
"quoteleft",            "a",                    "b",                    "c",                    "d",
"e",                    "f",                    "g",                    "h",                    "i",                // 70
"j",                    "k",                    "l",                    "m",                    "n",
"o",                    "p",                    "q",                    "r",                    "s",                // 80
"t",                    "u",                    "v",                    "w",                    "x",
"y",                    "z",                    "braceleft",            "bar",                  "braceright",       // 90
"asciitilde",           "exclamdown",           "cent",                 "sterling",             "fraction",
"yen",                  "florin",               "section",              "currency",             "quotesingle",      // 100
"quotedblleft",         "guillemotleft",        "guilsinglleft",        "guilsinglright",       "fi",
"fl",                   "endash",               "dagger",               "daggerdbl",            "periodcentered",   // 110
"paragraph",            "bullet",               "quotesinglbase",       "quotedblbase",         "quotedblright",
"guillemotright",       "ellipsis",             "perthousand",          "questiondown",         "grave",            // 120
"acute",                "circumflex",           "tilde",                "macron",               "breve",
"dotaccent",            "dieresis",             "ring",                 "cedilla",              "hungarumlaut",     // 130
"ogonek",               "caron",                "emdash",               "AE",                   "ordfeminine",
"Lslash",               "Oslash",               "OE",                   "ordmasculine",         "ae",               // 140
"dotlessi",             "lslash",               "oslash",               "oe",                   "germandbls",
"onesuperior",          "logicalnot",           "mu",                   "trademark",            "Eth",              // 150
"onehalf",              "plusminus",            "Thorn",                "onequarter",           "divide",
"brokenbar",            "degree",               "thorn",                "threequarters",        "twosuperior",      // 160
"registered",           "minus",                "eth",                  "multiply",             "threesuperior",
"copyright",            "Aacute",               "Acircumflex",          "Adieresis",            "Agrave",           // 170
"Aring",                "Atilde",               "Ccedilla",             "Eacute",               "Ecircumflex",
"Edieresis",            "Egrave",               "Iacute",               "Icircumflex",          "Idieresis",        // 180
"Igrave",               "Ntilde",               "Oacute",               "Ocircumflex",          "Odieresis",
"Ograve",               "Otilde",               "Scaron",               "Uacute",               "Ucircumflex",      // 190
"Udieresis",            "Ugrave",               "Yacute",               "Ydieresis",            "Zcaron",
"aacute",               "acircumflex",          "adieresis",            "agrave",               "aring",            // 200
"atilde",               "ccedilla",             "eacute",               "ecircumflex",          "edieresis",
"egrave",               "iacute",               "icircumflex",          "idieresis",            "igrave",           // 210
"ntilde",               "oacute",               "ocircumflex",          "odieresis",            "ograve",
"otilde",               "scaron",               "uacute",               "ucircumflex",          "udieresis",        // 220
"ugrave",               "yacute",               "ydieresis",            "zcaron",               "exclamsmall",
"Hungarumlautsmall",    "dollaroldstyle",       "dollarsuperior",       "ampersandsmall",       "Acutesmall",       // 230
"parenleftsuperior",    "parenrightsuperior",   "twodotenleader",       "onedotenleader",       "zerooldstyle",
"oneoldstyle",          "twooldstyle",          "threeoldstyle",        "fouroldstyle",         "fiveoldstyle",     // 240
"sixoldstyle",          "sevenoldstyle",        "eightoldstyle",        "nineoldstyle",         "commasuperior",
"threequartersemdash",  "periodsuperior",       "questionsmall",        "asuperior",            "bsuperior",        // 250
"centsuperior",         "dsuperior",            "esuperior",            "isuperior",            "lsuperior",
"msuperior",            "nsuperior",            "osuperior",            "rsuperior",            "ssuperior",        // 260
"tsuperior",            "ff",                   "ffi",                  "ffl",                  "parenleftinferior",
"parenrightinferior",   "Circumflexsmall",      "hyphensuperior",       "Gravesmall",           "Asmall",           // 270
"Bsmall",               "Csmall",               "Dsmall",               "Esmall",               "Fsmall",
"Gsmall",               "Hsmall",               "Ismall",               "Jsmall",               "Ksmall",           // 280
"Lsmall",               "Msmall",               "Nsmall",               "Osmall",               "Psmall",
"Qsmall",               "Rsmall",               "Ssmall",               "Tsmall",               "Usmall",           // 290
"Vsmall",               "Wsmall",               "Xsmall",               "Ysmall",               "Zsmall",
"colonmonetary",        "onefitted",            "rupiah",               "Tildesmall",           "exclamdownsmall",  // 300
"centoldstyle",         "Lslashsmall",          "Scaronsmall",          "Zcaronsmall",          "Dieresissmall",
"Brevesmall",           "Caronsmall",           "Dotaccentsmall",       "Macronsmall",          "figuredash",       // 310
"hypheninferior",       "Ogoneksmall",          "Ringsmall",            "Cedillasmall",         "questiondownsmall",
"oneeighth",            "threeeighths",         "fiveeighths",          "seveneighths",         "onethird",         // 320
"twothirds",            "zerosuperior",         "foursuperior",         "fivesuperior",         "sixsuperior",
"sevensuperior",        "eightsuperior",        "ninesuperior",         "zeroinferior",         "oneinferior",      // 330
"twoinferior",          "threeinferior",        "fourinferior",         "fiveinferior",         "sixinferior",
"seveninferior",        "eightinferior",        "nineinferior",         "centinferior",         "dollarinferior",   // 340
"periodinferior",       "commainferior",        "Agravesmall",          "Aacutesmall",          "Acircumflexsmall",
"Atildesmall",          "Adieresissmall",       "Aringsmall",           "AEsmall",              "Ccedillasmall",    // 350
"Egravesmall",          "Eacutesmall",          "Ecircumflexsmall",     "Edieresissmall",       "Igravesmall",
"Iacutesmall",          "Icircumflexsmall",     "Idieresissmall",       "Ethsmall",             "Ntildesmall",      // 360
"Ogravesmall",          "Oacutesmall",          "Ocircumflexsmall",     "Otildesmall",          "Odieresissmall",
"OEsmall",              "Oslashsmall",          "Ugravesmall",          "Uacutesmall",          "Ucircumflexsmall", // 370
"Udieresissmall",       "Yacutesmall",          "Thornsmall",           "Ydieresissmall",       "001.000",
"001.001",              "001.002",              "001.003",              "Black",                "Bold",             // 380
"Book",                 "Light",                "Medium",               "Regular",              "Roman",
"Semibold"                                                                                                          // 390
};

const SID stdEncoding0[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 16
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, // 32
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, // 48
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, // 64
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, // 80
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, // 96
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 0,  // 112
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 128
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 144
    0,  96, 97, 98, 99, 100,101,102,103,104,105,106,107,108,109,110,// 160
    0,  111,112,113,114,0,  115,116,117,118,119,120,121,122,0,  123,// 176
    0,  124,125,126,127,128,129,130,131,0,  132,133,0,  134,135,136,// 192
    137,0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 208
    0,  138,0,  139,0,  0,  0,  0,  140,141,142,143,0,  0,  0,  0,  // 224
    0,  144,0,  0,  0,  145,0,  0,  146,147,148,149,0,  0,  0,  0   // 240
};

const SID stdEncoding1[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 16
    1,  229,230,0,  231,232,233,234,235,236,237,238,13, 14, 15, 99, // 32
    239,240,241,242,243,244,245,246,247,248,27, 28, 249,250,251,252,// 48
    0,  253,254,255,256,257,0,  0,  0,  258,0,  0,  259,260,261,262,// 64
    0,  0,  263,264,265,0,  266,109,110,267,268,269,0,  270,271,272,// 80
    273,274,275,276,277,278,279,280,281,282,283,284,285,286,287,288,// 96
    289,290,291,292,293,294,295,296,297,298,299,300,301,302,303,0,  // 112
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 128
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 144
    0,  304,305,306,0,  0,  307,308,309,310,311,0,  312,0,  0,  313,// 160
    0,  0,  314,315,0,  0,  316,317,318,0,  0,  0,  158,155,163,319,// 176
    320,321,322,323,324,325,0,  0,  326,150,164,169,327,328,329,330,// 192
    331,332,333,334,335,336,337,338,339,340,341,342,343,344,345,346,// 208
    347,348,349,350,351,352,353,354,355,356,357,358,359,360,361,362,// 224
    363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378 // 240
};

const SID stdCharset1[166] = {
    0,  1,  229,230,231,232,233,234,235,236,237,238,13, 14, 15, 99, // 0
    239,240,241,242,243,244,245,246,247,248,27, 28, 249,250,251,252,// 16
    253,254,255,256,257,258,259,260,261,262,263,264,265,266,109,110,// 32
    267,268,269,270,271,272,273,274,275,276,277,278,279,280,281,282,// 48
    283,284,285,286,287,288,289,290,291,292,293,294,295,296,297,298,// 64
    299,300,301,302,303,304,305,306,307,308,309,310,311,312,313,314,// 80
    315,316,317,318,158,155,163,319,320,321,322,323,324,325,326,150,// 96
    164,169,327,328,329,330,331,332,333,334,335,336,337,338,339,340,// 112
    341,342,343,344,345,346,347,348,349,350,351,352,353,354,355,356,// 128
    357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,// 144
    373,374,375,376,377,378                                         // 160
};

const SID stdCharset2[87] = {
    0,  1,  231,232,235,236,237,238,13, 14, 15, 99, 239,240,241,242,// 0
    243,244,245,246,247,248,27, 28, 249,250,251,253,254,255,256,257,// 16
    258,259,260,261,262,263,264,265,266,109,110,267,268,269,270,272,// 32
    300,301,302,305,314,315,158,155,163,320,321,322,323,324,325,326,// 48
    150,164,169,327,328,329,330,331,332,333,334,335,336,337,338,339,// 64
    340,341,342,343,344,345,346                                     // 80
};
