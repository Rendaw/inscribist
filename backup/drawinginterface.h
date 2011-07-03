#ifndef drawinginterface_h
#define drawinginterface_h

#include <cairo/cairo.h>

#include <general/color.h>
#include <general/region.h>

#include "cursorstate.h"

class DrawingInterface
{
	public:
		virtual ~DrawingInterface(void) {}
		virtual bool Save(const String &Filename) = 0;
		virtual bool Export(const String &Filename, const Color &Foreground, const Color &Background, int Downscale) = 0;

		virtual Region Mark(const CursorState &Start, const CursorState &End, const bool &Black) = 0;
		virtual bool Render(const Region &Invalid, cairo_t *Destination,
			const Color &Foreground, const Color &Background) = 0;

		virtual int Zoom(int Amount) = 0;
		virtual FlatVector &GetDisplaySize(void) = 0;

		// TODO for version 2.0 or something.  Is undo necessary?  Undo vs sensitivity -- if lots of strokes are started,
		// there will be lots of undo levels.
		virtual bool HasChanges(void) = 0;
		virtual void Undo(void) = 0;
		virtual void Redo(void) = 0;
};

#endif
