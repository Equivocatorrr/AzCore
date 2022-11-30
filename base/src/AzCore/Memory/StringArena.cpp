#include "StringArena.hpp"

namespace AzCore {
	
StringArena genericStringArena;

i32 StringArena::GetID(const String& string) {
	i32 result = map.ValueOf(string, strings.size);
	if (result == strings.size) {
		strings.Append(string);
	}
	return result;
}

i32 StringArena::GetID(String &&string) {
	i32 result = map.ValueOf(string, strings.size);
	if (result == strings.size) {
		strings.Append(std::move(string));
	}
	return result;
}

} // namespace AzCore