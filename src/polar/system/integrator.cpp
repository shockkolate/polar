#include <polar/core/polar.h>
#include <polar/property/integrable.h>
#include <polar/system/event.h>
#include <polar/system/integrator.h>

namespace polar::system {
	void integrator::update(DeltaTicks &dt) {
		accumulator += dt;
		if(accumulator.Seconds() > 1.0f) { accumulator.SetSeconds(1.0f); }

		while(accumulator >= timestep) {
			tick(timestep.Seconds());
			accumulator -= timestep;
		}

		deltaMicroseconds = static_cast<uint_fast32_t>(accumulator.Seconds() * 1000000.0f);
	}

	void integrator::tick(DeltaTicks::seconds_type seconds) {
		for(auto it = engine->objects.left.begin();
		    it != engine->objects.left.end(); ++it) {
			auto property = it->info->get<property::integrable>().lock();
			if(property) {
				for(auto integrable : *property->get()) {
					integrable->integrate(seconds);
				}
			}
		}
		engine->get<event>().lock()->firein("integrator", "ticked", seconds);
	}

	void integrator::revert_by(size_t n) {
		for(auto it = engine->objects.left.begin();
		    it != engine->objects.left.end(); ++it) {
			auto property = it->info->get<property::integrable>().lock();
			if(property) {
				for(auto integrable : *property->get()) {
					integrable->revert_by(n);
				}
			}
		}
	}

	void integrator::revert_to(size_t n) {
		for(auto it = engine->objects.left.begin();
		    it != engine->objects.left.end(); ++it) {
			auto property = it->info->get<property::integrable>().lock();
			if(property) {
				for(auto integrable : *property->get()) {
					integrable->revert_to(n);
				}
			}
		}
	}
}
