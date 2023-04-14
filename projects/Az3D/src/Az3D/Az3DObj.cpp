/*
	File: Az3DObj.cpp
	Author: Philip Haynes
*/

#include "Az3DObj.hpp"
#include "AzCore/IO/Log.hpp"
#include "profiling.hpp"
#include "AzCore/Memory/Endian.hpp"

// NOTE: File is little endian, and current target platform is little endian.
// If this ever changes in the future we can do endian stuff. For now we do the easy thing.

namespace Az3D::Az3DObj {

using namespace AzCore;

constexpr Str AZ3D_MAGIC = Str("Az3DObj\0", 8);

// 1st argument must be a "string literal", __VA_ARGS__ breaks somewhere when I add the comma.
#define PRINT_ERROR(...) io::cerr.PrintLn(__FUNCTION__ " error: " __VA_ARGS__)

#define EXPECT_SPACE_IN_BUFFER(length) if (buffer.size-cur < (length)) {\
	PRINT_ERROR("Buffer underflow!");\
	return false;\
}

#define EXPECT_TAG_IN_BUFFER(tagName, len) if (strncmp(&buffer[cur], (tagName), (len)) != 0) {\
	PRINT_ERROR("Incorrect tag \"", Str(&buffer[cur], (len)), "\" (expected \"", (tagName), "\")");\
	return false;\
}

namespace Headers {
	// Header that must exist at the start of the file
	struct File {
		static constexpr i64 SIZE = 16;
		char magic[8];
		u16 versionMajor;
		u16 versionMinor;
		u16 vertexStride;
		u16 indexStride;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER(AZ3D_MAGIC.str, 8);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			return true;
		}
	};
	
	// Header that defines a section of data with a length
	struct Table {
		static constexpr i64 SIZE = 8;
		// ident denotes what kind of data this section contains
		char ident[4];
		// section length includes this header
		u32 length;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			return true;
		}
	};

} // namespace Headers

namespace Types {

	// Encodes a string of text in the file
	struct Name {
		static constexpr i64 SIZE = 8;
		char ident[4];
		u32 length;
		// char name[length];
		Str name;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Name", 4);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			EXPECT_SPACE_IN_BUFFER(length);
			name = Str(&buffer[cur], length);
			cur += align(length, 4);
			return true;
		}
	};
	
	struct Vert {
		static constexpr i64 SIZE = 8;
		char ident[4];
		u32 count;
		// Vertex vertices[count];
		Array<Vertex> vertices;
		bool FromBuffer(Str buffer, i64 &cur, u32 vertexStride) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Vert", 4);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			i64 length = count * vertexStride;
			EXPECT_SPACE_IN_BUFFER(length);
			vertices.Resize(count);
			if (vertexStride == sizeof(Vertex)) {
				memcpy(vertices.data, &buffer[cur], length);
			} else if (vertexStride < 32) {
				return false;
			} else {
				for (i64 i = 0; i < count; i++) {
					vertices[i] = *(Vertex*)&buffer[cur + i*vertexStride];
				}
			}
			cur += align(length, 4);
			return true;
		}
	};
	
	struct Indx {
		static constexpr i64 SIZE = 8;
		char ident[4];
		u32 count;
		// u32 indices[count];
		Array<u32> indices;
		bool FromBuffer(Str buffer, i64 &cur, u32 indexStride) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Indx", 4);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			i64 length = count * indexStride;
			EXPECT_SPACE_IN_BUFFER(length);
			indices.Resize(count);
			// Support indexStrides other than 4 just for funsies
			switch (indexStride) {
			case 1:
				for (i64 i = 0; i < count; i++) {
					indices[i] = *(u8*)&buffer[cur + i*indexStride];
				} break;
			case 2:
				for (i64 i = 0; i < count; i++) {
					indices[i] = *(u16*)&buffer[cur + i*indexStride];
				} break;
			case 4:
				memcpy(indices.data, &buffer[cur], length);
				break;
			default:
				io::cerr.PrintLn("Invalid indexStride: ", indexStride);
				return false;
			}
			cur += align(length, 4);
			return true;
		}
	};

	// Describes a material
	struct Mat0 {
		static constexpr i64 SIZE = 64;
		char ident[4];
		vec4 albedoColor;
		vec3 emissionColor;
		static_assert(sizeof(az::vec4) == 4*4);
		static_assert(sizeof(az::vec3) == 4*3);
		f32 normalDepth;
		f32 metalnessFactor;
		f32 roughnessFactor;
		// Texture indices are valid within the file
		// 0 indicates no texture
		u32 albedoIndex;
		u32 emissionIndex;
		u32 normalIndex;
		u32 metalnessIndex;
		u32 roughnessIndex;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Mat0", 4);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			return true;
		}
	};
	
	// Contains an entire image file
	struct ImageData {
		Name filename;
		u32 length;
		u32 isLinear;
		// char data[length];
		Str data;
		bool FromBuffer(Str buffer, i64 &cur) {
			if (!filename.FromBuffer(buffer, cur)) return false;
			EXPECT_SPACE_IN_BUFFER(4);
			memcpy(&length, &buffer[cur], 8);
			cur += 8;
			EXPECT_SPACE_IN_BUFFER(length);
			data = Str(&buffer[cur], length);
			cur += align(length, 4);
			return true;
		}
	};

} // namespace Types

