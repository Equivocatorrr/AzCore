/*
	File: None.hpp
	Author: Philip Haynes
	A cute and concise way to specify something should be empty.
	As of this comment, is used by Any and Optional.
*/

#ifndef AZCORE_NONE_HPP
#define AZCORE_NONE_HPP

namespace AzCore {

struct None_t {};
// Can be implicitly converted to Any and Optional types to construct an empty state.
constexpr None_t None;

// Wow so cute

} // namespace AzCore

#endif // AZCORE_NONE_HPP