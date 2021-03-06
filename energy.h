
#ifndef ENERGY_H_
#define ENERGY_H_

#include "globalmin/gmstructure.h"

class Energy {
public:
	Energy() {}
	virtual ~Energy() {};

	virtual bool setup() = 0;
	virtual bool execute(Structure &structure) = 0;
	virtual bool cleanup() = 0;
};

#endif
