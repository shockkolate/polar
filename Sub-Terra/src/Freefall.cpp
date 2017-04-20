#include "common.h"
#include <iomanip>
#include "Freefall.h"
#include <glm/gtc/random.hpp>
#include "Polar.h"
#include "JobManager.h"
#include "EventManager.h"
#include "AssetManager.h"
#include "InputManager.h"
#include "Integrator.h"
#include "Tweener.h"
#include "AudioManager.h"
#include "GL32Renderer.h"
#include "World.h"
#include "MenuSystem.h"
#include "TitlePlayerController.h"
#include "HumanPlayerController.h"
#include "Text.h"
#include "Level.h"

void Freefall::Run(const std::vector<std::string> &args) {
	const double secsPerBeat = 1.2631578947368421;
	Polar engine;
	IDType playerID;

	srand((unsigned int)time(0));
	std::mt19937_64 rng(time(0));

	engine.AddState("root", [] (Polar *engine, EngineState &st) {
		st.transitions.emplace("forward", Transition{Push("world"), Push("notplaying"), Push("title")});

		//st.AddSystem<JobManager>();
		st.AddSystem<EventManager>();
		st.AddSystem<InputManager>();
		st.AddSystem<AssetManager>();
		st.AddSystem<Integrator>();
		st.AddSystem<Tweener<float>>();
		st.AddSystem<AudioManager>();
		st.AddSystemAs<Renderer, GL32Renderer, const boost::container::vector<std::string> &>({ "perlin"/*, "fxaa", "bloom"*/ });

		auto assetM = engine->GetSystem<AssetManager>().lock();
		assetM->Get<AudioAsset>("nexus");
		assetM->Get<AudioAsset>("laser");
		assetM->Get<AudioAsset>("beep1");
		assetM->Get<AudioAsset>("menu1");
		assetM->Get<AudioAsset>("30");
		assetM->Get<AudioAsset>("60");
		assetM->Get<AudioAsset>("1");
		assetM->Get<AudioAsset>("2");
		assetM->Get<AudioAsset>("3");
		assetM->Get<AudioAsset>("4");
		assetM->Get<AudioAsset>("5");
		assetM->Get<AudioAsset>("6");
		assetM->Get<AudioAsset>("7");
		assetM->Get<AudioAsset>("8");
		assetM->Get<AudioAsset>("9");
		assetM->Get<AudioAsset>("seconds");
		assetM->Get<AudioAsset>("hundred");
		assetM->Get<AudioAsset>("fifty");
		assetM->Get<AudioAsset>("freefall");

		engine->transition = "forward";
	});

	engine.AddState("world", [&rng, &playerID] (Polar *engine, EngineState &st) {
		auto assetM = engine->GetSystem<AssetManager>().lock();

		st.dtors.emplace_back(engine->AddObject(&playerID));

		Point3 seed = glm::ballRand(Decimal(1000.0));
		engine->AddComponent<PositionComponent>(playerID, seed);
		engine->AddComponent<OrientationComponent>(playerID);

		st.AddSystem<World>(assetM->Get<Level>("1"), false);
	});

	engine.AddState("notplaying", [] (Polar *engine, EngineState &st) {
		auto assetM = engine->GetSystem<AssetManager>().lock();
		IDType laserID;
		st.dtors.emplace_back(engine->AddObject(&laserID));
		engine->AddComponent<AudioSource>(laserID, assetM->Get<AudioAsset>("laser"), true);
	});

	engine.AddState("title", [&playerID] (Polar *engine, EngineState &st) {
		st.transitions.emplace("forward", Transition{ Pop(), Pop(), Push("playing") });
		st.transitions.emplace("back", Transition{ QuitAction() });

		st.AddSystem<TitlePlayerController>(playerID);

		auto assetM = engine->GetSystem<AssetManager>().lock();
		auto inputM = engine->GetSystem<InputManager>().lock();
		auto tweener = engine->GetSystem<Tweener<float>>().lock();
		auto renderer = engine->GetSystem<Renderer>().lock();
		auto audioM = engine->GetSystem<AudioManager>().lock();

		Menu menu = {
			MenuItem("Solo Play", [engine] (Decimal) {
				engine->transition = "forward";
				return false;
			}),
			MenuItem("Options", {
				MenuItem("Graphics", {
					MenuItem("Base Detail", MenuControl::Slider<Decimal>(6, 40, renderer->GetUniformDecimal("u_baseDetail", 10)), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_baseDetail", x);
						return true;
					}),
					//MenuItem("Far Detail", MenuControl::Slider<Decimal>(), [] (Decimal) { return true; }),
					//MenuItem("Far Limiter", MenuControl::Slider<Decimal>(), [] (Decimal) { return true; }),
					//MenuItem("Precision", MenuControl::Selection({"Float", "Double"}), [] (Decimal) { return true; }),
					MenuItem("Pixel Factor", MenuControl::Slider<Decimal>(0, 20, renderer->GetUniformDecimal("u_pixelFactor", 0)), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_pixelFactor", x);
						return true;
					}),
					MenuItem("Voxel Factor", MenuControl::Slider<Decimal>(0, 20, renderer->GetUniformDecimal("u_voxelFactor", 0)), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_voxelFactor", x);
						return true;
					}),
					MenuItem("Show FPS", MenuControl::Checkbox(renderer->showFPS), [engine] (Decimal state) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->showFPS = state;
						return true;
					}),
				}),
				MenuItem("Audio", {
					MenuItem("Mute", MenuControl::Checkbox(audioM->muted), [engine] (Decimal state) {
						auto audioM = engine->GetSystem<AudioManager>().lock();
						audioM->muted = state;
						return true;
					}),
				}),
				//MenuItem("Controls", [] (Decimal) { return true; }),
				/*MenuItem("World", {
					MenuItem("u_baseThreshold", MenuControl::Slider<Decimal>(0, 1, renderer->GetUniformDecimal("u_baseThreshold"), 0.05), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_baseThreshold", x);
						return true;
					}),
					MenuItem("u_beatTicks", MenuControl::Slider<Decimal>(50, 10000, renderer->GetUniformDecimal("u_beatTicks"), 50), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_beatTicks", x);
						return true;
					}),
					MenuItem("u_beatPower", MenuControl::Slider<Decimal>(1, 16, renderer->GetUniformDecimal("u_beatPower")), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_beatPower", x);
						return true;
					}),
					MenuItem("u_beatStrength", MenuControl::Slider<Decimal>(-1, 1, renderer->GetUniformDecimal("u_beatStrength"), 0.002), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_beatStrength", x);
						return true;
					}),
					MenuItem("u_waveTicks", MenuControl::Slider<Decimal>(50, 10000, renderer->GetUniformDecimal("u_waveTicks"), 50), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_waveTicks", x);
						return true;
					}),
					MenuItem("u_wavePower", MenuControl::Slider<Decimal>(1, 16, renderer->GetUniformDecimal("u_wavePower")), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_wavePower", x);
						return true;
					}),
					MenuItem("u_waveStrength", MenuControl::Slider<Decimal>(-1, 1, renderer->GetUniformDecimal("u_waveStrength"), 0.002), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						renderer->SetUniform("u_waveStrength", x);
						return true;
					}),
					MenuItem("u_worldScale.x", MenuControl::Slider<Decimal>(1, 100, renderer->GetUniformPoint3("u_worldScale").x, 0.5), [engine] (Decimal x) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						auto p = renderer->GetUniformPoint3("u_worldScale");
						p.x = x;
						renderer->SetUniform("u_worldScale", p);
						return true;
					}),
					MenuItem("u_worldScale.y", MenuControl::Slider<Decimal>(1, 100, renderer->GetUniformPoint3("u_worldScale").y, 0.5), [engine] (Decimal y) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						auto p = renderer->GetUniformPoint3("u_worldScale");
						p.y = y;
						renderer->SetUniform("u_worldScale", p);
						return true;
					}),
					MenuItem("u_worldScale.z", MenuControl::Slider<Decimal>(1, 100, renderer->GetUniformPoint3("u_worldScale").z, 0.5), [engine] (Decimal z) {
						auto renderer = engine->GetSystem<Renderer>().lock();
						auto p = renderer->GetUniformPoint3("u_worldScale");
						p.z = z;
						renderer->SetUniform("u_worldScale", p);
						return true;
					}),
				}),*/
			}),
			MenuItem("Quit Game", [engine] (Decimal) {
				engine->Quit();
				return false;
			}),
		};
		st.AddSystem<MenuSystem>(menu);
	});

	engine.AddState("playing", [secsPerBeat, &playerID] (Polar *engine, EngineState &st) {
		st.transitions.emplace("back", Transition{Pop(), Pop(), Push("world"), Push("notplaying"), Push("title")});
		st.transitions.emplace("gameover", Transition{Pop(), Push("notplaying"), Push("gameover")});

		st.AddSystem<HumanPlayerController>(playerID);

		auto assetM = engine->GetSystem<AssetManager>().lock();
		auto inputM = engine->GetSystem<InputManager>().lock();
		auto tweener = engine->GetSystem<Tweener<float>>().lock();
		auto renderer = engine->GetSystem<Renderer>().lock();
		engine->GetSystem<World>().lock()->active = true;

		for(auto k : { Key::Escape, Key::Backspace, Key::MouseRight, Key::ControllerBack }) {
			st.dtors.emplace_back(inputM->On(k, [engine] (Key) { engine->transition = "back"; }));
		}

		IDType beepID;
		st.dtors.emplace_back(engine->AddObject(&beepID));
		engine->AddComponent<AudioSource>(beepID, assetM->Get<AudioAsset>("begin"));

		IDType musicID;
		st.dtors.emplace_back(engine->AddObject(&musicID));
		engine->AddComponent<AudioSource>(musicID, assetM->Get<AudioAsset>("nexus"), LoopIn{3565397});
	});

	engine.AddState("gameover", [] (Polar *engine, EngineState &st) {
		st.transitions.emplace("back", Transition{Pop(), Pop(), Pop(), Push("world"), Push("notplaying"), Push("title")});
		st.transitions.emplace("forward", Transition{Pop(), Pop(), Pop(), Push("world"), Push("playing")});

		auto assetM = engine->GetSystem<AssetManager>().lock();
		auto inputM = engine->GetSystem<InputManager>().lock();
		auto world = engine->GetSystem<World>().lock();

		for(auto k : { Key::Space, Key::Enter, Key::MouseLeft, Key::ControllerA }) {
			st.dtors.emplace_back(inputM->On(k, [engine] (Key) { engine->transition = "forward"; }));
		}

		for(auto k : { Key::Escape, Key::Backspace, Key::MouseRight, Key::ControllerBack }) {
			st.dtors.emplace_back(inputM->On(k, [engine] (Key) { engine->transition = "back"; }));
		}

		world->active = false;
		auto seconds = Decimal(world->GetTicks()) / ENGINE_TICKS_PER_SECOND;

		auto font = assetM->Get<FontAsset>("nasalization-rg");

		IDType textID;
		st.dtors.emplace_back(engine->AddObject(&textID));
		engine->AddComponent<Text>(textID, font, "Game Over", Point2(0), Origin::Center);

		std::ostringstream oss;
		oss << std::setiosflags(std::ios::fixed) << std::setprecision(1) << seconds << 's';

		IDType timeID;
		st.dtors.emplace_back(engine->AddObject(&timeID));
		engine->AddComponent<Text>(timeID, font, oss.str(), Point2(0, -150), Origin::Center);

		IDType beepID;
		st.dtors.emplace_back(engine->AddObject(&beepID));
		engine->AddComponent<AudioSource>(beepID, assetM->Get<AudioAsset>("gameover"));
	});

	engine.Run("root");
}
