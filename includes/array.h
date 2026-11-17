#ifndef ARRAY_H
#define ARRAY_H

#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

template<typename T>
class Array {
private:
	T* data;
	size_t size;
	size_t capacity;

	void resize() {
		if (size == capacity) {
			size_t newCapacity;

			if (capacity == 0) {
				newCapacity = 1;
			} else {
				newCapacity = capacity * 2;
			}

			T* newData = new T[newCapacity];

			for (size_t i = 0; i < size; ++i) {
				newData[i] = data[i];
			}

			delete[] data;
			data = newData;
			capacity = newCapacity;
		}
	}

public:
	Array() {
		data = nullptr;
		size = 0;
		capacity = 0;
	}

	Array(const Array& other) {
		size = other.size;
		capacity = other.capacity;

		if (capacity == 0) {
			data = nullptr;
		} else {
			data = new T[capacity];

			for (size_t i = 0; i < size; ++i) {
				data[i] = other.data[i];
			}
		}
	}

	Array& operator=(const Array& other) {
		if (this == &other) return *this;

		T* newData = nullptr;

		if (other.capacity > 0) {
			newData = new T[other.capacity];

			for (size_t i = 0; i < other.size; ++i) {
				newData[i] = other.data[i];
			}
		}

		delete[] data;
		data = newData;
		size = other.size;
		capacity = other.capacity;
		
		return *this;
	}

	void pushBack(const T& value) {
		T temp = value;
		resize();
		data[size] = temp;
		size++;
	}

	void insert(const T& value, size_t index) {
		if (index > size) throw out_of_range("index out of range");

		T temp = value;
		resize();

		for (size_t i = size; i > index; --i) {
			data[i] = data[i - 1];
		}

		data[index] = temp;
		size++;
	}

	T& get(size_t index) {
		if (index >= size) throw out_of_range("index out of range");

		return data[index];
	}

	void replace(size_t index, const T& value) {
    if (index >= size) {
			throw out_of_range("index out of range");
    }

    data[index] = value;
	}

	size_t length() const {
    return size;
	}

	bool isEmpty() const {
    return size == 0;
	}

	void clear() {
    delete[] data;

    data = nullptr;
    size = 0;
    capacity = 0;
	}

	void remove(size_t index) {
		if (index >= size) return;

		for (size_t i = index; i + 1 < size; ++i) {
			data[i] = data[i + 1];
		}

		size--;
		data[size] = T{};
	}



	~Array() {
		delete[] data;
	}

};

#endif
