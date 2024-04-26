/*
	File: TemplateUtil.hpp
	Author: Philip Haynes
	Because I can't wait for C++ 20, but also don't want to actually switch from C++ 17
*/

#ifndef AZCORE_TEMPLATE_UTIL_HPP
#define AZCORE_TEMPLATE_UTIL_HPP

#include <type_traits>

namespace AzCore {

template<typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

} // namespace AzCore

#endif // AZCORE_TEMPLATE_UTIL_HPP