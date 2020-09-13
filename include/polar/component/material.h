#pragma once

#include <polar/component/base.h>

namespace polar::component {
	class material : public base {
	  public:
		core::ref stage;
		std::optional<core::ref> diffuse;

		material(core::ref stage, decltype(diffuse) diffuse = {}) : stage(stage), diffuse(diffuse) {}

		virtual std::string name() const override { return "material"; }
	};
} // namespace polar::component