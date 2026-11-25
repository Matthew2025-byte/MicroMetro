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

	SDL_Texture* circle;
	SDL_Texture* rect;
	SDL_Texture* triangle;
	SDL_Texture* trainCarriage;

	void drawLine(SDL_Texture* texture, Vector2f a, Vector2f b, float width);
	void drawTrace(std::array<Vector2f, 3> points, SDL_Color color);
	void drawOverlay(World& world);
};