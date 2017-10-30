#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <SDL_assert.h>
#include <new>

template <typename T>
class Vector {
public:
	Vector() : mBegin(NULL), mEnd(NULL), mCapacity(NULL) {};
	Vector(size_t size) : mBegin(NULL), mEnd(NULL), mCapacity(NULL) { resize(size); };
	Vector(const Vector& v);
	~Vector();

	Vector<T>& operator=(const Vector<T> &rhs);
	T* data() const { return mBegin; }
	size_t size() const { return length(); }
	size_t length() const { return mEnd - mBegin; };
	size_t capacity() const { return mCapacity - mBegin; };
	bool empty() const { return mBegin == mEnd; };

	T& operator[](size_t i) const { return at(i); }
	T& at(size_t i) const;

	void assign(const Vector<T> &rhs);
	void assign(const T* begin, const T* end);

	T* begin() const { return mBegin; }
	T* back() const { return mEnd - 1; }
	T* end() const { return mEnd; }
	bool within_range(const T* it) const;
	T* erase(const T* it);
	void erase(const T* begin, const T* end);

	void reserve(size_t size);
	void resize(size_t size);
	void clear();

	T& push_back();
	void push_back(const T& value);

	void pop_front() { erase(begin()); }
	void pop_back() { erase(back()); }

	void insert(const T* dst, const T* begin, const T* end);
	void appendBinary(const void* begin, const void* end);
private:
	T* mBegin;
	T* mEnd;
	T* mCapacity;
};

template<typename T>
inline Vector<T>::Vector(const Vector &v) : mBegin(NULL), mEnd(NULL), mCapacity(NULL) {
	assign(v);
}

template<typename T>
inline Vector<T>::~Vector() {
	clear();
	if (mBegin)
		free(mBegin);
}

template<typename T>
inline Vector<T>& Vector<T>::operator=(const Vector<T> &rhs) {
	assign(rhs);
	return *this;
}

template<typename T>
inline T & Vector<T>::at(size_t i) const {
	SDL_assert(i < size());
	return mBegin[i];
}

template<typename T>
inline void Vector<T>::assign(const Vector<T> &rhs) {
	assign(rhs.begin(), rhs.end());
}

template<typename T>
inline void Vector<T>::assign(const T *begin, const T *end) {
	resize(end - begin);
	T* dst = mBegin;
	for (const T* it = begin; it != end; ++it, ++dst)
		*dst = *it;
}

template<typename T>
inline bool Vector<T>::within_range(const T *it) const {
	return it >= mBegin && it <= mEnd;
}

template<typename T>
inline T * Vector<T>::erase(const T *it) {
	erase(it, it + 1);
	return (T*)it;
}

template<typename T>
inline void Vector<T>::erase(const T *begin, const T *end) {
	SDL_assert(within_range(begin));
	SDL_assert(within_range(end));

	T* dst = (T*)begin;
	for (T* it = (T*)end; it != mEnd; ++it, ++dst) {
		*dst = *it;
	}
	resize(size() - (size_t)(end - begin));
}

template<typename T>
inline void Vector<T>::reserve(size_t size) {
	if (mBegin) {
		if (mBegin + size > mCapacity) {
			size_t strlen = length();

			T* newBegin = (T*)malloc(size * sizeof(T));
			T* dst = newBegin;
			for (T* it = mBegin; it != mEnd; ++it, ++dst) {
				new (dst) T();
				*dst = *it;
				it->~T();
			}

			free(mBegin);

			mBegin = newBegin;
			mEnd = mBegin + strlen;
			mCapacity = mBegin + size;
		}
	} else if(size > 0) {
		mEnd = mBegin = (T*)malloc(size * sizeof(T));
		mCapacity = mBegin + size;
	}
}

template<typename T>
inline void Vector<T>::resize(size_t size) {
	size_t oldSize = this->size();
	reserve(size);

	if (size > oldSize) {
		for (T* it = mBegin + oldSize, *end = mBegin + size; it != end; ++it)
			new (it) T();
	} else if (size < oldSize) {
		for (T* it = mEnd - 1, *end = mBegin + size - 1; it != end; --it)
			it->~T();
	}

	mEnd = mBegin + size;
}

template<typename T>
inline void Vector<T>::clear() {
	if (mBegin) {
		for (T* it = mBegin; it != mEnd; ++it)
			it->~T();
		mEnd = mBegin;
	}
}

template<typename T>
inline T& Vector<T>::push_back() {
	if (capacity() < size() + 1)
		reserve(size() * 2 + 1);

	new (mEnd) T();
	++mEnd;
	return mEnd[-1];
}

template<typename T>
inline void Vector<T>::push_back(const T &value) {
	if (capacity() < size() + 1)
		reserve(size() * 2 + 1);

	new (mEnd) T();
	mEnd[0] = value;
	++mEnd;
}

template<typename T>
inline void Vector<T>::insert(const T* dst, const T *begin, const T *end) {
	size_t datalen = end - begin;

	if (capacity() < size() + datalen)
		reserve(size() * 2 + datalen);

	for (const T* it = begin; it != end; ++it) {
		push_back(*it);
	}
}

template<typename T>
inline void Vector<T>::appendBinary(const void * begin, const void * end) {
	size_t datalen = (uint8_t*)end - (uint8_t*)begin;

	if (capacity() < size() + (datalen / sizeof(T)))
		reserve(size() * 2 + (datalen / sizeof(T)));

	memcpy(mEnd, begin, datalen);
	mEnd += datalen / sizeof(T);
}
