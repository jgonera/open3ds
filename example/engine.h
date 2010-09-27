#ifndef _ENGINE_H_
#define _ENGINE_H_

#include <vector>
#include <SFML/Window.hpp>
#include <GL/gl.h>
#include <GL/glu.h>
#include "../3ds.h"

using namespace std;

namespace cfg
{
	const GLfloat rotationSpeed = 0.5f;
	const GLfloat positionSpeed = 0.005f;
	const GLfloat zoomSpeed = 0.05f;
	
	const GLdouble selectTolerance = 2.0;
	const int selectBufferSize = 256;
	const GLint emptySelectName = 0xFFFFFFFF;
	
	const GLuint modelName = 0;
	
	const GLfloat ambientColor[] = {0.2f, 0.2f, 0.2f, 1.f};
	const GLfloat diffuseColor[] = {0.8f, 0.8f, 0.8f, 1.f};
	const GLfloat specularColor[] = {0.2f, 0.2f, 0.2f, 1.f};
}

enum Transformation { rotation, translation };

class Engine
{
	public:
		Engine();
		~Engine();
		void run();
	
	private:
		void init();
		void mainLoop();
		void display(bool select = false);
		void processEvents();
		void processKeyPressed(sf::Event &event);
		void processMouseButtonPressed(sf::Event &event);
		void processMouseButtonReleased(sf::Event &event);
		void processMouseMoved(sf::Event &event);
		void processMouseWheelMoved(sf::Event &event);
		
		bool running;
		sf::Window *app;
		sf::Clock *clock;
		Model3DS *model;
		
		unsigned int frames;
		float elapsedTime, lastTime;
		
		int lastMouseX, lastMouseY;
		
		bool rotatingCamera;
		GLfloat cameraRotationX, cameraRotationY;
		
		bool positioningCamera;
		GLfloat cameraPositionX, cameraPositionY;
		
		bool zoomingCamera;
		GLfloat cameraDistance;
		
		GLuint selectBuffer[cfg::selectBufferSize];
		bool transformingObject;
		Axis transformationAxis;
		Transformation transformation;
		
		bool drawAxes;
};

#endif // _ENGINE_H_