namespace Tables {

	struct Mesh {
		Headers::Table header;
		Types::Name name;
		Types::Vert vert;
		Types::Indx indx;
		Types::Mat0 mat0;
		// Expects header to already be set and cur to be immediately after
		bool FromBuffer(Str buffer, i64 &cur, u32 vertexStride, u32 indexStride) {
			if (strncmp(header.ident, "Mesh", 4) != 0) {
				PRINT_ERROR("Parsing a \"", Str(header.ident, 4), "\" as though it's a \"Mesh\"");
				return false;
			}
			EXPECT_SPACE_IN_BUFFER(header.length - Headers::Table::SIZE);
			i64 endCur = cur + header.length - Headers::Table::SIZE;
			bool hasName = false;
			bool hasVert = false;
			bool hasIndx = false;
			bool hasMat0 = false;
			bool skippedToEnd = false;
			while (cur < endCur) {
				i64 spaceLeft = endCur - cur;
				Str tag = Str(&buffer[cur], 4);
				if (tag == "Name") {
					if (!name.FromBuffer(buffer, cur)) return false;
					hasName = true;
				} else if (tag == "Vert") {
					if (!vert.FromBuffer(buffer, cur, vertexStride)) return false;
					hasVert = true;
				} else if (tag == "Indx") {
					if (!indx.FromBuffer(buffer, cur, indexStride)) return false;
					hasIndx = true;
				} else if (tag == "Mat0") {
					if (!mat0.FromBuffer(buffer, cur)) return false;
					hasMat0 = true;
				} else {
					io::cout.PrintLn("Unknown tag \"", tag, "\" in Mesh. Skipping to the end...");
					cur = endCur;
					skippedToEnd = true;
				}
			}
			if (!hasName) {
				PRINT_ERROR("\"Mesh\" has no \"Name\"", skippedToEnd ? "... was it skipped?" : "");
				return false;
			}
			if (!hasVert) {
				PRINT_ERROR("\"Mesh\" has no \"Vert\"", skippedToEnd ? "... was it skipped?" : "");
				return false;
			}
			if (!hasIndx) {
				PRINT_ERROR("\"Mesh\" has no \"Indx\"", skippedToEnd ? "... was it skipped?" : "");
				return false;
			}
			if (!hasMat0) {
				PRINT_ERROR("\"Mesh\" has no \"Mat0\"", skippedToEnd ? "... was it skipped?" : "");
				return false;
			}
			return true;
		}
	};

	struct Empt {
		Headers::Table header;
		Types::Name name;
		az::vec3 pos;
		az::vec3 eulerAngles;
		static_assert(sizeof(az::vec3) == 4*3);
		// Expects header to already be set and cur to be immediately after
		bool FromBuffer(Str buffer, i64 &cur) {
			if (strncmp(header.ident, "Empt", 4) != 0) {
				PRINT_ERROR("Parsing a \"", Str(header.ident, 4), "\" as though it's a \"Empt\"");
				return false;
			}
			EXPECT_SPACE_IN_BUFFER(header.length - Headers::Table::SIZE);
			i64 endCur = cur + header.length - Headers::Table::SIZE;
			if (!name.FromBuffer(buffer, cur)) return false;
			if (header.length < Headers::Table::SIZE + Types::Name::SIZE + name.length + 4*6) {
				PRINT_ERROR("\"Empt\" length (", header.length, ") is too short!");
				return false;
			}
			memcpy(&pos, &buffer[cur], 4*6);
			cur += 4*6;
			if (cur < endCur) {
				io::cout.PrintLn("Skipping ", endCur - cur, " bytes in \"Empt\"");
			}
			cur = endCur;
			return true;
		}
	};
	
	struct Imgs {
		Headers::Table header;
		u32 count;
		// Types::ImageData files[count];
		Array<Types::ImageData> files;
		// Expects header to already be set and cur to be immediately after
		bool FromBuffer(Str buffer, i64 &cur) {
			i64 startCur = cur - Headers::Table::SIZE;
			if (strncmp(header.ident, "Imgs", 4) != 0) {
				PRINT_ERROR("Parsing a \"", Str(header.ident, 4), "\" as though it's a \"Imgs\"");
				return false;
			}
			EXPECT_SPACE_IN_BUFFER(header.length - Headers::Table::SIZE);
			i64 endCur = cur + align(header.length, 4) - Headers::Table::SIZE;
			EXPECT_SPACE_IN_BUFFER(4);
			memcpy(&count, &buffer[cur], 4);
			cur += 4;
			files.Resize(count);
			for (u32 i = 0; i < count; i++) {
				if (!files[i].FromBuffer(buffer, cur)) return false;
			}
			if (cur > endCur) {
				io::cerr.PrintLn("\"Imgs\" had more data (", cur - startCur, " bytes) than expected (", header.length, " bytes reported in header)");
				return false;
			} else if (cur < endCur) {
				io::cout.PrintLn("Skipping ", endCur - cur, " bytes in \"Imgs\"");
			}
			cur = endCur;
			return true;
		}
	};

} // namespace Tables

