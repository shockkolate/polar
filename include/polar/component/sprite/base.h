#pragma once

#include <polar/component/base.h>
#include <polar/util/sdl.h>

class Sprite : public Component {
public:
	SDL_Surface *surface = nullptr;
	bool freeSurface = true;

	virtual ~Sprite() {
		if(surface && freeSurface) { SDL(SDL_FreeSurface(surface)); }
	}

	virtual void Render() final {
		if(surface && freeSurface) { SDL(SDL_FreeSurface(surface)); }
		RenderMe();
	}

	virtual void RenderMe() = 0;
};