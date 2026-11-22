#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "utils/Vector.hpp"

class Station {
   public:
	enum Type { Square, Circle, Triangle };

	const Vector2f& getPos() const { return pos; }
	Type getType() const { return type; }

	Station() = default;
	Station(Vector2f pos) : pos(pos), type(Circle) {}

   private:
	Vector2f pos;
	Type type;
};
struct RailSegment {
	Station* start;
	Station* end;
	Vector2f midpoint;

	RailSegment(Station* start, Station* end, Vector2f mid) : start(start), end(end), midpoint(mid) {}
};
struct AppState {
	SDL_Window* window;
	SDL_Renderer* renderer;

	SDL_Texture* circle;
	SDL_Texture* rect;
	SDL_Texture* triangle;

	std::optional<Vector2f> intercept;
	std::optional<Vector2f> current;

	bool mouseRight = false;
	Station* startStation;

	std::vector<std::unique_ptr<Station>> stations;
	std::vector<std::unique_ptr<RailSegment>> rails;

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
SDL_Texture* createTriangle(SDL_Renderer* renderer) { return nullptr; }
void drawShape(SDL_Renderer* renderer, SDL_Texture* texture, Vector2f center, float width, SDL_Color color) {
	SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
	SDL_SetTextureAlphaMod(texture, color.a);
	auto rect = SDL_FRect{ center.x - (width / 2), center.y - (width / 2), width, width };
	SDL_RenderTexture(renderer, texture, NULL, &rect);
}
void drawLine(SDL_Renderer* renderer, SDL_Texture* texture, Vector2f a, Vector2f b, float width) {
	auto diff = b - a;
	float dist = diff.magnitude();

	double angle = std::atan2(diff.y, diff.x) * (180 / SDL_PI_D);

	SDL_FRect line = { a.x, a.y - width / 2, dist, width };
	SDL_FPoint rotationPoint = { 0, width / 2 };

	SDL_RenderTextureRotated(renderer, texture, NULL, &line, angle, &rotationPoint, SDL_FlipMode::SDL_FLIP_NONE);
}
void drawTrace(SDL_Renderer* renderer, SDL_Texture* circle, SDL_Texture* rail, std::array<Vector2f, 3> points,
			   SDL_Color color) {
	drawShape(renderer, circle, points[0], 20, color);
	drawShape(renderer, circle, points[1], 20, color);
	drawShape(renderer, circle, points[2], 20, color);

	drawLine(renderer, rail, points[0], points[1], 20);
	drawLine(renderer, rail, points[1], points[2], 20);
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
	SDL_GetMouseState(&app.current->x, &app.current->y);

	SDL_RenderClear(app.renderer);

	if (app.current) {
		*app.intercept = getIntercept(app.startStation->getPos(), *app.current);
		drawTrace(app.renderer, app.circle, app.rect, { app.startStation->getPos(), *app.intercept, *app.current },
				  SDL_Color{ 255, 255, 255, 255 });
	}
	for (const auto& rail : app.rails) {
		drawTrace(app.renderer, app.circle, app.rect, { rail->start->getPos(), rail->midpoint, rail->end->getPos() },
				  SDL_Color{ 255, 255, 255, 255 });
	}
	for (auto& station : app.stations) {
		SDL_Texture* shape;
		switch (station->getType()) {
			case Station::Circle:
				shape = app.circle;
				break;
			case Station::Square:
				shape = app.rect;
				break;
			case Station::Triangle:
				shape = app.triangle;
				break;
		}
		float size = 20;
		auto dist = station->getPos() - *app.current;
		if (dist.magnitude() < size) {
			if (!app.startStation) {
				app.startStation = station.get();
			}
			if (station.get() != app.startStation) {
				app.rails.emplace_back(
					std::make_unique<RailSegment>(app.startStation, station.get(), app.intercept.value()));
				app.startStation = station.get();
			}
			size = 30;
		} else if (station.get() == app.startStation && !app.current) app.startStation = nullptr;

		drawShape(app.renderer, shape, station->getPos(), size, SDL_Color{ 255, 255, 255, 255 });
	}

	SDL_RenderPresent(app.renderer);
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	AppState& app = *static_cast<AppState*>(appstate);

	if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (event->button.button == SDL_BUTTON_LEFT && app.startStation) {
			app.current = app.startStation->getPos();
			app.intercept.emplace(0, 0);
		} else if (event->button.button == SDL_BUTTON_RIGHT && !app.mouseRight) {
			app.mouseRight = true;
			app.stations.emplace_back(std::make_unique<Station>(Vector2f{ event->button.x, event->button.y }));
		}
	} else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
		if (event->button.button == SDL_BUTTON_LEFT) {
			app.intercept.reset();
			app.current.reset();
			app.startStation = nullptr;
		} else if (event->button.button == SDL_BUTTON_RIGHT) app.mouseRight = false;
	}

	else if (event->type == SDL_EVENT_QUIT)
		return SDL_APP_SUCCESS;
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) { delete static_cast<AppState*>(appstate); }