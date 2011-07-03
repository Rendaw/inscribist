#ifndef runsketcher_h
#define runsketcher_h

#include "drawinginterface.h"

class RunSketcher : public DrawingInterface
{
	public:
		RunSketcher(const FlatVector &ImageSize, int Downscale);
		RunSketcher(const String &Filename, const FlatVector &ImageSize, int Downscale);
		~RunSketcher(void);

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
		bool RenderInternal(const Region &Invalid, cairo_t *Destination, int Scale,
			const Color &Foreground, const Color &Background);

		Region ImageSpace;
		unsigned int PixelsBelow;
		Region DisplaySpace;
		//cairo_surface_t *Data;
		//cairo_t *Context;

		struct StorageData
		{
			unsigned int const RowCount;
			unsigned int const Width;
			struct Run { unsigned int WhiteLength, BlackLength; /* White followed by black */ };
			struct Row { unsigned int RunCount; Run *Runs; } *Rows;

			StorageData(const FlatVector &Size);
			~StorageData(void);

			// Manipulation
			void Line(int Left, int Right, int const Y, bool Black);
			void Combine(unsigned int *Buffer,
				const unsigned int XStart, const unsigned int XEnd, const unsigned int Y, const unsigned int Scale);
		} *Data;
};

#endif
