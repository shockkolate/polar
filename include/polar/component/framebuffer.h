#pragma once

#include <polar/component/base.h>
#include <polar/core/ref.h>

namespace polar::component {
	class framebuffer : public base {
	  public:
		core::ref win;
		math::point4 clear_color;

		framebuffer(core::ref win) : win(win) {}

		virtual std::string name() const override { return "framebuffer"; }
	};
} // namespace polar::component
