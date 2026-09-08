/*
	File: Any.hpp
	Author: Philip Haynes
	A container that can represent any type in a safe way. Also guarantees that the raw pointer to the contained type doesn't move.
*/

#ifndef AZCORE_ANY_HPP
#define AZCORE_ANY_HPP

#include "None.hpp"
#include "../Assert.hpp"
#include "../Utility/TypeHash.hpp"
#include "../Utility/RAIIHacks.hpp"
#include "../Utility/TypeName.hpp"

#include <type_traits>
#include <utility> // std::move

namespace AzCore {

struct Any {
	enum class Action { COPY, MOVE, DELETE };
	typedef void (*fp_Actions)(void **pDst, void **pSrc, Action action);
	void *data = nullptr;
	fp_Actions actions = nullptr;
	u32 typeHash = 0;
	Any() = default;
	constexpr Any(None_t) {}

	template<typename T>
	static constexpr bool MustBeAlloc() {
		return sizeof(T) > sizeof(data);
	}

	inline Any(Any &&other) : actions(other.actions), typeHash(other.typeHash) {
		if (actions) {
			actions(&data, &other.data, Action::MOVE);
		}
		other.actions = nullptr;
		other.typeHash = 0;
	}

	// We need to catch this because otherwise it will go to the template constructor down below
	inline Any(Any &other) : Any(const_cast<const Any&>(other)) {}

	inline Any(const Any &other) : actions(other.actions) {
		if (actions) {
			actions(&data, const_cast<void**>(&other.data), Action::COPY);
			typeHash = other.typeHash;
		}
	}

	inline ~Any() {
		if (actions) {
			actions(&data, nullptr, Action::DELETE);
		}
	}

	template <typename T>
	static fp_Actions MakeActions() {
		return [](void **pDst, void **pSrc, Action action) {
			switch (action) {
				case Action::COPY: {
					if constexpr (std::is_copy_constructible_v<T>) {
						if constexpr (MustBeAlloc<T>()) {
							*pDst = new T(*(const T*)*pSrc);
						} else {
							AzPlacementNew(*((T*)pDst), *((const T*)pSrc));
						}
					} else {
						AzAssert(false, Stringify(TypeName<T>(), " is not copy constructible!"));
					}
				} break;
				case Action::MOVE: {
					if constexpr (std::is_move_constructible_v<T>) {
						if constexpr (MustBeAlloc<T>()) {
							*pDst = new T(std::move(*(T*)*pSrc));
						} else {
							AzPlacementNew(*((T*)pDst), std::move(*((T*)pSrc)));
						}
					} else {
						AzAssert(false, Stringify(TypeName<T>(), " is not move constructible!"));
					}
				} break;
				case Action::DELETE: {
					if constexpr (MustBeAlloc<T>()) {
						delete (T*)*pDst;
					} else {
						(*((T*)pDst)).~T(); // Did you get that?
					}
				} break;
			}
		};
	}

	template <typename T>
	Any(T &&value) : actions(MakeActions<T>()), typeHash(TypeHash<T>()) {
		if constexpr (MustBeAlloc<T>()) {
			data = new T(std::move(value));
		} else {
			AzPlacementNew(*((T*)&data), std::move(value));
		}
	}

	template <typename T>
	Any(const T &value) : actions(MakeActions<T>()), typeHash(TypeHash<T>()) {
		if constexpr (MustBeAlloc<T>()) {
			data = new T(value);
		} else {
			AzPlacementNew(*((T*)&data), value);
		}
	}

	Any& operator=(Any &&other) {
		if (typeHash != other.typeHash) {
			if (actions) {
				actions(&data, nullptr, Action::DELETE);
			}
			actions = other.actions;
			typeHash = other.typeHash;
		}
		if (actions) {
			actions(&data, &other.data, Action::MOVE);
		}
		other.actions = nullptr;
		other.typeHash = 0;
		return *this;
	}

	Any& operator=(Any &other) {
		return (*this = const_cast<const Any&>(other));
	}

	Any& operator=(const Any &other) {
		if (typeHash != other.typeHash) {
			if (actions) {
				actions(&data, nullptr, Action::DELETE);
			}
			actions = other.actions;
			typeHash = other.typeHash;
		}
		if (actions) {
			actions(&data, const_cast<void**>(&other.data), Action::COPY);
		}
		return *this;
	}

	template <typename T>
	Any& operator=(T &&value) {
		if constexpr (MustBeAlloc<T>()) {
			if (actions) {
				if (IsType<T>()) {
					*std::launder((T*)data) = std::move(value);
					return *this;
				} else {
					actions(&data, nullptr, Action::DELETE);
				}
			}
			data = new T(std::move(value));
		} else {
			if (actions) {
				if (IsType<T>()) {
					*std::launder((T*)&data) = std::move(value);
					return *this;
				} else {
					actions(&data, nullptr, Action::DELETE);
				}
			}
			*std::launder((T*)&data) = std::move(value);
		}
		actions = MakeActions<T>();
		typeHash = TypeHash<T>();
		return *this;
	}

	template <typename T>
	Any& operator=(const T &value) {
		if constexpr (MustBeAlloc<T>()) {
			if (actions) {
				if (IsType<T>()) {
					*std::launder((T*)data) = value;
					return *this;
				} else {
					actions(&data, nullptr, Action::DELETE);
				}
			}
			data = new T(value);
		} else {
			if (actions) {
				if (IsType<T>()) {
					*std::launder((T*)&data) = value;
					return *this;
				} else {
					actions(&data, nullptr, Action::DELETE);
				}
			}
			*std::launder((T*)&data) = value;
		}
		actions = MakeActions<T>();
		typeHash = TypeHash<T>();
		return *this;
	}

	// Begin treating the stored type as the given type. There are no guardrails for this, so do it at your own peril.
	template<typename T>
	void Reinterpret() {
		actions = MakeActions<T>();
		typeHash = TypeHash<T>();
	}

	template <typename T>
	T& Get() {
		AzAssert(nullptr != actions, "Trying to Get() data that doesn't exist");
		AzAssert(IsType<T>(), "Trying to Get() with a type that doesn't match what we are");
		if constexpr (MustBeAlloc<T>()) {
			return *std::launder((T*)data);
		} else {
			return *std::launder((T*)&data);
		}
	}

	template <typename T>
	const T& Get() const {
		AzAssert(nullptr != actions, "Trying to Get() data that doesn't exist");
		AzAssert(IsType<T>(), "Trying to Get() with a type that doesn't match what we are");
		if constexpr (MustBeAlloc<T>()) {
			return *std::launder((const T*)data);
		} else {
			return *std::launder((const T*)&data);
		}
	}

	template <typename T>
	bool IsType() const {
		return typeHash == TypeHash<T>();
	}

	// NOTE: Only use this when you don't know or care what the type is. IsType<T>() will only return true if this is also true, so that should be preferred when the expected type is known.
	inline bool IsSomething() const {
		return actions != nullptr;
	}
};

} // namespace AzCore

#endif // AZCORE_ANY_HPP