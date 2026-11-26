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
void Renderer::drawPath(std::vector<Vector2f> points, float width, SDL_Color color) {
	if (points.size() < 2) return;
	SDL_FColor col{ color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f };
	std::vector<SDL_Vertex> verts;
	std::vector<int> idx;
	float h = width / 2;

	for (std::size_t i = 0; i < points.size(); ++i) {
		Vector2f prev = points[i == 0 ? 0 : i - 1];
		Vector2f next = points[i + 1 < points.size() ? i + 1 : i];
		Vector2f dir = (next - prev).normalized();
		Vector2f n = { -dir.y, dir.x };
		Vector2f l = points[i] + n * h, r = points[i] - n * h;
		verts.push_back({ { l.x, l.y }, col, { 0, 0 } });
		verts.push_back({ { r.x, r.y }, col, { 0, 0 } });
		if (i > 0) {
			int k = int(i) * 2;
			idx.insert(idx.end(), { k - 2, k - 1, k, k - 1, k + 1, k });
		}
	}
	SDL_RenderGeometry(this->renderer, nullptr, verts.data(), int(verts.size()), idx.data(), int(idx.size()));
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
	ensureTarget();
	SDL_SetRenderTarget(renderer, target);
	SDL_SetRenderScale(renderer, Supersample, Supersample);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	for (const auto& line : world.lines) {
		for (const auto& seg : line.line) {
			drawPath(seg->path(), 20, line.color);
		}
		for (const auto& train : line.trains) {
			if (!train.segment) continue;
			auto [pos, angle] = train.getPos(*train.segment);
			drawShape(TextureShape::Carriage, pos, 50, line.color, angle);
		}
	}

	if (world.activeDrag) {
		const auto& drag = *world.activeDrag;
		drawPath(makePath(drag.startStation->getPos(), drag.intercept, drag.current), 20,
				 world.railColors[world.selectedRail]);
	}

	for (const auto& station : world.stations) {
		float size = 30;
		if (station.get() == world.hoverStation) size *= 1.25;

		drawShape(station->getType(), station->getPos(), size, SDL_Color{ 255, 255, 255, 255 });
	}

	SDL_SetRenderScale(renderer, 1, 1);
	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, target, NULL, NULL);

	drawOverlay(world);
	SDL_RenderPresent(renderer);
}

void Renderer::ensureTarget() {
	int w, h;
	SDL_GetCurrentRenderOutputSize(renderer, &w, &h);
	if (target && w == targetW && h == targetH) return;

	if (target) SDL_DestroyTexture(target);
	target =
		SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, w * Supersample, h * Supersample);
	SDL_SetTextureScaleMode(target, SDL_SCALEMODE_LINEAR);
	targetW = w;
	targetH = h;
}