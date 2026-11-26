#pragma once
#include <array>
#include <cassert>
#include <cmath>
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

inline std::vector<Vector2f> makePath(Vector2f start, Vector2f mid, Vector2f end, float radius = 25.f) {
	constexpr float epsilon = 1e-3f;
	Vector2f v1 = mid - start, v2 = end - mid;
	float d1 = v1.magnitude(), d2 = v2.magnitude();
	if (d1 < epsilon || d2 < epsilon) return { start, mid, end };

	float phi = std::atan2(std::abs(v1.x * v2.y - v1.y * v2.x), v1.x * v2.x + v1.y * v2.y);
	if (phi < epsilon) return { start, mid, end };

	float tanLen = std::min(radius * std::tan(phi / 2), std::min(d1, d2));
	Vector2f dIn = v1 / d1, dOut = v2 / d2;
	float r = tanLen / std::tan(phi / 2);
	float sign = (dIn.x * dOut.y - dIn.y * dOut.x) > 0 ? 1.f : -1.f;
	Vector2f p1 = mid - dIn * tanLen;
	Vector2f center = p1 + Vector2f{ -dIn.y * sign, dIn.x * sign } * r;
	Vector2f v = p1 - center;

	std::vector<Vector2f> points{ start };
	int steps = std::max(4, int(r * phi / 4));
	for (int i = 0; i <= steps; ++i) {
		float angle = sign * phi * i / steps;
		float cs = std::cos(angle), sn = std::sin(angle);
		points.push_back(center + Vector2f{ v.x * cs - v.y * sn, v.x * sn + v.y * cs });
	}
	points.push_back(end);
	return points;
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
	std::array<Station*, 2> stations;
	Vector2f midpoint;
	std::array<bool, 2> end{ false, false };

	std::tuple<float, float> dist() const {
		float first = (stations[0]->getPos() - midpoint).magnitude();
		float second = (midpoint - stations[1]->getPos()).magnitude();
		return { first, second };
	}
	std::tuple<float, float> arc(float radius = 25) const {
		auto [d1, d2] = dist();
		constexpr float epsilon = 1e-3;
		if (d1 < epsilon || d2 < epsilon) return { 0.f, 0.f };

		Vector2f v1 = midpoint - stations[0]->getPos();
		Vector2f v2 = stations[1]->getPos() - midpoint;
		float phi = std::atan2(std::abs(v1.x * v2.y - v1.y * v2.x), v1.x * v2.x + v1.y * v2.y);
		if (phi < epsilon) return { 0.f, 0.f };
		float t = std::min(radius * std::tan(phi / 2), std::min(d1, d2));
		return { phi, t };
	}
	float length() const {
		auto [d1, d2] = dist();
		auto [phi, t] = arc();
		if (t == 0) return d1 + d2;
		float r = t / std::tan(phi / 2);
		return d1 + d2 - 2 * t + r * phi;
	}
	std::vector<Vector2f> path() const { return makePath(stations[0]->getPos(), midpoint, stations[1]->getPos()); }

	bool contains(Station* station) const {
		if (stations[0] == station || stations[1] == station) return true;
		return false;
	}

	RailSegment(Station* start, Station* end, Vector2f mid) : stations{ start, end }, midpoint(mid) {}
};
struct Train {
	Train(TextureShape type, RailSegment* pos) : type(type), segment(pos) {
		if (type == TextureShape::Carriage) speed = CarriageSpeed;
		else speed = BulletSpeed;
	}
	std::tuple<Vector2f, double> getPos(const RailSegment& seg) const {
		constexpr double toDegrees = 180 / SDL_PI_D;
		constexpr float epsilon = 1e-3;
		auto [d1, d2] = seg.dist();
		auto [phi, tanLen] = seg.arc();
		Vector2f start = seg.stations[0]->getPos(), end = seg.stations[1]->getPos(), mid = seg.midpoint;

		Vector2f dIn = d1 > epsilon ? (mid - start) / d1 : Vector2f{ 1, 0 };
		Vector2f dOut = d2 > epsilon ? (end - mid) / d2 : dIn;
		if (d1 <= epsilon) dIn = dOut;

		float leg1 = d1 - tanLen;
		if (dist < leg1 || tanLen == 0) {
			if (tanLen == 0 && dist >= d1) return { mid + dOut * (dist - d1), std::atan2(dOut.y, dOut.x) * toDegrees };
			return { start + dIn * dist, std::atan2(dIn.y, dIn.x) * toDegrees };
		}

		float radius = tanLen / std::tan(phi / 2);
		float arcLen = radius * phi;
		if (dist < leg1 + arcLen) {
			float sign = (dIn.x * dOut.y - dIn.y * dOut.x) > 0 ? 1 : -1;
			Vector2f p1 = mid - dIn * tanLen;
			Vector2f center = p1 + Vector2f{ -dIn.y * sign, dIn.x * sign } * radius;
			float ang = sign * (dist - leg1) / radius;
			float cs = std::cos(ang), sn = std::sin(ang);

			auto rot = [&](Vector2f v) { return Vector2f{ v.x * cs - v.y * sn, v.x * sn + v.y * cs }; };
			Vector2f dir = rot(dIn);
			return { center + rot(p1 - center), std::atan2(dir.y, dir.x) * toDegrees };
		}

		Vector2f p2 = mid + dOut * tanLen;
		return { p2 + dOut * (dist - leg1 - arcLen), std::atan2(dOut.y, dOut.x) * toDegrees };
	}

