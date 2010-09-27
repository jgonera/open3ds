#include "engine.h"

Engine::Engine()
{
	running = false;
	app = NULL;
	clock = NULL;
	
	frames = 0;
	lastTime = 0;
	
	lastMouseX = 0;
	lastMouseY = 0;
	
	rotatingCamera = false;
	cameraRotationX = 0.f;
	cameraRotationY = 0.f;
	
	positioningCamera = false;
	cameraPositionX = 0.f;
	cameraPositionY = 0.f;
	
	zoomingCamera = false;
	cameraDistance = 500.f;

	transformingObject = false;
	transformation = rotation;
	transformationAxis = x;
	
	drawAxes = true;
}

Engine::~Engine()
{
	delete model;
	delete app;
	delete clock;
}

void Engine::run()
{
	running = true;
	init();
	mainLoop();
}

void Engine::init()
{
	// Create the main window
	app = new sf::Window(sf::VideoMode(800, 600, 32), "3DS Loader");

	// Create a clock for measuring time elapsed
	clock = new sf::Clock;

	model = new Model3DS(cfg::modelName);
	if (!model->load("body.3ds")) {
		cout << "Can't find model file!" << endl;
		exit(1);
	}
	
	glSelectBuffer(cfg::selectBufferSize, selectBuffer);

	// Set color and depth clear value
	//glClearDepth(1.f);
	glClearColor(0.f, 0.f, 0.f, 1.f);

	// Enable Z-buffer read and write
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	
	// Light
	glEnable(GL_LIGHTING);
	
	glLightfv(GL_LIGHT0, GL_AMBIENT, cfg::ambientColor);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, cfg::diffuseColor);
	glLightfv(GL_LIGHT0, GL_SPECULAR, cfg::specularColor);

	glEnable(GL_LIGHT0);
}

void Engine::mainLoop()
{
	while (running) {
		processEvents();
		display();
	}
}

void Engine::display(bool select)
{
	// Setup a perspective projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	if (select) {
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		
		gluPickMatrix(static_cast<GLdouble>(lastMouseX), static_cast<GLdouble>(viewport[3] - lastMouseY), cfg::selectTolerance, cfg::selectTolerance, viewport);
	}
	
	gluPerspective(90.f, app->GetWidth()/static_cast<float>(app->GetHeight()), 1.f, 10000.f);
	
	// Clear color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Apply some transformations
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glTranslatef(cameraPositionX, cameraPositionY, -cameraDistance);
	glRotatef(cameraRotationX, 1.f, 0.f, 0.f);
	glRotatef(cameraRotationY, 0.f, 1.f, 0.f);
	
	// 3D artists normally use (0,0,1) UP vector instead of (0,1,0)
	glRotatef(-90, 1.f, 0.f, 0.f);

	// Axes
	if (!select && drawAxes) {
		glDisable(GL_LIGHTING);
		glLineWidth(2.f);
		
		glBegin(GL_LINES);
		
		glColor3f(1.f, 0.f, 0.f);
		glVertex3f(0.f, 0.f, 0.f);
		glVertex3f(cameraDistance, 0.f, 0.f);
		
		glColor3f(0.f, 1.f, 0.f);
		glVertex3f(0.f, 0.f, 0.f);
		glVertex3f(0.f, cameraDistance, 0.f);
		
		glColor3f(0.f, 0.f, 1.f);
		glVertex3f(0.f, 0.f, 0.f);
		glVertex3f(0.f, 0.f, cameraDistance);
		
		glEnd();
		
		glEnable(GL_LIGHTING);
	}

	// Draw
	model->draw();

	// Finally, display rendered frame on screen
	if (!select)
		app->Display();

	++frames;
	if (frames >= 1000) {
		elapsedTime = clock->GetElapsedTime();
		cout << "FPS: " << (frames/(elapsedTime-lastTime)) << endl;
		lastTime = elapsedTime;
		frames = 0;
	}
}

