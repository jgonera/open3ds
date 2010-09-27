#include "3ds.h"

Object::Object(GLuint *&tex, GLuint sel):
	name(NULL),
	vertices(NULL),
	normals(NULL),
	faces(NULL),
	mapCoords(NULL),
	numVertices(0),
	numFaces(0),
	selectName(sel),
	selected(false)
{
	textures = &tex;
}

Object::~Object()
{
	delete [] name;
	delete [] vertices;
	delete [] normals;
	delete [] faces;
	delete [] mapCoords;
		
	for_each(vertexLists.begin(), vertexLists.end(), deleteElement<VertexList>);
}

Model3DS::Model3DS(GLuint sel):
	path(NULL),
	rootLevel(-1),
	previousObject(NULL),
	textures(NULL),
	selectName(sel),
	currentSelectName(0),
	selectedObject(NULL)
{}

Model3DS::~Model3DS()
{
	for_each(objects.begin(), objects.end(), deleteElement<Object>);
	for_each(materials.begin(), materials.end(), deleteElement<Material>);
	glDeleteTextures(numTextures, textures);
	delete [] path;
	delete [] textures;
}

bool Model3DS::load(const char *fileName)
{
	char *file = strrchr(const_cast<char *>(fileName), '/');
	if (file == NULL)
		file = strrchr(const_cast<char *>(fileName), '\\');
	if (file == NULL)
		file = const_cast<char *>(fileName);
	
	cout << file << endl;
	
	path = new char[strlen(fileName)-strlen(file)+2];
	
	strncpy(path, fileName, strlen(fileName)-strlen(file)+1);
	path[strlen(fileName)-strlen(file)+1] = '\0';
	
	cout << path << endl;

	fp = fopen(fileName, "rb");
	
	if (fp == NULL)
		return false;
	
	parse();
	
	fclose(fp);
	
	return true;
}

void Object::draw(bool highlighted) const
{
	glPushName(selectName);
	
	glPushMatrix();

	if (numVertices != 0) {	
		GLfloat m[] = {
			u.x, u.y, u.z, 0.f,
			v.x, v.y, v.z, 0.f,
			w.x, w.y, w.z, 0.f,
			0,0,0, 1.f
		};

		GLfloat mInv[] = {
			u.x, v.x, w.x, 0.f,
			u.y, v.y, w.y, 0.f,
			u.z, v.z, w.z, 0.f,
			0.f, 0.f, 0.f, 1.f
		};
		
		glTranslatef(position.x, position.y, position.z);

		glTranslatef(origin.x, origin.y, origin.z);

		glMultMatrixf(m);
		
		if (selected) {
			glDisable(GL_LIGHTING);
			glLineWidth(2.f);
			
			glBegin(GL_LINES);
			
			glColor3f(1.f, 0.f, 0.f);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(20.f, 0.f, 0.f);
			
			glColor3f(0.f, 1.f, 0.f);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(0.f, 20.f, 0.f);
			
			glColor3f(0.f, 0.f, 1.f);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(0.f, 0.f, 20.f);
			
			glEnd();
			
			glEnable(GL_LIGHTING);
		}
		
		glRotatef(rotation.x, 1.f, 0.f, 0.f);
		glRotatef(rotation.y, 0.f, 1.f, 0.f);
		glRotatef(rotation.z, 0.f, 0.f, 1.f);
		
		glTranslatef(-pivot.x, -pivot.y, -pivot.z);

		glMultMatrixf(mInv);

		glTranslatef(-origin.x, -origin.y, -origin.z);
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		if (mapCoords != NULL)
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glNormalPointer(GL_FLOAT, 0, normals);
		glTexCoordPointer(2, GL_FLOAT, 0, mapCoords);
		
		for (list<VertexList *>::const_iterator vIt = vertexLists.begin(); vIt != vertexLists.end(); ++vIt) {
			glMaterialfv(GL_FRONT, GL_DIFFUSE, reinterpret_cast<GLfloat *>(&(*vIt)->material->diffuse));
			glMaterialfv(GL_FRONT, GL_SPECULAR, reinterpret_cast<GLfloat *>(&(*vIt)->material->specular));
			
			if (selected || highlighted)
				glMaterialfv(GL_FRONT, GL_AMBIENT, cfg3ds::selectedColor);
			else
				glMaterialfv(GL_FRONT, GL_AMBIENT, reinterpret_cast<GLfloat *>(&(*vIt)->material->ambient));
			
			if ((*vIt)->material->texmapFile != NULL) {
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, (*textures)[(*vIt)->material->textureRef]);
			}
			
			glDrawElements(GL_TRIANGLES, (*vIt)->numVerticesRefs, GL_UNSIGNED_SHORT, (*vIt)->verticesRefs);
		}
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		if (mapCoords != NULL)
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		
	}
	
	for (list<Object *>::const_iterator oIt = children.begin(); oIt != children.end(); ++oIt) {
		(*oIt)->draw(selected || highlighted);
	}

	glPopMatrix();
	
	glPopName();
}

