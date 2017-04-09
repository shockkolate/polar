#pragma once

#include "SDL/SDL.h"
#include "Key.h"

/* SDL defines main to be SDL_main which is annoying */
#ifdef main
#undef main
#endif

inline bool _SDL_real(const char *file, const long line, const char *code) {
	const char *err = SDL_GetError();
	if(err[0] != '\0') {
		ENGINE_DEBUGERROR("SDL: " << err,
						  "    " << file << ' ' << line << '\n' <<
						  "    " << code);
	}
	return err[0] == '\0';
}

#define ENGINE_SDL(CODE) ((CODE), _SDL_real(BASEFILE, __LINE__, #CODE))
#define SDL ENGINE_SDL

#ifdef _DEBUG
#define IGNORE_SDL(CODE) ((CODE), SDL_ClearError())
#else
#define IGNORE_SDL(CODE) (CODE)
#endif

inline Key mkKeyFromSDL(SDL_Keycode k) {
	switch(k) {
	case SDLK_a:
		return Key::A;
	case SDLK_b:
		return Key::B;
	case SDLK_c:
		return Key::C;
	case SDLK_d:
		return Key::D;
	case SDLK_e:
		return Key::E;
	case SDLK_f:
		return Key::F;
	case SDLK_g:
		return Key::G;
	case SDLK_h:
		return Key::H;
	case SDLK_i:
		return Key::I;
	case SDLK_j:
		return Key::J;
	case SDLK_k:
		return Key::K;
	case SDLK_l:
		return Key::L;
	case SDLK_m:
		return Key::M;
	case SDLK_n:
		return Key::N;
	case SDLK_o:
		return Key::O;
	case SDLK_p:
		return Key::P;
	case SDLK_q:
		return Key::Q;
	case SDLK_r:
		return Key::R;
	case SDLK_s:
		return Key::S;
	case SDLK_t:
		return Key::T;
	case SDLK_u:
		return Key::U;
	case SDLK_v:
		return Key::V;
	case SDLK_w:
		return Key::W;
	case SDLK_x:
		return Key::X;
	case SDLK_y:
		return Key::Y;
	case SDLK_z:
		return Key::Z;
	case SDLK_1:
		return Key::Num1;
	case SDLK_2:
		return Key::Num2;
	case SDLK_3:
		return Key::Num3;
	case SDLK_4:
		return Key::Num4;
	case SDLK_5:
		return Key::Num5;
	case SDLK_6:
		return Key::Num6;
	case SDLK_7:
		return Key::Num7;
	case SDLK_8:
		return Key::Num8;
	case SDLK_9:
		return Key::Num9;
	case SDLK_0:
		return Key::Num0;
	case SDLK_ESCAPE:
		return Key::Escape;
	case SDLK_SPACE:
		return Key::Space;
	default:
		return Key::None;
	}
}

inline Key mkButtonFromSDL(SDL_GameControllerButton b) {
	switch(b) {
	case SDL_CONTROLLER_BUTTON_A:
		return Key::ControllerA;
	case SDL_CONTROLLER_BUTTON_BACK:
		return Key::ControllerBack;
	default:
		return Key::None;
	}
}