#ifndef simplesketcher_h
#define simplesketcher_h

#include <gtk/gtk.h>

#include "drawinginterface.h"

class SimpleSketcher : public DrawingInterface
{
	public:
		SimpleSketcher(const FlatVector &ImageSize, int Downscale);
		SimpleSketcher(const String &Filename, const FlatVector &ImageSize, int Downscale);
		~SimpleSketcher(void);

		bool Save(const String &Filename);
		bool Export(const String &Filename, const Color &Foreground, const Color &Background, int Downscale);

		Region Mark(const CursorState &Start, const CursorState &End, const bool &Black);
		bool Render(const Region &Invalid, cairo_t *Destination,
			const Color &Foreground, const Color &Background);

		int Zoom(int Amount);
		FlatVector &GetDisplaySize(void);

		bool HasChanges(void);
		void Undo(void);
		void Redo(void);

	private:
		bool RenderInternal(const Region &Invalid, cairo_t *Destination, float Scale,
			const Color &Foreground, const Color &Background);

		Region ImageSpace;
		int ZoomOut; float ZoomScale;
		Region DisplaySpace;
		cairo_surface_t *Data;
		cairo_t *Context;
};

#endif
