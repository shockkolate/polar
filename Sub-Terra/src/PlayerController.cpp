#include "common.h"
#include "PlayerController.h"
#include "InputManager.h"
#include "PositionComponent.h"
#include "OrientationComponent.h"
#include "ModelComponent.h"

void PlayerController::Init() {
	const std::vector<Triangle> triangles = {
		std::make_tuple(Point3(-0.5, -0.5,  0.5), Point3( 0.5, -0.5,  0.5), Point3(-0.5,  0.5,  0.5)),
		std::make_tuple(Point3( 0.5, -0.5,  0.5), Point3( 0.5,  0.5,  0.5), Point3(-0.5,  0.5,  0.5)),
		std::make_tuple(Point3(-0.5,  0.5,  0.5), Point3( 0.5,  0.5,  0.5), Point3(-0.5,  0.5, -0.5)),
		std::make_tuple(Point3( 0.5,  0.5,  0.5), Point3( 0.5,  0.5, -0.5), Point3(-0.5,  0.5, -0.5)),
		std::make_tuple(Point3(-0.5,  0.5, -0.5), Point3( 0.5,  0.5, -0.5), Point3(-0.5, -0.5, -0.5)),
		std::make_tuple(Point3( 0.5,  0.5, -0.5), Point3( 0.5, -0.5, -0.5), Point3(-0.5, -0.5, -0.5)),
		std::make_tuple(Point3(-0.5, -0.5, -0.5), Point3( 0.5, -0.5, -0.5), Point3(-0.5, -0.5,  0.5)),
		std::make_tuple(Point3( 0.5, -0.5, -0.5), Point3( 0.5, -0.5,  0.5), Point3(-0.5, -0.5,  0.5)),
		std::make_tuple(Point3( 0.5, -0.5,  0.5), Point3( 0.5, -0.5, -0.5), Point3( 0.5,  0.5,  0.5)),
		std::make_tuple(Point3( 0.5, -0.5, -0.5), Point3( 0.5,  0.5, -0.5), Point3( 0.5,  0.5,  0.5)),
		std::make_tuple(Point3(-0.5, -0.5, -0.5), Point3(-0.5, -0.5,  0.5), Point3(-0.5,  0.5, -0.5)),
		std::make_tuple(Point3(-0.5, -0.5,  0.5), Point3(-0.5,  0.5,  0.5), Point3(-0.5,  0.5, -0.5))
	};

	object = engine->AddObject();
	engine->AddComponent<PositionComponent>(object);
	engine->AddComponent<OrientationComponent>(object);
	engine->AddComponent<ModelComponent>(object, triangles);
	InitObject();

	/* gravity */
	//auto pos = object->Get<PositionComponent>();
	//pos->position.Derivative(1) = Point(0, -9.8f, 0, 1);
}

void PlayerController::Update(DeltaTicks &dt) {
	auto pos = engine->GetComponent<PositionComponent>(object);
	auto orient = engine->GetComponent<OrientationComponent>(object);

	auto rel = glm::normalize(Point4((moveLeft ? -1 : 0) + (moveRight ? 1 : 0), 0, (moveForward ? -1 : 0) + (moveBackward ? 1 : 0), 1));
	auto abs = (glm::inverse(orient->orientation) * rel) * 2.0f * 16.0f * 0.25f;
	pos->position.Derivative()->x = abs.x;
	pos->position.Derivative()->y = abs.y;
	pos->position.Derivative()->z = abs.z;
}
