#pragma once
#include <memory>
#include <optional>
#include <vector>

#include "utils/Vector.hpp"

namespace {
Vector2f getIntercept(Vector2f a, Vector2f b) {
	Vector2f diff = b - a;
	Vector2f abs = { std::abs(diff.x), std::abs(diff.y) };
	Vector2f intercept = { b.x, a.y };
	if (abs.x > abs.y) intercept.x -= std::copysign(abs.y, diff.x);
	else intercept.y += std::copysign(abs.x, diff.y);

	return intercept;
}
}  // namespace

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
struct RailLine {
	std::vector<RailSegment> line;
	SDL_Color color{ 255, 255, 255, 255 };

	const RailSegment& operator[](std::size_t i) { return this->line[i % line.size()]; }

	auto begin() const { return line.begin(); }
	auto end() const { return line.end(); }
};

struct DragState {
	Station* startStation;
	Vector2f intercept;
	Vector2f current;

	SDL_Color lineColor{ 255, 255, 255, 255 };
};
struct World {
	std::vector<std::unique_ptr<Station>> stations;
	std::vector<RailLine> lines;

	Station* hoverStation = nullptr;
	std::optional<DragState> activeDrag;
	bool rightMouseDown = false;

	World() { lines.push_back(RailLine{ {}, { 255, 255, 255, 255 } }); }

	void update(Vector2f mousePos) {
		hoverStation = nullptr;
		for (auto& s : stations) {
			if ((s->getPos() - mousePos).magnitude() < 30) {
				hoverStation = s.get();
				break;
			}
		}

		if (activeDrag) {
			activeDrag->current = mousePos;
			activeDrag->intercept = getIntercept(activeDrag->startStation->getPos(), mousePos);

			if (hoverStation && hoverStation != activeDrag->startStation) {
				Vector2f midpoint = getIntercept(activeDrag->startStation->getPos(), hoverStation->getPos());
				lines[0].line.push_back({ activeDrag->startStation, hoverStation, midpoint });

				activeDrag->startStation = hoverStation;
			}
		}
	}
	void processEvent(const SDL_Event* event) {
		if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
			if (event->button.button == SDL_BUTTON_LEFT) {
				onLeftMouseDown();
			} else if (event->button.button == SDL_BUTTON_RIGHT && !rightMouseDown) {
				rightMouseDown = true;
				stations.emplace_back(std::make_unique<Station>(Vector2f{ event->button.x, event->button.y }));
			}
		} else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
			if (event->button.button == SDL_BUTTON_LEFT) {
				onLeftMouseUp();
			} else if (event->button.button == SDL_BUTTON_RIGHT) rightMouseDown = false;
		}
	}

	void onLeftMouseDown() {
		if (hoverStation) {
			activeDrag = DragState{ hoverStation, hoverStation->getPos(), hoverStation->getPos() };
		}
	}
	void onLeftMouseUp() { activeDrag.reset(); }
};