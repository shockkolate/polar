#pragma once

#include <polar/system/base.h>
#include <polar/system/world.h>
#include <polar/component/position.h>
#include <polar/component/orientation.h>
#include <polar/component/playercamera.h>

class TitlePlayerController : public System {
private:
	IDType object;
	Point2 orientVel;
	Decimal accum;
	const Decimal timestep = Decimal(0.02);
protected:
	inline void Init() override {
		engine->AddComponent<PlayerCameraComponent>(object);
	}

	inline void Update(DeltaTicks &dt) override {
		accum += dt.Seconds();

		auto pos = engine->GetComponent<PositionComponent>(object);
		auto orient = engine->GetComponent<OrientationComponent>(object);
		auto camera = engine->GetComponent<PlayerCameraComponent>(object);
		auto world = engine->GetSystem<World>().lock();

		if(accum > timestep * 10) { accum = timestep * 10; }
		while(accum >= timestep) {
			accum -= timestep;

			orientVel *= static_cast<Decimal>(glm::pow(0.999, timestep * 1000.0));

			unsigned int i = 0;
			auto average = Point2(0);
			if(world) {
				for(Decimal x = -fieldOfView; x < fieldOfView; x += 0.5f) {
					for(Decimal y = -fieldOfView; y < fieldOfView; y += 0.5f) {
						for(Decimal d = 1; d < viewDistance; d += 0.5f) {
							auto p = Point4(x, y, -d, 1);
							auto abs = glm::inverse(orient->orientation) * glm::inverse(camera->orientation) * p;
							auto eval = world->Eval(pos->position.Get() + Point3(abs) / Decimal(WORLD_SCALE));
							if(eval) {
								average.x += Decimal(0.0005) * ((y >= 0) ? 1 : -1) / (glm::max(Decimal(1), d - 2) * 2 / viewDistance);
								average.y += Decimal(0.0005) * ((x <= 0) ? 1 : -1) / (glm::max(Decimal(1), d - 2) * 2 / viewDistance);
								++i;
								break;
							}
						}
					}
				}
			}

			if(i > 0) {
				average /= static_cast<Decimal>(i);
				if(average.length() < 0.1f && average.length() >= 0) { average = Point2(0,  1); }
				if(average.length() > 0.1f && average.length() <= 0) { average = Point2(0, -1); }
				orientVel += average;
			}
		}

		orient->orientation = Quat(Point3(orientVel.x, 0, 0)) * Quat(Point3(0, orientVel.y, 0)) * orient->orientation;

		const auto forward = glm::normalize(Point4(0, 0, -1, 1));
		auto abs = (glm::inverse(orient->orientation) * glm::inverse(camera->orientation) * forward) * velocity;

		pos->position.Derivative()->x = abs.x;
		pos->position.Derivative()->y = abs.y;
		pos->position.Derivative()->z = abs.z;
	}
public:
	const Decimal fieldOfView = 2;
	const Decimal viewDistance = 10.0f;
	const Decimal velocity = WORLD_DECIMAL(5.0);

	static bool IsSupported() { return true; }
	TitlePlayerController(Polar *engine, const IDType object) : System(engine), object(object) {}
};