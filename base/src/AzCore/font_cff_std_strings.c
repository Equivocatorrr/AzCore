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
