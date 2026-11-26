#pragma once
#include <array>
#include <cmath>
#include <concepts>
#include <format>

template <std::size_t N, typename T>
struct VectorData {
	static_assert(std::is_arithmetic_v<T>);
	std::array<T, N> data{};
};

template <typename T>
struct VectorData<2, T> {
	T x{}, y{};
};

template <typename T>
struct VectorData<3, T> {
	T x{}, y{}, z{};
};

template <std::size_t N, typename T>
struct Vector : VectorData<N, T> {
	constexpr T sqrMagnitude() const {
		T result{};
		for (std::size_t i = 0; i < N; ++i) {
			result += (*this)[i] * (*this)[i];
		}
		return result;
	}
	auto magnitude() const { return std::sqrt(sqrMagnitude()); }
	Vector normalized() const {
		const auto dist = magnitude();
		return dist > 0 ? (*this) / dist : Vector{};
	}

	constexpr T& operator[](std::size_t i) noexcept {
		if constexpr (N == 2) return i == 0 ? this->x : this->y;
		else if constexpr (N == 3) return i == 0 ? this->x : (i == 1 ? this->y : this->z);
		else return this->data[i];
	}
	constexpr const T& operator[](std::size_t i) const noexcept {
		if constexpr (N == 2) return i == 0 ? this->x : this->y;
		else if constexpr (N == 3) return i == 0 ? this->x : (i == 1 ? this->y : this->z);
		else return this->data[i];
	}
	constexpr const std::size_t size() const noexcept { return N; }

	constexpr Vector operator+(const Vector& other) const noexcept {
		Vector result{};
		for (std::size_t i = 0; i < N; ++i) {
			result[i] = (*this)[i] + other[i];
		}
		return result;
	}
	constexpr Vector operator-(const Vector& other) const noexcept {
		Vector result{};
		for (std::size_t i = 0; i < N; ++i) {
			result[i] = (*this)[i] - other[i];
		}
		return result;
	}
	template <typename S>
		requires(std::is_arithmetic_v<T>)
	constexpr Vector operator/(const S& scalar) const {
		Vector result{};
		for (std::size_t i = 0; i < N; ++i) {
			result[i] = (*this)[i] / scalar;
		}
		return result;
	}
	template <typename S>
		requires(std::is_arithmetic_v<T>)
	constexpr Vector operator*(const S& scalar) const {
		Vector result{};
		for (std::size_t i = 0; i < N; ++i) {
			result[i] = (*this)[i] * scalar;
		}
		return result;
	}
};

template <typename T>
constexpr std::string_view type_name() {
#if defined(__clang__) || defined(__GNUC__)
	constexpr std::string_view p = __PRETTY_FUNCTION__;
	constexpr auto start = p.find("T = ") + 4;
	constexpr auto end = p.find_last_of(']');
	return p.substr(start, end - start);
#elif defined(_MSC_VER)
	constexpr std::string_view p = __FUNCSIG__;
	constexpr auto start = p.find("type_name<") + 10;
	constexpr auto end = p.find_last_of('>');
	return p.substr(start, end - start);
#else
	return "unknown";
#endif
}

template <std::size_t N, typename T>
struct std::formatter<Vector<N, T>> : std::formatter<T> {
	bool verbose = false;

	constexpr auto parse(std::format_parse_context& ctx) {
		auto it = ctx.begin();
		if (it != ctx.end() && *it == 'v') {
			verbose = true;
			++it;
			ctx.advance_to(it);
		}
		return std::formatter<T>::parse(ctx);
	}

	auto format(const Vector<N, T>& vec, std::format_context& ctx) const {
		auto out = ctx.out();

		if (verbose) out = std::format_to(out, "Vector<{}, {}>(", N, type_name<T>());
		else out = std::format_to(out, "(");

		for (std::size_t i = 0; i < N; ++i) {
			if (i > 0) out = std::format_to(out, ", ");

			ctx.advance_to(out);
			out = std::formatter<T>::format(vec[i], ctx);
		}
		return std::format_to(out, ")");
	}
};

using Vector2i = Vector<2, int>;
using Vector2f = Vector<2, float>;