void Model3DS::draw() const
{
	glLoadName(selectName);
	
	glPushMatrix();
	
	for (list<Object *>::const_iterator oIt = roots.begin(); oIt != roots.end(); ++oIt) {
		(*oIt)->draw();
	}
	
	glPopMatrix();
}

void Model3DS::select(GLint selectedName)
{
	if (selectedName == -1)
		selectedObject = NULL;
	
	for (list<Object *>::const_iterator it = objects.begin(); it != objects.end(); ++it) {
		if ((*it)->selectName == static_cast<GLuint>(selectedName)) {
			(*it)->selected = true;
			selectedObject = *it;
		} else {
			(*it)->selected = false;
		}
	}
}

void Model3DS::rotateSelected(GLfloat delta, Axis axis)
{
	if (selectedObject == NULL)
		return;
	
	switch (axis) {
		case x: selectedObject->rotation.x += delta; break;
		case y: selectedObject->rotation.y += delta; break;
		case z: selectedObject->rotation.z += delta; break;
	}
}

void Model3DS::translateSelected(GLfloat delta, Axis axis)
{
	if (selectedObject == NULL)
		return;
	
	switch (axis) {
		case x: selectedObject->position.x += delta; break;
		case y: selectedObject->position.y += delta; break;
		case z: selectedObject->position.z += delta; break;
	}
}

void Model3DS::parse()
{
	readChunkHeader();
	
	if (currentChunk.id == chunks::MAIN)
		parseMain();
	else
		exit(1);
}

void Model3DS::parseMain()
{
	cout << "parseMain" << endl;
	DWord length = currentChunk.length;
	DWord n = cfg3ds::chunkHeaderSize;
	
	while (n < length)
	{
		readChunkHeader();
		n += currentChunk.length;
		switch (currentChunk.id)
		{
			case chunks::EDIT:
				parseEdit();
				break;
				
			case chunks::KEYFRAMER:
				parseKeyframer();
				break;
			
			default:
				skipChunk();
		}
	}
}

void Model3DS::parseEdit()
{
	cout << "parseEdit" << endl;
	DWord length = currentChunk.length;
	DWord n = cfg3ds::chunkHeaderSize;
	
	numTextures = 0;
	
	while (n < length)
	{
		readChunkHeader();
		n += currentChunk.length;
		switch (currentChunk.id)
		{
			case chunks::EDIT_OBJECT:
				parseObject();
				break;
				
			case chunks::EDIT_MATERIAL:
				parseMaterial();
				break;
				
			default:
				skipChunk();
		}
	}
	
	textures = new GLuint[numTextures];
	glGenTextures(numTextures, textures);
	
	for (list<Material *>::iterator it = materials.begin(); it != materials.end(); ++it) {
		if ((*it)->texmapFile == NULL)
			continue;
		
		char *texmapFileName = new char[strlen(path) + strlen((*it)->texmapFile) + 1];
		sprintf(texmapFileName, "%s%s", path, (*it)->texmapFile);
		
		sf::Image image;
		bool result = image.LoadFromFile(texmapFileName);
		delete [] texmapFileName;
		
		if (result == false) {
			cout << "Can't read texture file!" << endl;
			(*it)->texmapFile = NULL;
			continue;
		}
		
		glBindTexture(GL_TEXTURE_2D, textures[(*it)->textureRef]);
				
		// select modulate to mix texture with color for shading
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		
		// when texture area is small, bilinear filter the closest mipmap
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		// when texture area is large, bilinear filter the original
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// the texture wraps over at the edges (repeat)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, image.GetWidth(), image.GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, image.GetPixelsPtr());
	}
}

