#pragma once

#include <math.h>
#include <float.h>

//Convenience function to assign 3 coordinates at once.
void set(float v[3], float x, float y, float z)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

//Integer triplet assignment
void set(int i[3], int i1, int i2, int i3)
{
	i[0] = i1;
	i[1] = i2;
	i[2] = i3;
}

//Rotate vector around Y axis.
void rotateY(float v[3], float angle)
{
	float c = cosf(angle);
	float s = sinf(angle);
	float x = v[0] * c - v[2] * s;
	float z = v[0] * s + v[2] * c;
	v[0] = x;
	v[2] = z;
}

//Rotate vector around X axis.
void rotateX(float v[3], float angle)
{
	float c = cosf(angle);
	float s = sinf(angle);
	float y = v[1] * c - v[2] * s;
	float z = v[1] * s + v[2] * c;
	v[1] = y;
	v[2] = z;
}

float dotProduct(float a[3], float b[3])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

//Intersect a ray originating at p in direction of v with a plane.
//Return the parametric value t such that p + v * t produces the intersection point.
float rayHitPlane(float p[3], float v[3], float plane[4])
{
	float dist = dotProduct(p, plane) - plane[3];
	float speed = -dotProduct(v, plane);
	if(speed == 0){
		return FLT_MAX;//The ray is parallel to the plane
	}
	return dist / speed;
}

//A common operation on vectors: out = p + v * t
void mulAdd(float p[3], float v[3], float t, float out[4])
{
	for(int i=0; i<3; i++){
		out[i] = p[i] + v[i] * t;
	}
}

//Returns the index of the largest magnitude component of a vector.
int maxDim(float v[3])
{
	if(abs(v[0])>abs(v[1]) && abs(v[0])>abs(v[2])){
		return 0;
	}
	if(abs(v[1]) > abs(v[2])){
		return 1;
	}
	return 2;
}
