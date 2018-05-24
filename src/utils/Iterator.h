#pragma once

template <typename T>  
class Iterator {
public:
	boolean hasNext() = 0;
	T &next() = 0;
	void remove() = 0;
};