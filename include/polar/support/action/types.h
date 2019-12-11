#pragma once

#include <polar/core/ref.h>

namespace polar::support::action {
	struct digital {};
	struct analog {};

	using digital_function_t = std::function<void(core::ref)>;
	using analog_function_t  = std::function<void(core::ref, Decimal)>;
	using analog_predicate_t = std::function<bool(core::ref, Decimal, Decimal)>;
	using digital_cont_t     = std::function<bool(core::ref)>;
	using analog_cont_t      = std::function<bool(core::ref, Decimal)>;

	struct digital_data {
		std::unordered_map<core::ref, bool> states;
	};

	struct analog_state {
		Decimal initial = 0;
		Decimal previous = 0;
		Decimal value = 0;
		Decimal saved = 0;
	};

	struct analog_data {
		std::unordered_map<core::ref, analog_state> states;
	};

	using digital_map = std::unordered_map<std::type_index, digital_data>;
	using analog_map  = std::unordered_map<std::type_index, analog_data>;
}
