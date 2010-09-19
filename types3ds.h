#ifndef _TYPES3DS_H_
#define _TYPES3DS_H_

#include <cmath>

template <typename T>
void deleteElement(T *p)
{
	delete p;
}

typedef unsigned char Byte;
typedef unsigned short Word;
typedef unsigned int DWord;

enum Axis { x, y, z };

struct ChunkHeader
{
	Word id;
	DWord length;
};// __attribute__((packed));

struct Color
{
	Color(): r(1.f), g(1.f), b(1.f), a(1.f) {}
	
	GLfloat r, g, b, a;
};

struct Vector
{
	Vector(GLfloat x = 0.f, GLfloat y = 0.f, GLfloat z = 0.f): x(x), y(y), z(z) {}
	Vector(const Vector &v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}
	Vector &operator +=(const Vector &v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		
		return *this;
	}
	Vector &operator -=(const Vector &v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		
		return *this;
	}
	Vector operator *(const Vector &v) const
	{
		Vector r;
		r.x = y*v.z - v.y*z;
		r.y = z*v.x - v.z*x;
		r.z = x*v.y - v.x*y;
		
		return r;
	}
	GLfloat dotProduct(const Vector &v) const
	{
		return x*v.x + y*v.y + z*v.z;
	}
	Vector operator *(GLfloat c) const
	{
		Vector r;
		r.x = x*c;
		r.y = y*c;
		r.z = z*c;
		
		return r;
	}
	float length() const
	{
		return sqrt(x*x + y*y + z*z);
	}
	Vector normalize()
	{
		GLfloat length = this->length();
		x /= length;
		y /= length;
		z /= length;
		return *this;
	}
	Vector normalized() const
	{
		Vector r(*this);
		r.normalize();
		return r;
	}
	Vector operator +(const Vector &v) const
	{
		Vector r;
		r.x = x+v.x;
		r.y = y+v.y;
		r.z = z+v.z;
		
		return r;
	}
	Vector operator -(const Vector &v) const
	{
		Vector r;
		r.x = x-v.x;
		r.y = y-v.y;
		r.z = z-v.z;
		
		return r;
	}

	GLfloat x, y, z;
};

typedef Vector Vertex;

struct Face
{
	Word vertexA, vertexB, vertexC;
};

struct MapCoord
{
	GLfloat u, v;
};

struct Material
{
	Material(): name(NULL), texmapFile(NULL) {}
	~Material()
	{
		delete [] name;
		delete [] texmapFile;
	}
	
	char *name, *texmapFile;
	Color ambient, diffuse, specular;
	GLuint textureRef;
};

struct VertexList
{
	VertexList(): material(NULL), verticesRefs(NULL), numVerticesRefs(0) {}
	~VertexList()
	{
		if (verticesRefs != NULL)
			delete [] verticesRefs;
	}
	
	Material *material;
	Word *verticesRefs;
	DWord numVerticesRefs;
};

#endif // _TYPES3DS_H_
