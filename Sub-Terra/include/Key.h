#pragma once

enum class Key : uint8_t {
	None,
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	Escape,
	Space
};

struct KeyHash : public std::unary_function<Key, std::size_t> {
	std::size_t operator()(const Key &k) const {
		return static_cast<size_t>(k);
	}
};

struct KeyEqual : public std::binary_function<Key, Key, bool> {
	bool operator()(const Key &v0, const Key &v1) const {
		return v0 == v1;
	}
};