void Engine::processEvents()
{
	sf::Event event;
	
	while (app->GetEvent(event))
	{
		// Close window: exit
		if (event.Type == sf::Event::Closed)
			running = false;
			
		// Resize event: adjust viewport
		else if (event.Type == sf::Event::Resized) {
			glViewport(0, 0, event.Size.Width, event.Size.Height);
		}

		else if (event.Type == sf::Event::KeyPressed)
			processKeyPressed(event);
			
		else if (event.Type == sf::Event::MouseButtonPressed)
			processMouseButtonPressed(event);
			
		else if (event.Type == sf::Event::MouseButtonReleased)
			processMouseButtonReleased(event);
					
		else if (event.Type == sf::Event::MouseMoved)
			processMouseMoved(event);
			
		else if (event.Type == sf::Event::MouseWheelMoved)
			processMouseWheelMoved(event);
	}
}

void Engine::processKeyPressed(sf::Event &event)
{
	switch (event.Key.Code) {
		case sf::Key::Escape: running = false; break;
		
		case sf::Key::R: transformation = rotation; break;
		case sf::Key::T: transformation = translation; break;
		
		case sf::Key::X: transformationAxis = x; break;
		case sf::Key::Y: transformationAxis = y; break;
		case sf::Key::Z: transformationAxis = z; break;
		
		case sf::Key::A: drawAxes = !drawAxes; break;
		
		default: break;
	}
}

void Engine::processMouseButtonPressed(sf::Event &event)
{
	if (event.MouseButton.Button == sf::Mouse::Left) {
		glRenderMode(GL_SELECT);
		glInitNames();
		glPushName(cfg::emptySelectName);
		display(true);
		GLint nHits = glRenderMode(GL_RENDER);
		cout << "nhits: " << nHits << endl;
		
		GLint selectedName = -1;
		GLuint lowestZMin = 0xFFFFFFFF;
		
		for (int i = 0, index = 0; i < nHits; ++i) {
			GLint nItems = selectBuffer[index++];
			GLuint zMin = selectBuffer[index++];
			//GLuint zMax = selectBuffer[index++];
			++index;
						
			if (zMin < lowestZMin) {
				selectedName = selectBuffer[index+nItems-1];
				lowestZMin = zMin;
			}
			
			index += nItems;
		}
		
		model->select(selectedName);
		
		transformingObject = true;
	} else if (event.MouseButton.Button == sf::Mouse::Right)
		rotatingCamera = true;
	else if (event.MouseButton.Button == sf::Mouse::Middle)
		positioningCamera = true;
}

void Engine::processMouseButtonReleased(sf::Event &event)
{
	if (event.MouseButton.Button == sf::Mouse::Left)
		transformingObject = false;
	else if (event.MouseButton.Button == sf::Mouse::Right)
		rotatingCamera = false;
	else if (event.MouseButton.Button == sf::Mouse::Middle)
		positioningCamera = false;
}

void Engine::processMouseMoved(sf::Event &event)
{
	if (transformingObject) {
		if (transformation == rotation)
			model->rotateSelected(cfg::rotationSpeed * (static_cast<int>(event.MouseMove.Y) - lastMouseY), transformationAxis);
		else
			model->translateSelected(cfg::positionSpeed * cameraDistance * (static_cast<int>(event.MouseMove.Y) - lastMouseY), transformationAxis);
			
	} else if (rotatingCamera) {
		cameraRotationX += cfg::rotationSpeed * (static_cast<int>(event.MouseMove.Y) - lastMouseY);
		cameraRotationY += cfg::rotationSpeed * (static_cast<int>(event.MouseMove.X) - lastMouseX);
		
		if (cameraRotationX >= 360.f)
			cameraRotationX = 0.f;
		
		if (cameraRotationY >= 360.f)
			cameraRotationY = 0.f;
	}
	
	else if (positioningCamera) {
		cameraPositionX += cfg::positionSpeed * cameraDistance * (static_cast<int>(event.MouseMove.X) - lastMouseX);
		cameraPositionY -= cfg::positionSpeed * cameraDistance * (static_cast<int>(event.MouseMove.Y) - lastMouseY);
	}
	
//	else if (zoomingCamera) {
//		cameraDistance += cfg::zoomSpeed * (static_cast<int>(event.MouseMove.Y) - lastMouseY);
//	}
	
	lastMouseX = event.MouseMove.X;
	lastMouseY = event.MouseMove.Y;
}

void Engine::processMouseWheelMoved(sf::Event &event)
{
	cameraDistance -= cfg::zoomSpeed * cameraDistance * event.MouseWheel.Delta;
}
