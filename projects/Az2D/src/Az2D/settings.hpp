/*
	File: settings.hpp
	Author: Philip Haynes
	How we save, load, and access game settings.
*/

#ifndef AZ2D_SETTINGS_HPP
#define AZ2D_SETTINGS_HPP

#include "AzCore/Memory/StringArena.hpp"

namespace Az2D::Settings {

AZCORE_CREATE_STRING_ARENA_HPP()

using Name = Settings::AString;

class Setting {
	union {
		bool val_bool;
		struct {
			i64 val;
			i64 val_min;
			i64 val_max;
		} val_int;
		struct {
			f64 val;
			f64 val_min;
			f64 val_max;
		} val_real;
		az::String val_string;
	};
public:
	enum class Type {
		NONE,
		BOOL,
		INT,
		REAL,
		STRING,
	} type;
	static const char *typeStrings[5];
	inline Setting() : type(Type::NONE) {}
	inline Setting(bool val) : type(Type::BOOL), val_bool(val) {}
	inline Setting(i64 val, i64 val_min, i64 val_max) : type(Type::INT), val_int{val, val_min, val_max} {}
	inline Setting(f64 val, f64 val_min, f64 val_max) : type(Type::REAL), val_real{val, val_min, val_max} {}
	inline Setting(az::String val) : type(Type::STRING), val_string(val) {}
	inline ~Setting() {
		if (type == Type::STRING) {
			val_string.az::String::~String();
		}
	}
	inline Setting(const Setting &other) : type(other.type) {
		switch (type) {
			case Type::BOOL: {
				val_bool = other.val_bool;
			} break;
			case Type::INT: {
				val_int = other.val_int;
			} break;
			case Type::REAL: {
				val_real = other.val_real;
			} break;
			case Type::STRING: {
				new(&val_string) az::String(other.val_string);
			} break;
			default: break;
		}
	}
	inline Setting(Setting &&other) : type(other.type) {
		switch (type) {
			case Type::BOOL: {
				val_bool = other.val_bool;
			} break;
			case Type::INT: {
				val_int = other.val_int;
			} break;
			case Type::REAL: {
				val_real = other.val_real;
			} break;
			case Type::STRING: {
				new(&val_string) az::String(std::move(other.val_string));
			} break;
			default: break;
		}
	}
	inline Setting& operator=(const Setting &other) {
		if (type == Type::STRING) {
			if (other.type == Type::STRING) {
				val_string = other.val_string;
				return *this;
			} else {
				val_string.az::String::~String();
			}
		}
		type = other.type;
		switch (type) {
			case Type::BOOL: {
				val_bool = other.val_bool;
			} break;
			case Type::INT: {
				val_int = other.val_int;
			} break;
			case Type::REAL: {
				val_real = other.val_real;
			} break;
			case Type::STRING: {
				new(&val_string) az::String(other.val_string);
			} break;
			default: break;
		}
		return *this;
	}
	inline Setting& operator=(Setting &&other) {
		if (type == Type::STRING) {
			if (other.type == Type::STRING) {
				val_string = std::move(other.val_string);
				return *this;
			} else {
				val_string.az::String::~String();
			}
		}
		type = other.type;
		switch (type) {
			case Type::BOOL: {
				val_bool = other.val_bool;
			} break;
			case Type::INT: {
				val_int = other.val_int;
			} break;
			case Type::REAL: {
				val_real = other.val_real;
			} break;
			case Type::STRING: {
				new(&val_string) az::String(std::move(other.val_string));
			} break;
			default: break;
		}
		return *this;
	}
	inline Setting& operator=(bool val) {
		if (type == Type::NONE) type = Type::BOOL;
		AzAssert(type == Type::BOOL, az::Stringify("Cannot assign a bool to a Setting of type \"", typeStrings[(u32)type], "\""));
		val_bool = val;
		return *this;
	}
	inline Setting& operator=(i64 val) {
		// if (type == Type::NONE) type = Type::INT;
		AzAssert(type == Type::INT, az::Stringify("Cannot assign an int to a Setting of type \"", typeStrings[(u32)type], "\""));
		val_int.val = clamp(val, val_int.val_min, val_int.val_max);
		return *this;
	}
	inline Setting& operator=(f64 val) {
		// if (type == Type::NONE) type = Type::REAL;
		AzAssert(type == Type::REAL, az::Stringify("Cannot assign a real to a Setting of type \"", typeStrings[(u32)type], "\""));
		val_real.val = clamp(val, val_real.val_min, val_real.val_max);
		return *this;
	}
	inline Setting& operator=(az::String val) {
		if (type == Type::NONE) {
			new(&val_string) az::String();
			type = Type::STRING;
		}
		AzAssert(type == Type::STRING, az::Stringify("Cannot assign a string to a Setting of type \"", typeStrings[(u32)type], "\""));
		val_string = val;
		return *this;
	}
	inline bool GetBool() const {
		AzAssert(type == Type::BOOL, az::Stringify("Cannot GetBool from a Setting of type \"", typeStrings[(u32)type], "\""));
		return val_bool;
	}
	inline i64 GetInt() const {
		AzAssert(type == Type::INT, az::Stringify("Cannot GetInt from a Setting of type \"", typeStrings[(u32)type], "\""));
		return val_int.val;
	}
	inline f64 GetReal() const {
		AzAssert(type == Type::REAL, az::Stringify("Cannot GetReal from a Setting of type \"", typeStrings[(u32)type], "\""));
		return val_real.val;
	}
	inline const az::String& GetString() const {
		AzAssert(type == Type::STRING, az::Stringify("Cannot GetString from a Setting of type \"", typeStrings[(u32)type], "\""));
		return val_string;
	}
};

extern Name sFullscreen;
extern Name sVSync;
extern Name sDebugInfo;
extern Name sFramerate;
extern Name sVolumeMain;
extern Name sVolumeMusic;
extern Name sVolumeEffects;
extern Name sLocaleOverride;

void Add(Name name, Setting &&defaultValue);

bool ReadBool(Name name);
i64 ReadInt(Name name);
f64 ReadReal(Name name);
az::String ReadString(Name name);

void SetBool(Name name, bool value);
void SetInt(Name name, i64 value);
void SetReal(Name name,  f64 value);
void SetString(Name name, const az::String &value);
void SetString(Name name, az::String &&value);

bool Load();
bool Save();

} // namespace Az2D::Settings

#endif // AZ2D_SETTINGS_HPP
