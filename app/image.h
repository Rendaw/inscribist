// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#ifndef image_h
#define image_h

#include <cairo/cairo.h>
#include <deque>

#include "settings.h"
#include "cursorstate.h"

class UndoLevel;

struct RunData
{
	public:
		unsigned int const RowCount;
		unsigned int const Width;
		struct Run { unsigned int WhiteLength, BlackLength; /* White followed by black */ };
		struct Row { unsigned int RunCount; Run *Runs; } *Rows;

		RunData(const FlatVector &Size);
		~RunData(void);

		// Manipulation
		void Line(int Left, int Right, int const Y, bool Black);
		void Combine(unsigned int *Buffer,
			const unsigned int XStart, const unsigned int XEnd, const unsigned int Y, const unsigned int Scale);
		void FlipVertically(void);
		void FlipHorizontally(void);

		// Undo management
		bool IsUndoEmpty(void);
		void Undo(bool &FlippedHorizontally, bool &FlippedVertically);
		void Redo(bool &FlippedHorizontally, bool &FlippedVertically);
		void PushUndo(void);

	protected:
		friend class UndoLevel;
		void InternalFlipVertically(void);
		void InternalFlipHorizontally(void);

	private:
		UndoLevel *CurrentUndo;
		std::deque<UndoLevel *> UndoLevels, RedoLevels;
};

class UndoLevel
{
	public:
		UndoLevel(unsigned int const &RowCount);
		~UndoLevel(void);

		bool IsClean(void)
			{ return Clean; }

		void CopyFrom(RunData &Source, UndoLevel &Matching);

		void AddLine(RunData &Source, unsigned int const &LineNumber);
		void SetHorizontal(void);
		void SetVertical(void);

		void Apply(RunData &Destination, bool &OutFlippedHorizontally, bool &OutFlippedVertically);

	private:
		bool Clean;

		// Flip undo
		bool FlippedHorizontally;
		bool FlippedVertically;

		// Pixel undo
		unsigned int const RowCount;
		RunData::Row *Rows;
};

class Image
{
	public:
		Image(SettingsData &Settings);
		Image(SettingsData &Settings, const String &Filename);
		~Image(void);

		bool Save(String const &Filename);
		bool Export(String const &Filename);

		Region Mark(CursorState const &Start, CursorState const &End, bool const &Black);
		bool Render(Region const &Invalid, cairo_t *Destination);

		int Zoom(int Amount);
		FlatVector &GetDisplaySize(void);

		void FlipHorizontally(void);
		void FlipVertically(void);

		bool HasChanges(void);
		void PushUndo(void);
		void Undo(bool &FlippedHorizontally, bool &FlippedVertically);
		void Redo(bool &FlippedHorizontally, bool &FlippedVertically);

	private:
		SettingsData &Settings;

		bool RenderInternal(Region const &Invalid, cairo_t *Destination, int Scale,
			Color const &Foreground, Color const &Background);

		Region ImageSpace;
		unsigned int PixelsBelow;
		Region DisplaySpace;

		RunData *Data;

		bool ModifiedSinceSave;
};

#endif
