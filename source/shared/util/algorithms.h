#pragma once

#include <cctype>
#include <type_traits>
#include <string_view>
#include <utility>

namespace algo {
	template <class T, class U, typename = void>
	struct is_comparable : std::false_type {};
	template <class T, class U>
	struct is_comparable<T, U, decltype((std::declval<T>() == std::declval<U>()), void())>
		: std::true_type {};
	template<class T, class U>
	inline constexpr auto is_comparable_v = is_comparable<T, U>::value;

	template<class T>
	concept range = requires (T && cont) {
		begin(cont);
		end(cont);
	};

	template<class ContainerT, class ValueT>
	requires range<ContainerT>&& is_comparable_v<decltype(*begin(std::declval<ContainerT>())), ValueT>
		void erase(ContainerT&& container, ValueT&& value) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		container.erase(std::remove(begin_it, end_it, std::forward<ValueT>(value)), end_it);
	}

	template<class ContainerT, class FunT>
	requires range<ContainerT>&& std::is_invocable_v<FunT, decltype(*begin(std::declval<ContainerT>()))>
		void erase_if(ContainerT&& container, FunT&& fun) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		container.erase(std::remove_if(begin_it, end_it, std::forward<FunT>(fun)), end_it);
	}

	template<class ContainerT, class ValueT>
	requires range<ContainerT>&& is_comparable_v<decltype(*begin(std::declval<ContainerT>())), ValueT>
		bool contains(ContainerT&& container, ValueT&& value) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		return std::find(begin_it, end_it, std::forward<ValueT>(value)) != end_it;
	}

	template<class ContainerT, class FunT>
	requires range<ContainerT>&& std::is_invocable_v<FunT, decltype(*begin(std::declval<ContainerT>()))>
		bool contains_if(ContainerT&& container, FunT&& fun) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		return std::find_if(begin_it, end_it, std::forward<FunT>(fun)) != end_it;
	}

	template<class ContainerT, class FunT>
	requires range<ContainerT>&& std::is_invocable_v<FunT, decltype(*begin(std::declval<ContainerT>()))>
		auto find_if(ContainerT&& container, FunT&& fun) -> decltype(&*begin(std::declval<ContainerT>())) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		auto found_it = std::find_if(begin_it, end_it, std::forward<FunT>(fun));
		if (found_it != end_it) {
			return &*found_it;
		}
		return nullptr;
	}

	template<class ContainerT, class ValueT>
	requires range<ContainerT>&& is_comparable_v<decltype(*begin(std::declval<ContainerT>())), ValueT>
		auto count(ContainerT&& container, ValueT&& value) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		return static_cast<std::size_t>(std::count(begin_it, end_it, std::forward<ValueT>(value)));
	}

	template<class ContainerT, class FunT>
	requires range<ContainerT>&& std::is_invocable_v<FunT, decltype(*begin(std::declval<ContainerT>()))>
		auto count_if(ContainerT&& container, FunT&& fun) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		return static_cast<std::size_t>(std::count_if(begin_it, end_it, std::forward<FunT>(fun)));
	}

	inline bool is_sub_path(const std::filesystem::path& path, const std::filesystem::path& base) {
		const auto first_mismatch = std::mismatch(path.begin(), path.end(), base.begin(), base.end());
		return first_mismatch.second == base.end();
	}

	inline std::string trim(std::string str) {
		str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
			return !std::isspace(ch);
		}));
		str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
		}).base(), str.end());
		return std::move(str);
	}

	inline std::string trim(std::string str, char to_strip) {
		str.erase(str.begin(), std::find_if(str.begin(), str.end(), [to_strip](unsigned char ch) {
			return ch != to_strip;
		}));
		str.erase(std::find_if(str.rbegin(), str.rend(), [to_strip](unsigned char ch) {
			return ch != to_strip;
		}).base(), str.end());
		return std::move(str);
	}

	inline bool case_insensitive_equal(std::string_view lhs, std::string_view rhs) {
		struct case_insensitive_char_traits : public std::char_traits<char> {
			static bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
			static bool ne(char c1, char c2) { return toupper(c1) != toupper(c2); }
			static bool lt(char c1, char c2) { return toupper(c1) < toupper(c2); }
			static int compare(const char* s1, const char* s2, size_t n) {
				while (n-- != 0) {
					if (toupper(*s1) < toupper(*s2)) return -1;
					if (toupper(*s1) > toupper(*s2)) return 1;
					++s1; ++s2;
				}
				return 0;
			}
			static const char* find(const char* s, int n, char a) {
				while (n-- > 0 && toupper(*s) != toupper(a)) {
					++s;
				}
				return s;
			}
		};
		using case_insensitive_string_view = std::basic_string_view<char, case_insensitive_char_traits>;
		return case_insensitive_string_view{ lhs.data(), lhs.size() } == case_insensitive_string_view{ rhs.data(), rhs.size() };
	}
	template<std::size_t N>
	inline bool case_insensitive_equal(char (&lhs)[N], std::string_view rhs) {
		return case_insensitive_equal(std::string_view{ lhs, N }, rhs);
	}
	template<std::size_t N>
	inline bool case_insensitive_equal(std::string_view lhs, char (&rhs)[N]) {
		return case_insensitive_equal(lhs, std::string_view{ rhs, N });
	}
	template<std::size_t N, std::size_t M>
	inline bool case_insensitive_equal(char (&lhs)[N], char (&rhs)[M]) {
		return case_insensitive_equal(std::string_view{ lhs, N }, std::string_view{ rhs, M });
	}
}
