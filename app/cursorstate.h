// Copyright 2013 Rendaw, under the FreeBSD license (See included license.txt)

#ifndef cursorstate_h
#define cursorstate_h

#include "ren-general/vector.h"

#include "settings.h"

struct CursorState
{
	FlatVector Position;
	float Radius;

	enum { mFree, mMarking, mPanning } Mode;
	void *LastDevice;
	DeviceSettings *Device;
	BrushSettings *Brush;

	CursorState(void);
};

#endif
