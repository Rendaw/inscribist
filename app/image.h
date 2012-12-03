// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#ifndef image_h
#define image_h

#include <cairo/cairo.h>
#include <deque>

#include "settings.h"
#include "cursorstate.h"

class UndoLevel;

class Change
{
	public:
		enum class CombineResult
		{
			Fail,
			Nullify,
			Combine
		};

		virtual ~Change(void);
		virtual Change *Apply(bool &FlippedHorizontally, bool &FlippedVertically) = 0; // Returns undo change for this change
		virtual CombineResult Combine(Change *Other) = 0;
};

class ChangeManager
{
	public:
		void AddUndo(Change *Undo);
		bool CanUndo(void);
		void Undo(bool &FlippedHorizontally, bool &FlippedVertically);
		bool CanRedo(void);
		void Redo(bool &FlippedHorizontally, bool &FlippedVertically);
	private:
		DeleterStack<Change> Undos, Redos;
};

struct RunData
{
	public:
		typedef unsigned int Run; // Each row starts with white (first run is white)
		typedef std::vector<Run> RunArray;
		typedef std::vector<RunArray> RowArray;

		RowArray Rows;
		unsigned int const Width;

		RunData(const FlatVector &Size);
		RunData(std::vector<std::vector<Run> > const &InitialRows);

		/// Manipulation
		void Line(int Left, int Right, int const Y, bool Black);

		// Places counts black pixels in Buffer from 0 to BufferWidth
		// Counts come from the row of pixels on screen at X, Y (scale Scale)
		void Combine(unsigned int *Buffer,
			const unsigned int BufferWidth, const unsigned int X, const unsigned int Y, const unsigned int Scale);
		void FlipVertically(void);
		void FlipHorizontally(void);
		void ShiftHorizontally(int Columns);
		void ShiftVertically(int Rows);
	private:
		static bool IsBlack(unsigned int const &Index);
		void FlipSubsectionVertically(unsigned int const &Start, unsigned int const &Count);
};

class Mark : public Change
{
	public:
		Mark(RunData &Base);
		Change *Apply(bool &FlippedHorizontally, bool &FlippedVertically);
		CombineResult Combine(Change *Other);
		
		void AddLine(unsigned int const &LineNumber);
	private:
		RunData &Base;
		RunData::RowArray Rows;
};

class HorizontalFlip : public Change
{
	public:
		HorizontalFlip(RunData &Base);
		Change *Apply(bool &FlippedHorizontally, bool &FlippedVertically);
		CombineResult Combine(Change *Other);
	private:
		RunData &Base;
};

class VerticalFlip : public Change
{
	public:
		VerticalFlip(RunData &Base);
		Change *Apply(bool &FlippedHorizontally, bool &FlippedVertically);
		CombineResult Combine(Change *Other);
	private:
		RunData &Base;
};

class Shift : public Change
{
	public:
		Shift(RunData &Base);
		Change *Apply(bool &FlippedHorizontally, bool &FlippedVertically);
		CombineResult Combine(Change *Other);
	private:
		RunData &Base;
		int Right, Down;
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
		void FinishMark(void);
		bool Render(Region const &Invalid, cairo_t *Destination);

		int Zoom(int Amount);
		FlatVector &GetDisplaySize(void);

		void FlipHorizontally(void);
		void FlipVertically(void);

		bool HasChanges(void);
		void Undo(bool &FlippedHorizontally, bool &FlippedVertically);
		void Redo(bool &FlippedHorizontally, bool &FlippedVertically);

	private:
		SettingsData &Settings;

		void Operate(std::function<void(void)> &&Operation);

		bool RenderInternal(Region const &Invalid, cairo_t *Destination, int Scale,
			Color const &Foreground, Color const &Background);

		Region ImageSpace;
		unsigned int PixelsBelow;
		Region DisplaySpace;

		RunData *Data;
		ChangeManager Changes;
		Anchor< ::Mark> CurrentMarkUndo;

		bool ModifiedSinceSave;
};

#endif