void Model3DS::parseObject()
{
	cout << "parseObject" << endl;
	DWord length = currentChunk.length;
	
	Object *object = new Object(textures, currentSelectName++);
	
	DWord n = cfg3ds::chunkHeaderSize;
	n += readString(object->name);
	cout << "\tname: " << object->name << endl;
	
	while (n < length)
	{
		readChunkHeader();
		n += currentChunk.length;
		switch (currentChunk.id)
		{
			case chunks::OBJECT_MESH:
				parseMesh(object);
				break;
			
			default:
				skipChunk();
		}
	}
	
	for (int i=0; i<object->numVertices; ++i)
		object->normals[i].normalize();
	
	objects.push_back(object);
}

void Model3DS::parseMesh(Object *object)
{
	cout << "parseMesh" << endl;
	DWord length = currentChunk.length;
	DWord n = cfg3ds::chunkHeaderSize;
	
	while (n < length)
	{
		readChunkHeader();
		n += currentChunk.length;
		switch (currentChunk.id)
		{
			case chunks::MESH_VERTICES:
				read(object->numVertices);
				object->vertices = new Vertex[object->numVertices];
				object->normals = new Vector[object->numVertices];
				memset(object->normals, 0, sizeof(Vector)*object->numVertices);
				
				for (int i=0; i<object->numVertices; ++i) {
					read(object->vertices[i].x);
					read(object->vertices[i].y);
					read(object->vertices[i].z);
				}
				break;
				
			case chunks::MESH_FACES:
				parseFaces(object);
				break;
				
			case chunks::MESH_MAPCOORDS:
				Word numEntries;
				read(numEntries);
				
				object->mapCoords = new MapCoord[numEntries];
				
				for (int i=0; i<numEntries; ++i) {
					read(object->mapCoords[i].u);
					read(object->mapCoords[i].v);
				}
				
				break;
				
			case chunks::MESH_LOCALCOORDS:
				read(object->u);
				read(object->v);
				read(object->w);
				read(object->origin);
				
				object->u.normalize();
				object->v.normalize();
				object->w.normalize();
				
				break;
				
			default:
				skipChunk();
		}
	}
}

void Model3DS::parseFaces(Object *object)
{
	cout << "parseFaces" << endl;
	DWord length = currentChunk.length;
	DWord n = cfg3ds::chunkHeaderSize;
	
	n += read(object->numFaces);
	
	object->faces = new Face[object->numFaces];
	
	Word faceFlag;

	for (int i=0; i<object->numFaces; ++i) {
		n += read(object->faces[i].vertexA);
		n += read(object->faces[i].vertexB);
		n += read(object->faces[i].vertexC);
		n += read(faceFlag);
		
		Vector vectorAB = object->vertices[object->faces[i].vertexB] - object->vertices[object->faces[i].vertexA];
		Vector vectorBC = object->vertices[object->faces[i].vertexC] - object->vertices[object->faces[i].vertexB];
		
		Vector faceNormal = vectorAB * vectorBC;
		
		object->normals[object->faces[i].vertexA] += faceNormal;
		object->normals[object->faces[i].vertexB] += faceNormal;
		object->normals[object->faces[i].vertexC] += faceNormal;
	}
	
	VertexList *vertexList;
	
	while (n < length) {
		readChunkHeader();
		n += currentChunk.length;
		
		switch (currentChunk.id) {
			case chunks::FACES_MATERIALS:
				vertexList = new VertexList();
				
				char *materialName;
				readString(materialName);
				
				// find the material
				
				for (list<Material *>::const_iterator it = materials.begin(); it != materials.end(); ++it) {
					if (strcmp((*it)->name, materialName) == 0) {
						vertexList->material = *it;
						break;
					}
				}
				
				delete [] materialName;
				
				if (vertexList->material == NULL)
					throw runtime_error("Needed material not found in materials list!");
				
				Word numEntries;
				read(numEntries);
				
				vertexList->numVerticesRefs = numEntries * 3; // *3 because there are 3 vertices per face
				vertexList->verticesRefs = new Word[vertexList->numVerticesRefs];
				
				Word faceRef;
				
				for (unsigned int i=0; i<vertexList->numVerticesRefs; i+=3) {
					read(faceRef);
					vertexList->verticesRefs[i] = object->faces[faceRef].vertexA;
					vertexList->verticesRefs[i+1] = object->faces[faceRef].vertexB;
					vertexList->verticesRefs[i+2] = object->faces[faceRef].vertexC;
				}
				
				object->vertexLists.push_back(vertexList);
				break;
			
			default:
				skipChunk();
		}
	}
}