	static constexpr float BulletSpeed = 100;
	static constexpr float CarriageSpeed = 50;
	float dist = 0;
	float speed = 1;
	TextureShape type;
	RailSegment* segment = nullptr;
};
struct RailLine {
	std::vector<std::unique_ptr<RailSegment>> line;
	std::vector<Train> trains;
	SDL_Color color{ 255, 255, 255, 255 };
	bool loop = false;

	RailLine(SDL_Color color) : color(color) {}

	void update(float dt) {
		for (auto& train : trains) {
			train.dist += train.speed * dt;
			std::size_t index = getIndex(train.segment);
			const std::size_t n = line.size();

			float length = (*this)[index].length();
			if (train.dist > length) {
				if ((*this)[index].stations[1] != (*this)[index + 1].stations[0]) {
					train.speed *= -1;
					continue;
				}
				train.dist -= length;
				train.segment = &(*this)[index + 1];
				continue;
			} else if (train.dist < 0) {
				std::size_t prevIndex = (index + n - 1) % n;
				if ((*this)[prevIndex].stations[1] != (*this)[index - 1].stations[0]) {
					train.speed *= -1;
					continue;
				}
				train.segment = &(*this)[prevIndex - 1];
				train.dist += (*this)[prevIndex].length();
			}
		}
	}

	bool isEnd(Station* station) const {
		for (const auto& seg : line) {
			if (seg->stations[0] == station && seg->end[0]) return true;
			if (seg->stations[1] == station && seg->end[1]) return true;
		}
		return false;
	}
	void addSegment(RailSegment segment) {
		for (std::size_t i = 0; i < line.size(); ++i) {
			if (line[i]->stations[0] == segment.stations[1]) {
				insert(segment, i);
				updateEnd();
				return;
			}
		}
		append(segment);
		updateEnd();
	}
	std::size_t getIndex(const RailSegment* segment) const {
		for (std::size_t i = 0; i < line.size(); ++i) {
			if (line[i].get() == segment) return i;
		}
		assert(false && "Segment not present");
		return 0;
	}

	RailSegment& operator[](std::size_t i) const { return *this->line[i % line.size()]; }

	auto begin() const { return line.begin(); }
	auto end() const { return line.end(); }

   private:
	void insert(RailSegment segment, std::size_t index) {
		assert(index < line.size() && "Index out of bounds");

		if (index > 0) line[index - 1]->stations[1] = segment.stations[0];
		line[index]->stations[0] = segment.stations[1];
		line.insert(line.begin() + index, std::make_unique<RailSegment>(segment));
	}
	void append(RailSegment segment) {
		assert(!loop && "Cannot append a segment to a loop");
		if (line.size() == 0) {
			line.push_back(std::make_unique<RailSegment>(segment));
			return;
		}
		line.back()->stations[1] = segment.stations[0];

		line.push_back(std::make_unique<RailSegment>(segment));
	}
	void updateEnd() {
		for (auto& seg : line) {
			seg->end = { false, false };
		}
		if (line.front()->stations[0] == line.back()->stations[1]) {
			loop = true;
			return;
		}
		line.front()->end = { true, false };
		line.back()->end = { false, true };
	}
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
				lines[selectedRail].addSegment({ activeDrag->startStation, hoverStation, midpoint });
				activeDrag->startStation = hoverStation;

				// Spawn train
				if (lines[selectedRail].line.size() == 1) {
					lines[selectedRail].trains.emplace_back(TextureShape::Carriage,
															lines[selectedRail].line.front().get());
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
			if (lines[selectedRail].isEnd(hoverStation) || lines[selectedRail].line.empty())
				activeDrag = DragState{ hoverStation, hoverStation->getPos(), hoverStation->getPos() };
		}
	}
	void onLeftMouseUp() { activeDrag.reset(); }
};