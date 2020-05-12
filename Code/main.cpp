#include "SceneRenderer.h"
#include "InputManager.h"
#include "MathClass.h"

const int g_WindowSizeX = 1024, g_WindowSizeY = 768;

std::unique_ptr<SceneRenderer> g_SceneRenderer;
std::unique_ptr<InputManager> g_InputManager; 
std::shared_ptr<Camera> g_Camera;

void fn_SceneRenderer() {
	g_SceneRenderer->RenderScene();
}

void fn_KeyboardInput(unsigned char key, int x, int y) {
	g_InputManager->KeyboardInput(key);
}

void fn_MouseMotionInput(int x, int y) {
	g_InputManager->MouseMotionInput(x, y);
}

int main(int argc, char *argv[]) {


	//Set up scene renderer
	g_SceneRenderer = std::make_unique<SceneRenderer>(g_WindowSizeX, g_WindowSizeY, g_Camera);


	return 0;
}