void Model3DS::parseMaterial()
{
	cout << "parseMaterial" << endl;
	DWord length = currentChunk.length;
	DWord n = cfg3ds::chunkHeaderSize;
	
	Material *material = new Material();
	
	while (n < length)
	{
		readChunkHeader();
		n += currentChunk.length;
		switch (currentChunk.id)
		{
			case chunks::MATERIAL_NAME:
				readString(material->name);
				break;
			
			case chunks::MATERIAL_AMBIENT:
				parseColor(material->ambient);
				break;
				
			case chunks::MATERIAL_DIFFUSE:
				parseColor(material->diffuse);
				break;
			
			case chunks::MATERIAL_SPECULAR:
				parseColor(material->specular);
				break;
				
			case chunks::MATERIAL_TEXMAP:
				parseTexmap(material);
				break;
			
			default:
				skipChunk();
		}
	}
	
	cout << "\tname: " << material->name << endl;
	materials.push_back(material);
}

void Model3DS::parseTexmap(Material *material)
{
	cout << "parseTexmap" << endl;
	DWord length = currentChunk.length;
	DWord n = cfg3ds::chunkHeaderSize;
	
	while (n < length)
	{
		readChunkHeader();
		n += currentChunk.length;
		switch (currentChunk.id)
		{
			case chunks::TEXMAP_FILE:
			{
				readString(material->texmapFile);
				cout << material->texmapFile << endl;
				
				material->textureRef = numTextures++;
				
				break;
			}
			
			default:
				skipChunk();
		}
	}
}

void Model3DS::parseKeyframer()
{
	cout << "parseKeyframer" << endl;
	DWord length = currentChunk.length;
	DWord n = cfg3ds::chunkHeaderSize;
	
	while (n < length)
	{
		readChunkHeader();
		n += currentChunk.length;
		switch (currentChunk.id)
		{
			case chunks::KEYFRAMER_MESHINFO:
				parseMeshinfo();
				break;
				
			default:
				skipChunk();
		}
	}
}

