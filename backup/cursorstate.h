#ifndef cursorstate_h
#define cursorstate_h

#include <general/vector.h>

#include "settings.h"

struct CursorState
{
	FlatVector Position;
	float Radius;

	enum { mFree, mMarking, mPanning } Mode;
	void *LastDevice;
	DeviceSettings *Settings;

	CursorState(void);
};

#endif
