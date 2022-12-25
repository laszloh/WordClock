#pragma once

#include <array>
#include <type_traits>

namespace std {

#if __cplusplus <= 201703L

template <typename E> constexpr typename underlying_type<E>::type to_underlying(E e) noexcept {
    return static_cast<typename underlying_type<E>::type>(e);
}

namespace detail {

template <class T, std::size_t N, std::size_t... I>
constexpr std::array<std::remove_cv_t<T>, N> to_array_impl(T (&&a)[N], std::index_sequence<I...>) {
    return {{std::move(a[I])...}};
}

}

template <class T, std::size_t N> constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&&a)[N]) {
    return detail::to_array_impl(std::move(a), std::make_index_sequence<N>{});
}

#endif

} // namespace std