bool File::Load(Str filepath) {
	Array<char> buffer = FileContents(filepath, true);
	if (buffer.size == 0) {
		io::cerr.PrintLn("Failed to open '", filepath, "'");
		return false;
	}
	return LoadFromBuffer(buffer);
}

bool File::LoadFromBuffer(Str buffer) {
	i64 cur = 0;
	Headers::File header;
	if (!header.FromBuffer(buffer, cur)) return false;
	if (header.versionMajor > 1 || header.versionMinor > 0) {
		io::cout.PrintLn("Az3DObj version ", header.versionMajor, ".", header.versionMinor, " is newer than our importer (version 1.0). Some features may not be available.");
	}
	u32 numTexturesExpected = 0;
	u32 numTexturesActual = 0;
	for (; cur < buffer.size;) {
		Headers::Table table;
		if (!table.FromBuffer(buffer, cur)) return false;
		i64 endCur = cur + table.length - Headers::Table::SIZE;
		if (table.length <= Headers::Table::SIZE) {
			io::cerr.PrintLn("Header length invalid (", table.length, ")");
			return false;
		}
		Str tag = Str(table.ident, 4);
		if (tag == "Mesh") {
			Tables::Mesh meshData;
			meshData.header = table;
			if (!meshData.FromBuffer(buffer, cur, header.vertexStride, header.indexStride)) return false;
			Mesh &mesh = meshes.Append(Mesh());
			mesh.name = meshData.name.name;
			mesh.vertices = std::move(meshData.vert.vertices);
			mesh.indices = std::move(meshData.indx.indices);
			mesh.material.color = meshData.mat0.albedoColor;
			mesh.material.emit = meshData.mat0.emissionColor;
			mesh.material.normal = meshData.mat0.normalDepth;
			mesh.material.metalness = meshData.mat0.metalnessFactor;
			mesh.material.roughness = meshData.mat0.roughnessFactor;
			mesh.material.texAlbedo = meshData.mat0.albedoIndex;
			mesh.material.texEmit = meshData.mat0.emissionIndex;
			mesh.material.texNormal = meshData.mat0.normalIndex;
			mesh.material.texMetalness = meshData.mat0.metalnessIndex;
			mesh.material.texRoughness = meshData.mat0.roughnessIndex;
			for (i32 i = 0; i < 5; i++) {
				if (mesh.material.tex[i] > numTexturesExpected) numTexturesExpected = mesh.material.tex[i];
			}
		} else if (tag == "Empt") {
			Tables::Empt emptyData;
			emptyData.header = table;
			if (!emptyData.FromBuffer(buffer, cur)) return false;
			Empty &empty = empties.Append(Empty());
			empty.name = emptyData.name.name;
			empty.pos = emptyData.pos;
			empty.eulerAngles = emptyData.eulerAngles;
		} else if (tag == "Imgs") {
			Tables::Imgs imagesData;
			imagesData.header = table;
			if (!imagesData.FromBuffer(buffer, cur)) return false;
			for (u32 i = 0; i < imagesData.count; i++) {
				Image &image = images.Append(Image());
				if (!image.LoadFromBuffer(imagesData.files[i].data)) {
					io::cerr.PrintLn("Failed to decode image data for \"", imagesData.files[i].filename.name, "\" embedded in Az3DObj.");
					return false;
				}
				image.colorSpace = imagesData.files[i].isLinear ? Image::LINEAR : Image::SRGB;
			}
			numTexturesActual += imagesData.count;
		} else {
			io::cout.PrintLn("Ignoring unsupported table '", tag, "'");
			cur = endCur;
		}
		if (cur > endCur) {
			io::cerr.PrintLn("We exceeded the limits of '", tag, "' table by ", cur - endCur, " bytes!");
			return false;
		} else if (cur < endCur) {
			io::cout.PrintLn("There seems to be some trailing information in '", tag, "' table of ", endCur - cur, " bytes. Skipping...");
			cur = endCur;
		}
	}
	if (numTexturesActual != numTexturesExpected) {
		io::cerr.PrintLn("Materials expected ", numTexturesExpected, " textures, but we actually had ", numTexturesActual);
		return false;
	}
	return true;
}

} // namespace Az3D::Az3DObj