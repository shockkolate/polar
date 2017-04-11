#pragma once

#include "System.h"

class Menu : public System {
private:
	std::array<IDType, 3> menuIDs;
	int currentID = 0;
	FontAsset font;

	// the size of the array determines how many concurrent sounds we can play at once
	std::array<boost::shared_ptr<Destructor>, 4> soundDtors;
	size_t soundIndex = 0;

	void Transition() {
		switch(currentID) {
		case 0:
			engine->transition = "forward";
		case 1:
			break;
		case 2:
			engine->Quit();
			break;
		}
	}

	void DownBy(int delta) {
		if(delta != 0) {
			auto assetM = engine->GetSystem<AssetManager>().lock();

			IDType soundID;
			soundDtors[soundIndex++] = engine->AddObject(&soundID);
			soundIndex %= soundDtors.size();
			engine->AddComponent<AudioSource>(soundID, assetM->Get<AudioAsset>("menu1"));

			engine->GetComponent<Text>(menuIDs[currentID])->color.b = 1.0f;
			currentID += delta;
			if(currentID < 0) { currentID += menuIDs.size(); }
			currentID %= menuIDs.size();
		}
	}
protected:
	void Init() override final {
		auto assetM = engine->GetSystem<AssetManager>().lock();
		auto inputM = engine->GetSystem<InputManager>().lock();
		auto tweener = engine->GetSystem<Tweener<float>>().lock();

		auto font = assetM->Get<FontAsset>("nasalization-rg");

		dtors.emplace_back(engine->AddObject(&menuIDs[0]));
		dtors.emplace_back(engine->AddObject(&menuIDs[1]));
		dtors.emplace_back(engine->AddObject(&menuIDs[2]));
		engine->AddComponent<Text>(menuIDs[0], font, "Solo Play", Point2(60, 170));
		engine->AddComponent<Text>(menuIDs[1], font, "Options",   Point2(60, 110));
		engine->AddComponent<Text>(menuIDs[2], font, "Quit Game", Point2(60, 50));
		engine->GetComponent<Text>(menuIDs[0])->scale *= 0.375f;
		engine->GetComponent<Text>(menuIDs[1])->scale *= 0.375f;
		engine->GetComponent<Text>(menuIDs[2])->scale *= 0.375f;

		dtors.emplace_back(inputM->On(Key::Down, [this] (Key) { DownBy(1); }));
		dtors.emplace_back(inputM->On(Key::S,    [this] (Key) { DownBy(1); }));

		dtors.emplace_back(inputM->On(Key::Up, [this] (Key) { DownBy(-1); }));
		dtors.emplace_back(inputM->On(Key::W,  [this] (Key) { DownBy(-1); }));

		dtors.emplace_back(tweener->Tween(0.0f, 1.0f, 0.25, true, [this] (Polar *engine, const float &x) {
			engine->GetComponent<Text>(menuIDs[currentID])->color.b = x;
		}));

		dtors.emplace_back(inputM->On(Key::Space,       [this] (Key) { Transition(); }));
		dtors.emplace_back(inputM->On(Key::Enter,       [this] (Key) { Transition(); }));
		dtors.emplace_back(inputM->On(Key::ControllerA, [this] (Key) { Transition(); }));
	}

	void Update(DeltaTicks &) override final {

	}
public:
	static bool IsSupported() { return true; }
	Menu(Polar *engine) : System(engine) {}
};
