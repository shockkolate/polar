#pragma once

#include <array>
#include <polar/system/base.h>

class HumanPlayerController : public System {
private:
	std::shared_ptr<Destructor> timeDtor;
	IDType timeID = 0;

	// the size of the array determines how many concurrent sounds we can play at once
	std::array<std::shared_ptr<Destructor>, 4> soundDtors;
	size_t soundIndex = 0;

	IDType object;
	Point2 orientVel;
	Point2 orientRot;
	Decimal velocity = 10.0;
protected:
	virtual void Init() override;
	virtual void Update(DeltaTicks &) override;
public:
	Decimal oldTime = 0;
	Decimal smoothing = Decimal(0.995);

	static bool IsSupported() { return true; }
	HumanPlayerController(Polar *engine, const IDType object) : System(engine), object(object) {}
};