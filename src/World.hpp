#pragma once
#include <memory>
#include <optional>
#include <tuple>
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
enum class TextureShape { Square, Circle, Triangle, Carriage, Bullet };
class Station {
   public:
	const Vector2f& getPos() const { return pos; }
	TextureShape getType() const { return type; }

	Station() = default;
	Station(Vector2f pos) : pos(pos), type(TextureShape::Circle) {}

   private:
	Vector2f pos;
	TextureShape type;
};
struct RailSegment {
	Station* start;
	Station* end;
	Vector2f midpoint;

	std::tuple<float, float> dist() const {
		float first = (start->getPos() - midpoint).magnitude();
		float second = (midpoint - end->getPos()).magnitude();
		return { first, second };
	}

	RailSegment(Station* start, Station* end, Vector2f mid) : start(start), end(end), midpoint(mid) {}
};
struct Train {
	Train(TextureShape type) : type(type) {
		if (type == TextureShape::Carriage) speed = CarriageSpeed;
		else speed = BulletSpeed;
	}
	std::tuple<Vector2f, double> getPos(const RailSegment& seg) const {
		auto [d1, d2] = seg.dist();
		Vector2f start, end;
		double localDist = dist;

		if (dist < d1) {
			start = seg.start->getPos();
			end = seg.midpoint;
		} else {
			start = seg.midpoint;
			end = seg.end->getPos();
			localDist -= d1;
		}
		auto diff = end - start;
		Vector2f dir = diff.normalized();
		double angle = std::atan2(diff.y, diff.x) * (180 / SDL_PI_D);
		return { start + dir * localDist, angle };
	}

	static constexpr float BulletSpeed = 100;
	static constexpr float CarriageSpeed = 50;
	float dist = 0;
	float speed = 1;
	TextureShape type;
	std::size_t current;
};
struct RailLine {
	std::vector<RailSegment> line;
	std::vector<Train> trains;
	SDL_Color color{ 255, 255, 255, 255 };

	RailLine(SDL_Color color) : color(color) {}

	// For this to work I am going to need RailSegment be a unique_ptr and store the current segment in Train instead of
	// an index so position is persistent when the line is modified
	void update(float dt) {
		for (auto& train : trains) {
			train.dist += train.speed * dt;
			auto [d1, d2] = (*this)[train.current].dist();
			float dist = d1 + d2;
			if (train.dist > dist) {
				if ((*this)[train.current].end != (*this)[train.current + 1].start) {
					train.speed *= -1;
					continue;
				}
				train.dist -= dist;
				++train.current;

			} else if (train.dist < 0) {
				if ((*this)[train.current].end != (*this)[train.current - 1].start) {
					train.speed *= -1;
					continue;
				}
				--train.current;
			}
		}
	}

	const RailSegment& operator[](std::size_t i) const { return this->line[i % line.size()]; }

	auto begin() const { return line.begin(); }
	auto end() const { return line.end(); }
};

struct DragState {
	Station* startStation;
	Vector2f intercept;
	Vector2f current;
};
struct World {
	std::vector<std::unique_ptr<Station>> stations;
	std::vector<RailLine> lines;

	Station* hoverStation = nullptr;
	std::optional<DragState> activeDrag;
	bool rightMouseDown = false;

	int selectedRail = 0;
	std::array<SDL_Color, 4> railColors;

	World() {
		railColors[0] = { 255, 50, 50, 255 };
		railColors[1] = { 0, 255, 0, 255 };
		railColors[2] = { 0, 0, 255, 255 };
		railColors[3] = { 255, 165, 0, 255 };

		for (const auto& color : railColors) {
			lines.emplace_back(RailLine(color));
		}
	}

	void update(float dt, Vector2f mousePos) {
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

			// Add RailSegment
			if (hoverStation && hoverStation != activeDrag->startStation) {
				Vector2f midpoint = getIntercept(activeDrag->startStation->getPos(), hoverStation->getPos());
				lines[selectedRail].line.push_back({ activeDrag->startStation, hoverStation, midpoint });
				activeDrag->startStation = hoverStation;

				// Spawn train
				if (lines[selectedRail].line.size() == 1) {
					lines[selectedRail].trains.emplace_back(TextureShape::Carriage);
				}
			}
		}

		for (auto& line : lines) {
			line.update(dt);
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

		if (event->type == SDL_EVENT_KEY_DOWN) {
			if (event->key.scancode == SDL_SCANCODE_1) selectedRail = 0;
			if (event->key.scancode == SDL_SCANCODE_2) selectedRail = 1;
			if (event->key.scancode == SDL_SCANCODE_3) selectedRail = 2;
			if (event->key.scancode == SDL_SCANCODE_4) selectedRail = 3;
		}
	}

	void onLeftMouseDown() {
		if (hoverStation) {
			activeDrag = DragState{ hoverStation, hoverStation->getPos(), hoverStation->getPos() };
		}
	}
	void onLeftMouseUp() { activeDrag.reset(); }
};