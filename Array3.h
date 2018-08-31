#pragma once

//A convenient representation for a three-dimensional array [size x size x size].

template<typename T>
struct Array3
{
	int size;
	T* buffer;
	Array3()
	{
		size = 0;
		buffer = 0;
	}
	Array3(int sz)
	{
		size = 0;
		buffer = 0;
		allocate(sz);
	}
	~Array3()
	{
		if(buffer){
			delete[] buffer;
		}
	}
	int bufferSize()
	{
		return size*size*size;
	}
	void allocate(int sz)
	{
		if(sz != size){
			if(buffer){
				delete[] buffer;
			}
			size = sz;
			buffer = new T[size*size*size];
		}
	}
	int index(int i, int j, int k)
	{
		return i + j * size + k*size*size;
	}
	void getIndices(int idx, int& i, int &j, int& k)
	{
		i = idx % size;
		j = (idx / size) % size;
		k = (idx / size) / size;
	}
	T& operator() (int i, int j, int k)
	{
		return buffer[index(i,j,k)];
	}
	T& operator() (int ijk[])
	{
		return buffer[index(ijk[0],ijk[1],ijk[2])];
	}
	T& operator[] (int i)
	{
		return buffer[i];
	}
	void set(T value)
	{
		for(int i=0; i<size*size*size; i++){
			buffer[i] = value;
		}
	}
};
