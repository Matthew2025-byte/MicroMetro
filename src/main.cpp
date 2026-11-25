#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>
#include <vector>

#include "Renderer.hpp"
#include "World.hpp"
#include "utils/Vector.hpp"

struct AppState {
	SDL_Window* window;
	std::unique_ptr<Renderer> renderer;

	World world;

	~AppState() {
		if (window) SDL_DestroyWindow(window);
	}
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	*appstate = new AppState();
	AppState& app = *static_cast<AppState*>(*appstate);
	SDL_Init(SDL_INIT_VIDEO);

	app.window = SDL_CreateWindow("MicroMetro", 1280, 720, NULL);
	app.renderer = std::make_unique<Renderer>(app.window);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	AppState& app = *static_cast<AppState*>(appstate);

	Vector2f mouse{ 0, 0 };
	SDL_GetMouseState(&mouse.x, &mouse.y);
	app.world.update(0.01, mouse);

	app.renderer->render(app.world);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	AppState& app = *static_cast<AppState*>(appstate);

	app.world.processEvent(event);

	if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) { delete static_cast<AppState*>(appstate); }