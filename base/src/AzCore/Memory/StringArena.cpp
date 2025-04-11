#include "StringArena.hpp"

namespace AzCore {

StringArena genericStringArena;

Array<char>& StringArena::ReserveMemory(Range<char> string) {
	AzAssert(string.size < PAGE_SIZE, Stringify("StringArena was given a large string (", string.size, " bytes)!"));
	Array<char> *lastPage = &arenas.Back();
	if (PAGE_SIZE - lastPage->size <= string.size) {
		lastPage = &arenas.Append(Array<char>());
		lastPage->Reserve(PAGE_SIZE);
	}
	return *lastPage;
}

StringArena::StringArena(i32 pageSize) {
	PAGE_SIZE = pageSize;
	Array<char> &page = arenas.Append(Array<char>());
	page.Reserve(PAGE_SIZE);
	page.Append('\0');
	map[Range<char>()] = strings.size;
	strings.Append(Range<char>(page.data, 0));
}

i32 StringArena::GetID(const Range<char> string) {
	ScopedLock lock(mutex);
	i32 result = map.ValueOf(string, strings.size);
	if (result == strings.size) {
		Array<char> &page = ReserveMemory(string);
		// We need to make a version that points to OUR memory.
		Range<char> newString(page.data+page.size, string.size);
		page.Append(string);
		page.Append('\0');
		strings.Append(newString);
	}
	return result;
}

} // namespace AzCore