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
SDL_Texture* createRect(SDL_Renderer* renderer, int width, int height) {
	SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			SDL_WriteSurfacePixel(surface, x, y, 255, 255, 255, 255);
		}
	}

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
	rect = createRect(renderer, 1, 1);
	trainCarriage = createRect(renderer, 2, 1);
}

void Renderer::drawShape(TextureShape type, Vector2f center, float width, SDL_Color color, double rotation) {
	SDL_Texture* tex;
	float height = width;
	switch (type) {
		case TextureShape::Circle:
			tex = circle;
			break;
		case TextureShape::Square:
			tex = rect;
			break;
		case TextureShape::Triangle:
			tex = triangle;
			break;
		case TextureShape::Carriage:
			tex = rect;
			height = width / 1.5;
			break;
		case TextureShape::Bullet:
			tex = rect;
			height = width * 2;
			break;
	}
	auto rect = SDL_FRect{ center.x - (width / 2), center.y - (height / 2), width, height };
	auto point = SDL_FPoint{ rect.w / 2, rect.h / 2 };
	SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
	SDL_SetTextureAlphaMod(tex, color.a);
	SDL_RenderTextureRotated(this->renderer, tex, NULL, &rect, rotation, &point, SDL_FlipMode::SDL_FLIP_NONE);
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
	drawShape(TextureShape::Circle, points[1], 20, color);

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
		drawShape(TextureShape::Circle, { offset + i * spacing, (float)windowSize.y - 50 },
				  (i == world.selectedRail) ? circleSize * 1.25 : circleSize, world.railColors[i]);
	}
}
void Renderer::render(World& world) {
	SDL_RenderClear(renderer);

	for (const auto& line : world.lines) {
		for (const auto& seg : line.line) {
			drawTrace({ seg->stations[0]->getPos(), seg->midpoint, seg->stations[1]->getPos() }, line.color);
		}
		for (const auto& train : line.trains) {
			std::size_t index = line.getIndex(train.segment);
			auto [pos, angle] = train.getPos(line[index]);
			drawShape(TextureShape::Carriage, pos, 50, line.color, angle);
		}
	}

	if (world.activeDrag) {
		const auto& drag = *world.activeDrag;
		drawTrace({ drag.startStation->getPos(), drag.intercept, drag.current }, world.railColors[world.selectedRail]);
	}

	for (const auto& station : world.stations) {
		float size = 30;
		if (station.get() == world.hoverStation) size *= 1.25;

		drawShape(station->getType(), station->getPos(), size, SDL_Color{ 255, 255, 255, 255 });
	}

	drawOverlay(world);

	SDL_RenderPresent(renderer);
}