#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>

#include "utils/Vector.hpp"

struct AppState {
	SDL_Window* window;
	SDL_Renderer* renderer;

	SDL_Texture* circle;
	SDL_Texture* rect;

	std::unique_ptr<Vector2f> initial;
	std::unique_ptr<Vector2f> current;

	~AppState() {
		if (circle) SDL_DestroyTexture(circle);
		if (rect) SDL_DestroyTexture(rect);
		if (renderer) SDL_DestroyRenderer(renderer);
		if (window) SDL_DestroyWindow(window);
	}
};

SDL_Texture* createCircle(SDL_Renderer* renderer, float radius) {
	SDL_Surface* surface = SDL_CreateSurface(radius * 2, radius * 2, SDL_PIXELFORMAT_RGBA32);
	const float radSq = radius * radius;
	for (int y = 0; y < radius * 2; ++y) {
		for (int x = 0; x < radius * 2; ++x) {
			if (Vector2f{ x - radius, y - radius }.sqrMagnitude() > radSq) continue;
			SDL_WriteSurfacePixel(surface, x, y, 255, 255, 255, 255);
		}
	}
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_DestroySurface(surface);
	return tex;
}
SDL_Texture* createRect(SDL_Renderer* renderer) {
	SDL_Surface* surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA32);
	SDL_WriteSurfacePixel(surface, 0, 0, 255, 255, 255, 255);

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_DestroySurface(surface);
	return tex;
}
void drawPoint(SDL_Renderer* renderer, SDL_Texture* texture, Vector2f point, float radius) {
	auto pointA = SDL_FRect{ point.x - radius, point.y - radius, 100, 100 };
	SDL_RenderTexture(renderer, texture, NULL, &pointA);
}
void drawLine(SDL_Renderer* renderer, SDL_Texture* texture, Vector2f a, Vector2f b, float width) {
	auto diff = b - a;
	float dist = diff.magnitude();

	double angle = std::atan2(diff.y, diff.x) * (180 / SDL_PI_D);

	SDL_FRect line = { a.x, a.y - width / 2, dist, width };
	SDL_FPoint rotationPoint = { 0, width / 2 };

	SDL_RenderTextureRotated(renderer, texture, NULL, &line, angle, &rotationPoint, SDL_FlipMode::SDL_FLIP_NONE);
}
Vector2f getIntercept(Vector2f a, Vector2f b) {
	Vector2f diff = b - a;
	Vector2f abs = { std::abs(diff.x), std::abs(diff.y) };
	Vector2f intercept = { b.x, a.y };
	if (abs.x > abs.y) intercept.x -= std::copysign(abs.y, diff.x);
	else intercept.y += std::copysign(abs.x, diff.y);

	return intercept;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	*appstate = new AppState();
	AppState& app = *static_cast<AppState*>(*appstate);
	SDL_Init(SDL_INIT_VIDEO);

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer("MicroMetro", 1280, 720, NULL, &app.window, &app.renderer);

	app.circle = createCircle(app.renderer, 100);
	app.rect = createRect(app.renderer);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	AppState& app = *static_cast<AppState*>(appstate);

	SDL_RenderClear(app.renderer);

	if (app.initial.get()) {
		SDL_GetMouseState(&app.current->x, &app.current->y);

		auto intercept = getIntercept(*app.initial, *app.current);

		drawPoint(app.renderer, app.circle, *app.initial, 50);
		drawPoint(app.renderer, app.circle, *app.current, 50);
		drawPoint(app.renderer, app.circle, intercept, 50);

		drawLine(app.renderer, app.rect, *app.initial, intercept, 100);
		drawLine(app.renderer, app.rect, intercept, *app.current, 100);
	}

	SDL_RenderPresent(app.renderer);
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	AppState& app = *static_cast<AppState*>(appstate);

	if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		app.initial = std::make_unique<Vector2f>(event->motion.x, event->motion.y);
		app.current = std::make_unique<Vector2f>(event->motion.x, event->motion.y);
	} else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
		app.initial.reset();
		app.current.reset();
	}

	else if (event->type == SDL_EVENT_QUIT)
		return SDL_APP_SUCCESS;
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) { delete static_cast<AppState*>(appstate); }