void Model3DS::parseMeshinfo()
{
	//cout << "parseMeshinfo" << endl;
	DWord length = currentChunk.length;
	DWord n = cfg3ds::chunkHeaderSize;

	Object *object = NULL;

	while (n < length)
	{
		readChunkHeader();
		n += currentChunk.length;
		switch (currentChunk.id)
		{
			case chunks::MESHINFO_HIERARCHY:
				char *name;
				Word flag1, flag2, hierarchy;
				object = NULL;
				
				readString(name);
				
//				if (strcmp(name, "$$$DUMMY") == 0)
//					break;
				
				read(flag1);
				read(flag2);
				read(hierarchy);
				cout << name << " " << static_cast<short int>(hierarchy) << endl;
				for (list<Object *>::const_iterator it = objects.begin(); it != objects.end(); ++it) {
					if (strcmp((*it)->name, name) == 0) {
						object = *it;
						break;
					}
				}
				
				if (object == NULL) {
					delete [] name;
					break;
				}
				
				if ((static_cast<short int>(hierarchy) <= rootLevel && strcmp(name, "$$$DUMMY") != 0) || previousObject == NULL) {
					cout << "adding root: " << name << endl;
					roots.push_back(object);
					parents.resize(static_cast<short int>(hierarchy)+2);
					parents[static_cast<short int>(hierarchy)+1] = object;
					currentParent = object;
					rootLevel = static_cast<short int>(hierarchy);
				} else {
					
					if (static_cast<short int>(hierarchy) > previousLevel) {
						currentParent = previousObject;
						parents.resize(hierarchy+1);
						parents[hierarchy] = currentParent;
					} else if (static_cast<short int>(hierarchy) < previousLevel)
						currentParent = parents[hierarchy];
					
					cout << "adding " << name << " " << object->selectName << " to " << currentParent->name << endl;
					currentParent->children.push_back(object);
				}
				
				for (int i=0; i<object->numVertices; i+=1) {
					cout << object->vertices[i].x << " " << object->vertices[i].y << " " << object->vertices[i].z << endl;
				}
				
				cout << object->u.x << " " << object->v.x << " " << object->w.x << " " << object->origin.x << endl;
				cout << object->u.y << " " << object->v.y << " " << object->w.y << " " << object->origin.y << endl;
				cout << object->u.z << " " << object->v.z << " " << object->w.z << " " << object->origin.z << endl;
				cout << 0.f << " " << 0.f << " " << 0.f << " " << 1.f << endl;
				
				previousLevel = static_cast<short int>(hierarchy);
				previousObject = object;
				
				delete [] name;
				break;
			
			case chunks::MESHINFO_PIVOT:
				if (object == NULL) {
					skipChunk();
					break;
				}
					
				read(object->pivot);
				cout << "pivot: " << object->pivot.x << " " << object->pivot.y << " " << object->pivot.z << endl;
			
				break;
				
			case chunks::MESHINFO_POSTRACK:
			case chunks::MESHINFO_ROTTRACK:
			case chunks::MESHINFO_SCALETRACK:
				if (object == NULL) {
					skipChunk();
					break;
				}
			
				Word flag;
				DWord unknown, keys;
				
				read(flag);
				read(unknown);
				read(unknown);
				read(keys);
				
				for (unsigned int i=0; i<keys; ++i) {
					DWord key;
					Word accelFlag;
					
					read(key);
					read(accelFlag);
					
					// Let's assume accelFlag is always 0 and skip to the track specific data
					
					switch (currentChunk.id) {
						case chunks::MESHINFO_POSTRACK:
							read(object->postrack);
							cout << "postrack:\t" << object->postrack.x << " " << object->postrack.y << " " << object->postrack.z << endl;
							break;
						
						case chunks::MESHINFO_ROTTRACK:
							read(object->rottrackAngle);
							read(object->rottrackAxis);
							cout << "rottrack:\t" << object->rottrackAxis.x << " " << object->rottrackAxis.y << " " << object->rottrackAxis.z << " " << object->rottrackAngle << endl;
							break;
							
						case chunks::MESHINFO_SCALETRACK:
							read(object->scaletrackX);
							read(object->scaletrackY);
							read(object->scaletrackZ);
							cout << "scaletrack:\t" << object->scaletrackX << " " << object->scaletrackY << " " << object->scaletrackZ << endl;
							break;
					}
				}
				
				break;
			
			default:
				skipChunk();
		}
	}
}

void Model3DS::parseColor(Color &color)
{
	cout << "parseColor" << endl;
	DWord length = currentChunk.length;
	DWord n = cfg3ds::chunkHeaderSize; 
	
	while (n < length)
	{
		readChunkHeader();
		n += currentChunk.length;
		switch (currentChunk.id)
		{
			case chunks::COLOR_FLOAT:
			case chunks::COLOR_FLOATG:
				fread(&color, sizeof(float), 3, fp);
				break;
			
			case chunks::COLOR_BYTE:
			case chunks::COLOR_BYTEG:
				Byte r, g, b;
				read(r);
				read(g);
				read(b);
				
				color.r = r/255.0;
				color.g = g/255.0;
				color.b = b/255.0;
				break;
			
			default:
				skipChunk();
		}
	}
}

size_t Model3DS::readString(char *&x)
{
	long int startPos = ftell(fp);
	int length = 1;
	int c;
	
	while ((c = fgetc(fp)) != '\0')
		++length;
	
	fseek(fp, startPos, SEEK_SET);
	x = new char[length];
	return fread(x, sizeof(char), length, fp);
}
