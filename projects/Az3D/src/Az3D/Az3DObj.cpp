/*
	File: Az3DObj.cpp
	Author: Philip Haynes
*/

#include "Az3DObj.hpp"
#include "AzCore/IO/Log.hpp"
#include "AzCore/Memory/Endian.hpp"

// NOTE: File is little endian, and current target platform is little endian.
// If this ever changes in the future we can do endian stuff. For now we do the easy thing.

namespace Az3D::Az3DObj {

using namespace AzCore;

constexpr Str AZ3D_MAGIC = Str("Az3DObj\0", 8);

constexpr u16 VERSION_MAJOR = 1;
constexpr u16 VERSION_MINOR = 1;

// 1st argument must be a "string literal", __VA_ARGS__ breaks somewhere when I add the comma.
#define PRINT_ERROR(...) io::cerr.PrintLn(__FUNCTION__, " error: " __VA_ARGS__)

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
		static constexpr i64 SIZE = 12;
		char magic[8];
		u16 versionMajor;
		u16 versionMinor;
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
		static constexpr i64 SIZE = 12;
		char ident[4];
		u32 count;
		u16 stride;
		// componentCount determines how many components are in the format string
		u16 componentCount;
		// format describes each component's purpose, type, and size in 4 bytes each
		// u8 format[componentCount*4];
		Str format;
		// u8 vertexBuffer[count*stride];
		Array<Vertex> vertices;
		struct SrcScalar {
			enum Kind {
				F32,
				S16,
				S8,
			} kind;
			// Stride of a single scalar in bytes
			i32 stride;
			// All values are given by: src/dimension + offset
			f32 dimension;
			f32 offset;
			// Offset into the Vertex struct in f32s.
			i32 dstOffset;
		};
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Vert", 4);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			Array<SrcScalar> srcScalars;
			if (!ParseFormat(buffer, cur, componentCount, stride, srcScalars)) return false;
			i64 length = count * stride;
			EXPECT_SPACE_IN_BUFFER(length);
			vertices.Resize(count);
			for (i64 i = 0; i < (i64)count; i++) {
				vertices[i] = GetVertex(&buffer[cur], srcScalars);
				cur += stride;
			}
			return true;
		}
		// Fills an array of offsets into our Vertex struct for each 4-byte chunk of data
		// The index into this array increases by one for every component in the input buffer
		bool ParseFormat(Str buffer, i64 &cur, i32 numComponents, u32 stride, Array<SrcScalar> &dst) {
			i32 totalSize = 0;
			for (i32 i = 0; i < numComponents; i++) {
				EXPECT_SPACE_IN_BUFFER(4);
				Str tag = buffer.SubRange(cur, 2);
				char type = buffer[cur + 2];
				u8 count = buffer[cur + 3];
				cur += 4;
				ArrayWithBucket<f32, 4> dimension, srcOffset;
				bool hasDimensionAndSrcOffset;
				SrcScalar::Kind kind;
				i32 scalarStride;
				switch (type) {
					case 'F':
						kind = SrcScalar::F32;
						dimension = ArrayWithBucket<f32, 4>((i32)count, 1.0f);
						srcOffset = ArrayWithBucket<f32, 4>((i32)count, 0.0f);
						hasDimensionAndSrcOffset = false;
						scalarStride = 4;
						break;
					case 'S':
						kind = SrcScalar::S16;
						hasDimensionAndSrcOffset = true;
						scalarStride = 2;
						break;
					case 'B':
						kind = SrcScalar::S8;
						hasDimensionAndSrcOffset = true;
						scalarStride = 1;
						break;
					default:
						io::cerr.PrintLn("Vert Component ", i, " with tag \"", tag, "\" has an invalid scalar type: '", type, "'");
						return false;
				}
				totalSize += scalarStride * count;
				if (hasDimensionAndSrcOffset) {
					EXPECT_SPACE_IN_BUFFER(8*count);
					for (i32 j = 0; j < count; j++) {
						dimension.Append(*(f32*)&buffer[cur]);
						cur += 4;
						srcOffset.Append(*(f32*)&buffer[cur]);
						cur += 4;
					}
				}
				i32 dstOffset;
				i32 maxInDst;
				if (tag == "Po") {
					dstOffset = 0;
					maxInDst = 3;
				} else if (tag == "No") {
					dstOffset = 3;
					maxInDst = 3;
				} else if (tag == "Ta") {
					dstOffset = 6;
					maxInDst = 3;
				} else if (tag == "UV") {
					dstOffset = 9;
					maxInDst = 2;
				} else {
					maxInDst = 0;
				}
				for (i32 j = 0; j < count; j++) {
					SrcScalar scalar;
					scalar.kind = kind;
					scalar.stride = scalarStride;
					scalar.dimension = dimension[j];
					scalar.offset = srcOffset[j];
					scalar.dstOffset = j < maxInDst ? dstOffset + j : -1;
					dst.Append(scalar);
				}
			}
			if (totalSize != stride) {
				io::cerr.PrintLn("Vert Format string describes a Vertex with a stride of ", totalSize, " when it was supposed to have a stride of ", stride);
				return false;
			}
			return true;
		}
		
		Vertex GetVertex(char *buffer, Array<SrcScalar> srcScalars) {
			Vertex result;
			// Default values
			result.pos = vec3(0.0f);
			result.normal = vec3(0.0f, 0.0f, 1.0f);
			result.tangent = vec3(1.0f, 0.0f, 0.0f);
			result.tex = vec2(0.0f);
			f32 *dst = (f32*)&result;
			u8 *src = (u8*)buffer;
			for (SrcScalar srcScalar : srcScalars) {
				AzAssert(srcScalar.dstOffset < sizeof(Vertex) / sizeof(f32), "dstOffset is out of bounds");
				if (srcScalar.dstOffset >= 0) {
					switch (srcScalar.kind) {
						case SrcScalar::F32:
							dst[srcScalar.dstOffset] = *(f32*)src * srcScalar.dimension + srcScalar.offset;
							break;
						case SrcScalar::S16:
							dst[srcScalar.dstOffset] = f32(*(i16*)src) * srcScalar.dimension / 32767.0f + srcScalar.offset;
							break;
						case SrcScalar::S8:
							dst[srcScalar.dstOffset] = f32(*(i8*)src) * srcScalar.dimension / 127.0f + srcScalar.offset;
							break;
						default: AzAssert(false, "Unreachable"); break;
					}
				}
				src += srcScalar.stride;
			}
			// This is mostly to fix our default value not relating to anything in particular, but if for whatever reason the file's tangent isn't orthogonal to the normal, we still ensure that.
			result.tangent = orthogonalize(result.tangent, result.normal);
			return result;
		}
	};
	
	struct Indx {
		static constexpr i64 SIZE = 8;
		char ident[4];
		u32 count;
		// u8 indices[count]; when count < 256
		// u16 indices[count]; when count < 65536
		// u32 indices[count]; otherwise
		// implicit from count
		u32 stride;
		Array<u32> indices;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Indx", 4);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			if (count < 0x100) {
				stride = 1;
			} else if (count < 0x10000) {
				stride = 2;
			} else {
				stride = 4;
			}
			i64 length = count * stride;
			EXPECT_SPACE_IN_BUFFER(length);
			indices.Resize(count);
			switch (stride) {
			case 1:
				for (i64 i = 0; i < count; i++) {
					indices[i] = *(u8*)&buffer[cur + i*stride];
				} break;
			case 2:
				for (i64 i = 0; i < count; i++) {
					indices[i] = *(u16*)&buffer[cur + i*stride];
				} break;
			case 4:
				memcpy(indices.data, &buffer[cur], length);
				break;
			}
			cur += align(length, 4);
			return true;
		}
	};

	// Describes a material
	struct Mat0 {
		static constexpr i64 SIZE = 8;
		char ident[4];
		u32 length;
		vec4 albedoColor = vec4(1.0f);
		vec3 emissionColor = vec3(0.0f);
		static_assert(sizeof(az::vec4) == 4*4);
		static_assert(sizeof(az::vec3) == 4*3);
		f32 normalDepth = 1.0f;
		f32 metalnessFactor = 0.0f;
		f32 roughnessFactor = 0.5f;
		// SubSurface Scattering
		f32 sssFactor = 0.0f;
		// Tints the entire subsurf lighting component evenly
		vec3 sssColor = vec3(1.0f);
		// sssRadius has one radius per primary color
		// Tints the subsurf lighting component depending on distance
		vec3 sssRadius = vec3(0.1f);
		// Texture indices are valid within the file
		// 0 indicates no texture
		u32 albedoIndex = 0;
		u32 emissionIndex = 0;
		u32 normalIndex = 0;
		u32 metalnessIndex = 0;
		u32 roughnessIndex = 0;
		// Texture that describes subsurface color. Probably won't really be used since you could just have separate materials for subsurf vs not, even in the same Mesh.
		u32 sssIndex = 0;
		bool isFoliage = false;
		bool FromBuffer(Str buffer, i64 &cur) {
			EXPECT_SPACE_IN_BUFFER(SIZE);
			EXPECT_TAG_IN_BUFFER("Mat0", 4);
			memcpy(this, &buffer[cur], SIZE);
			cur += SIZE;
			EXPECT_SPACE_IN_BUFFER(length);
			i64 endCur = cur + length;
			while (cur < endCur) {
				Str tag = Str(&buffer[cur], 4);
				cur += 4;
				u32 count = (u32)tag[3];
				EXPECT_SPACE_IN_BUFFER(4*count);
				if (tag == "ACF\004") {
					albedoColor = *(vec4*)&buffer[cur];
				} else if (tag == "ECF\003") {
					emissionColor = *(vec3*)&buffer[cur];
				} else if (tag == "NDF\001") {
					normalDepth = *(f32*)&buffer[cur];
				} else if (tag == "MFF\001") {
					metalnessFactor = *(f32*)&buffer[cur];
				} else if (tag == "RFF\001") {
					roughnessFactor = *(f32*)&buffer[cur];
				} else if (tag == "SFF\001") {
					sssFactor = *(f32*)&buffer[cur];
				} else if (tag == "SCF\003") {
					sssColor = *(vec3*)&buffer[cur];
				} else if (tag == "SRF\003") {
					sssRadius = *(vec3*)&buffer[cur];
				} else if (tag == "ATI\001") {
					albedoIndex = *(u32*)&buffer[cur];
				} else if (tag == "ETI\001") {
					emissionIndex = *(u32*)&buffer[cur];
				} else if (tag == "NTI\001") {
					normalIndex = *(u32*)&buffer[cur];
				} else if (tag == "MTI\001") {
					metalnessIndex = *(u32*)&buffer[cur];
				} else if (tag == "RTI\001") {
					roughnessIndex = *(u32*)&buffer[cur];
				} else if (tag == "STI\001") {
					sssIndex = *(u32*)&buffer[cur];
				} else if (tag == "Fol\0") {
					isFoliage = true;
				}
				cur += 4*count;
			}
			if (cur != endCur) {
				io::cerr.PrintLn("Mat0 data is misaligned somehow (cur is ", cur, " but expected ", endCur, ")");
				return false;
			}
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
			EXPECT_SPACE_IN_BUFFER(8);
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
		bool FromBuffer(Str buffer, i64 &cur) {
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
					if (!vert.FromBuffer(buffer, cur)) return false;
					hasVert = true;
				} else if (tag == "Indx") {
					if (!indx.FromBuffer(buffer, cur)) return false;
					hasIndx = true;
				} else if (tag == "Mat0") {
					if (!mat0.FromBuffer(buffer, cur)) return false;
					hasMat0 = true;
				} else {
					io::cout.PrintLn("Unknown tag \"", tag, "\" in Mesh. Skipping to the end...");
					cur = endCur;
					skippedToEnd = true;
				}
				cur = align(cur, 4);
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
	if (header.versionMajor > VERSION_MAJOR || header.versionMinor > VERSION_MINOR) {
		io::cout.PrintLn("Az3DObj version ", header.versionMajor, ".", header.versionMinor, " is newer than our importer (version ", VERSION_MAJOR, ".", VERSION_MINOR, "). Some features may not be available.");
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
			if (!meshData.FromBuffer(buffer, cur)) return false;
			Mesh &mesh = meshes.Append(Mesh());
			mesh.name = meshData.name.name;
			mesh.vertices = std::move(meshData.vert.vertices);
			mesh.indices = std::move(meshData.indx.indices);
			mesh.material.color = meshData.mat0.albedoColor;
			mesh.material.emit = meshData.mat0.emissionColor;
			mesh.material.normal = meshData.mat0.normalDepth;
			mesh.material.metalness = meshData.mat0.metalnessFactor;
			mesh.material.roughness = meshData.mat0.roughnessFactor;
			mesh.material.sssFactor = meshData.mat0.sssFactor;
			mesh.material.sssColor = meshData.mat0.sssColor;
			mesh.material.sssRadius = meshData.mat0.sssRadius;
			mesh.material.texAlbedo = meshData.mat0.albedoIndex;
			mesh.material.texEmit = meshData.mat0.emissionIndex;
			mesh.material.texNormal = meshData.mat0.normalIndex;
			mesh.material.texMetalness = meshData.mat0.metalnessIndex;
			mesh.material.texRoughness = meshData.mat0.roughnessIndex;
			mesh.material.isFoliage = meshData.mat0.isFoliage;
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
		cur = align(cur, 4);
	}
	if (numTexturesActual != numTexturesExpected) {
		io::cerr.PrintLn("Materials expected ", numTexturesExpected, " textures, but we actually had ", numTexturesActual);
		return false;
	}
	io::cout.PrintLn("Had ", meshes.size, " meshes, ", empties.size, " empties, ", images.size, " images.");
	return true;
}

} // namespace Az3D::Az3DObj