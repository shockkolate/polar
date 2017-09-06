#pragma once

namespace polar { namespace support { namespace ui { namespace control {
	class base {
	public:
		virtual ~base() {}
		virtual float get() { return 0; }
		virtual bool activate() { return false; }
		virtual bool navigate(int) { return false; }
		virtual std::shared_ptr<core::destructor> render(core::polar *engine, IDType &id, Point2, float) {
			return engine->addobject(&id);
		}
	};

	class button : public base {
	public:
		button() {}
		bool activate() override final { return true; }
	};

	class checkbox : public base {
	private:
		bool state;
	public:
		checkbox(bool initial = false) : state(initial) {}
		float get() override final { return state; }

		bool activate() override final {
			state = !state;
			return true;
		}

		bool navigate(int delta) override {
			// flip state delta times
			state ^= delta & 1;
			return true;
		}

		std::shared_ptr<core::destructor> render(core::polar *engine, IDType &id, Point2 origin, float scale) override final {
			auto dtor = engine->addobject(&id);

			Decimal pad = 15;
			Point2 offset = Point2(4 * scale, 0); // edgeOffset + edgePadding (SliderSprite)s
			Point4 color = state ? Point4(0, 1, 0, 1) : Point4(1, 0, 0, 1);

			engine->addcomponent_as<component::sprite::base, component::sprite::box>(id);
			engine->addcomponent<component::screenposition>(id, origin + pad + offset);
			engine->addcomponent<component::scale>(id, Point3(scale));
			engine->addcomponent<component::color>(id, color);

			return dtor;
		}
	};

	template<typename T> class slider : public base {
	private:
		T min;
		T max;
		T value;
		T step;
	public:
		slider(T min, T max, T initial = 0, T step = 1) : min(min), max(max), value(initial), step(step) {}
		float get() override final { return value; }
		bool activate() override final { return navigate(1); }

		bool navigate(int delta) override final {
			T newValue = glm::clamp(value + T(delta) * step, min, max);
			bool changed = newValue != value;
			value = newValue;
			return changed;
		}

		std::shared_ptr<core::destructor> render(core::polar *engine, IDType &id, Point2 origin, float scale) override final {
			auto dtor = engine->addobject(&id);

			Decimal pad = 15;
			float alpha = float(value - min) / float(max - min);

			engine->addcomponent_as<component::sprite::base, component::sprite::slider>(id, 12 * 8, 12, alpha);
			engine->addcomponent<component::screenposition>(id, origin + pad);
			engine->addcomponent<component::scale>(id, Point3(scale));

			return dtor;
		}
	};
} } } }
