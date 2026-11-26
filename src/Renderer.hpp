#pragma once
#include <SDL3/SDL.h>

#include "World.hpp"
#include "utils/Vector.hpp"

namespace {}
class Renderer {
   public:
	void render(World& world);

	Renderer(SDL_Window* window);

	void drawShape(TextureShape type, Vector2f center, float width, SDL_Color color, double rotation = 0);
	SDL_Renderer* getRenderer() { return renderer; }

   private:
	SDL_Renderer* renderer;
	static constexpr int Supersample = 4;
	SDL_Texture* target = nullptr;
	int targetW = 0, targetH = 0;

	SDL_Texture* circle;
	SDL_Texture* rect;
	SDL_Texture* triangle;
	SDL_Texture* trainCarriage;

	void drawPath(std::vector<Vector2f> points, float width, SDL_Color color);
	void drawOverlay(World& world);
	void ensureTarget();
};