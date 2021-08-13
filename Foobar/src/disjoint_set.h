#pragma once

#include <array>

template<size_t N>
class disjoint_set
{
public:
	disjoint_set()
	{
		for (size_t i = 0; i < N; ++i) {
			_parent[i] = i;
		}
		for (size_t i = 0; i < N; ++i) {
			_size[i] = 1;
		}
	}

	bool connect(size_t a, size_t b)
	{
		if (a >= N || b >= N) {
			return false;
		}

		size_t root_a = find_root(a);
		size_t root_b = find_root(b);

		if (root_a == root_b) {
			return false;
		}

		if (_size[root_a] < _size[root_b])
		{
			_parent[root_a] = root_b;
			_size[root_b] += _size[root_b];
		}
		else
		{
			_parent[root_b] = root_a;
			_size[root_a] += _size[root_b];
		}

		return true;
	}

	bool is_connected(size_t a, size_t b)
	{
		return find_root(a) == find_root(b);
	}

private:
	size_t find_root(size_t a)
	{
		if (a == _parent[a]) {
			return a;
		}

		_parent[a] = find_root(_parent[a]);
		return _parent[a];
	}

	std::array<size_t, N> _parent;
	std::array<size_t, N> _size;
};