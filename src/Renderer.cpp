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

void Renderer::drawShape(Station::Type type, Vector2f center, float width, SDL_Color color) {
	SDL_Texture* tex;
	switch (type) {
		case Station::Circle:
			tex = circle;
			break;
		case Station::Square:
			tex = rect;
			break;
		case Station::Triangle:
			tex = triangle;
			break;
	}
	auto rect = SDL_FRect{ center.x - (width / 2), center.y - (width / 2), width, width };
	SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
	SDL_SetTextureAlphaMod(tex, color.a);
	SDL_RenderTexture(this->renderer, tex, NULL, &rect);
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
	drawShape(Station::Circle, points[1], 20, color);

	SDL_SetTextureColorMod(rect, color.r, color.g, color.b);
	SDL_SetTextureAlphaMod(rect, color.a);
	drawLine(rect, points[0], points[1], 20);
	drawLine(rect, points[1], points[2], 20);
}

void Renderer::drawOverlay(World& world) {
	Vector2i windowSize{ 0, 0 };
	SDL_GetRenderOutputSize(renderer, &windowSize.x, &windowSize.y);
	int circleSize = 30;
	const int spacing = 50;
	const float offset = (windowSize.x - (world.railColors.size() - 1) * spacing) / 2;
	for (int i = 0; i < world.railColors.size(); ++i) {
		drawShape(Station::Circle, { offset + i * spacing, windowSize.y - 50 },
				  (i == world.selectedRail) ? circleSize * 1.25 : circleSize, world.railColors[i]);
	}
}
void Renderer::render(World& world) {
	SDL_RenderClear(renderer);

	for (const auto& station : world.stations) {
		float size = 30;
		if (station.get() == world.hoverStation) size *= 1.25;

		drawShape(station->getType(), station->getPos(), size, SDL_Color{ 255, 255, 255, 255 });
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

	drawOverlay(world);

	SDL_RenderPresent(renderer);
}