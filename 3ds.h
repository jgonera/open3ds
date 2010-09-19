#ifndef _3DS_H_
#define _3DS_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <list>
#include <algorithm>
#include <stdexcept>
#include <SFML/Graphics.hpp>
#include <GL/gl.h>

#include "types3ds.h"

#include <iostream>

using namespace std;

namespace cfg3ds {
	const int chunkHeaderSize = 6;
	const GLfloat selectedColor[] = {0.f, 1.f, 1.f, 1.f};
}

namespace chunks
{
	const Word MAIN = 0x4D4D;
		
		const Word EDIT = 0x3D3D;
			const Word EDIT_OBJECT = 0x4000;
				const Word OBJECT_MESH = 0x4100;
					const Word MESH_VERTICES = 0x4110;
					const Word MESH_FACES = 0x4120;
						const Word FACES_MATERIALS = 0x4130;
						
					const Word MESH_MAPCOORDS = 0x4140;
					const Word MESH_LOCALCOORDS = 0x4160;
					
			const Word EDIT_MATERIAL = 0xAFFF;
				const Word MATERIAL_NAME = 0xA000;
				const Word MATERIAL_AMBIENT = 0xA010;
				const Word MATERIAL_DIFFUSE = 0xA020;
				const Word MATERIAL_SPECULAR = 0xA030;
				const Word MATERIAL_TEXMAP = 0xA200;
					const Word TEXMAP_FILE = 0xA300;
		
		const Word KEYFRAMER = 0xB000;
			const Word KEYFRAMER_MESHINFO = 0xB002;
				const Word MESHINFO_HIERARCHY = 0xB010;
				const Word MESHINFO_PIVOT = 0xB013;
				const Word MESHINFO_POSTRACK = 0xB020;
				const Word MESHINFO_ROTTRACK = 0xB021;
				const Word MESHINFO_SCALETRACK = 0xB022;

	const Word COLOR_FLOAT = 0x0010;
	const Word COLOR_BYTE = 0x0011;
	const Word COLOR_BYTEG = 0x0012;
	const Word COLOR_FLOATG = 0x0013;
}

struct Object
{
	Object(GLuint *&tex, GLuint sel);
	~Object();
	
	void draw(bool highlighted = false) const;
	
	char *name;
	Vertex *vertices;
	Vector *normals;
	Face *faces;
	MapCoord *mapCoords;
	Word numVertices, numFaces;
	list<VertexList *> vertexLists;
	
	Vector u, v, w, origin;
	Vector pivot;
	
	Vector postrack;
	Vector rottrackAxis;
	GLfloat rottrackAngle;
	GLfloat scaletrackX, scaletrackY, scaletrackZ;
	
	Vector position;
	Vector rotation;
	
	list<Object *> children;
	GLuint **textures;
	
	GLuint selectName;
	bool selected;
};

class Model3DS
{
	public:
		Model3DS(GLuint sel = 0);
		~Model3DS();
		bool load(const char *fileName);
		void draw() const;
		void select(GLint selectedName);
		void rotateSelected(GLfloat delta, Axis axis);
		void translateSelected(GLfloat delta, Axis axis);
	
	protected:
		void parse();
			void parseMain();
				void parseEdit();
					void parseObject();
						void parseMesh(Object *object);
							void parseFaces(Object *object);
						
					void parseMaterial();
						void parseTexmap(Material *material);
				
				void parseKeyframer();
					void parseMeshinfo();
			void parseColor(Color &color);
		
		size_t readChunkHeader() { return (read(currentChunk.id) + read(currentChunk.length)); }
		void skipChunk() {
			cout << "skip: " << hex << currentChunk.id << dec << endl;
			fseek(fp, currentChunk.length - cfg3ds::chunkHeaderSize, SEEK_CUR);
		}
		
		size_t read(Byte &x) { return (fread(&x, sizeof(x), 1, fp) == 1 ? sizeof(x) : 0); }
		size_t read(Word &x) { return (fread(&x, sizeof(x), 1, fp) == 1 ? sizeof(x) : 0); }
		size_t read(DWord &x) { return (fread(&x, sizeof(x), 1, fp) == 1 ? sizeof(x) : 0); }
		size_t read(GLfloat &x) { return (fread(&x, sizeof(x), 1, fp) == 1 ? sizeof(x) : 0); }
		size_t read(Vector &x) { return (fread(&x, sizeof(x), 1, fp) == 1 ? sizeof(x) : 0); }
		size_t readString(char *&x);
		
		FILE *fp;
		char *path;
		ChunkHeader currentChunk; // currently parsed chunk header
		
		short int previousLevel, rootLevel;
		Object *previousObject, *currentParent;
		vector<Object *> parents;
		
		list<Object *> objects;
		list<Material *> materials;
		
		list<Object *> roots;
		
		GLuint *textures;
		GLuint numTextures;

		GLuint selectName;		
		GLuint currentSelectName;
		
		Object *selectedObject;
};

#endif // _3DS_H_
