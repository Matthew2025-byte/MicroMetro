#include "Renderer.hpp"

#include "utils/Vector.hpp"

namespace {
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
}  // namespace

Renderer::Renderer(SDL_Window* window) {
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	renderer = SDL_CreateRenderer(window, NULL);

	circle = createCircle(renderer, 100);
	rect = createRect(renderer);
}

void Renderer::drawShape(SDL_Texture* texture, Vector2f center, float width, SDL_Color color) {
	auto rect = SDL_FRect{ center.x - (width / 2), center.y - (width / 2), width, width };
	SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
	SDL_SetTextureAlphaMod(texture, color.a);
	SDL_RenderTexture(this->renderer, texture, NULL, &rect);
}
void Renderer::drawLine(SDL_Texture* texture, Vector2f a, Vector2f b, float width) {
	auto diff = b - a;
	float dist = diff.magnitude();

	double angle = std::atan2(diff.y, diff.x) * (180 / SDL_PI_D);

	SDL_FRect line = { a.x, a.y - width / 2, dist, width };
	SDL_FPoint rotationPoint = { 0, width / 2 };

	SDL_RenderTextureRotated(this->renderer, texture, NULL, &line, angle, &rotationPoint, SDL_FlipMode::SDL_FLIP_NONE);
}
void Renderer::drawTrace(std::array<Vector2f, 3> points, SDL_Color color) {
	drawShape(circle, points[1], 20, color);

	SDL_SetTextureColorMod(rect, color.r, color.g, color.b);
	SDL_SetTextureAlphaMod(rect, color.a);
	drawLine(rect, points[0], points[1], 20);
	drawLine(rect, points[1], points[2], 20);
}

void Renderer::render(World& world) {
	SDL_RenderClear(renderer);

	for (const auto& station : world.stations) {
		SDL_Texture* shape;
		switch (station->getType()) {
			case Station::Circle:
				shape = circle;
				break;
			case Station::Square:
				shape = rect;
				break;
			case Station::Triangle:
				shape = triangle;
				break;
		}
		float size = 30;
		if (station.get() == world.hoverStation) size *= 1.25;

		drawShape(shape, station->getPos(), size, SDL_Color{ 255, 255, 255, 255 });
	}

	for (const auto& line : world.lines) {
		for (const auto& seg : line.line) {
			drawTrace({ seg.start->getPos(), seg.midpoint, seg.end->getPos() }, line.color);
		}
	}

	if (world.activeDrag) {
		const auto& drag = *world.activeDrag;
		drawTrace({ drag.startStation->getPos(), drag.intercept, drag.current }, drag.lineColor);
	}

	SDL_RenderPresent(renderer);
}