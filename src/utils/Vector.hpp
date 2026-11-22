#pragma once
#include <array>
#include <cmath>
#include <concepts>

template <std::size_t N, typename T>
struct VectorData {
	static_assert(std::is_arithmetic_v<T>);
	std::array<T, N> data{};
};

template <typename T>
struct VectorData<2, T> {
	T x{}, y{};
	VectorData(T x, T y) : x(x), y(y) {}
};

template <typename T>
struct VectorData<3, T> {
	T x{}, y{}, z{};
	VectorData(T x, T y, T z) : x(x), y(y), z(z) {}
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
	Vector normalized() const { return (*this) / magnitude(); }

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

	constexpr Vector operator+(const Vector& other) noexcept {
		static_assert(N == other.data.size());
		Vector<T, N> result{};
		for (std::size_t i = 0; i < N; ++i) {
			result[i] = (*this)[i] + (*other)[i];
		}
		return result;
	}
	constexpr Vector operator-(const Vector& other) noexcept {
		static_assert(N == other.size());
		Vector<N, T> result(*this);
		for (std::size_t i = 0; i < N; ++i) {
			result[i] = (*this)[i] - other[i];
		}
		return result;
	}
	template <typename t>
		requires(std::is_arithmetic_v<t>)
	constexpr Vector operator/(const t& scalar) const {
		Vector result{};
		for (std::size_t i = 0; i < N; ++i) {
			result[i] = (*this)[i] / scalar;
		}
		return result;
	}

	Vector() = default;
	template <typename... Args>
		requires(sizeof...(Args) == N && (std::is_convertible_v<Args, T> && ...))
	constexpr Vector(Args... args) : VectorData<N, T>{ static_cast<T>(args)... } {}
};

using Vector2i = Vector<2, int>;
using Vector2f = Vector<2, float>;