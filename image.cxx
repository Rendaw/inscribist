// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#include "image.h"

#include <cassert>
#include <stdint.h>
#include <cmath>
#include <iomanip>
#include <bzlib.h>
#include <cstring>

#include "localization.h"

unsigned int const MaxUndoLevels = 25;

//////////////////////////////////////////////////////////////////////////////////////////
// RLE data methods and storage
RunData::RunData(const FlatVector &Size) :
	RowCount(Size[1]), Width(std::max(Size[0], 1.0f)), Rows(new Row[RowCount]),
	CurrentUndo(new UndoLevel(RowCount))
{
	for (unsigned int CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
	{
		Rows[CurrentRow].RunCount = 1;
		Rows[CurrentRow].Runs = new Run[1];
		Rows[CurrentRow].Runs[0].WhiteLength = Width;
		Rows[CurrentRow].Runs[0].BlackLength = 0;
	}
}

RunData::~RunData(void)
{
	// Clean up image data
	for (unsigned int CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
		delete [] Rows[CurrentRow].Runs;
	delete [] Rows;

	// Clean up undo data
	delete CurrentUndo;
	while (!UndoLevels.empty())
	{
		delete UndoLevels.front();
		UndoLevels.pop_front();
	}

	while (!RedoLevels.empty())
	{
		delete RedoLevels.front();
		RedoLevels.pop_front();
	}
}

void RunData::Line(int UnclippedLeft, int UnclippedRight, int const Y, bool Black)
{
	// Validate parameters
	if (Y < 0) return;
	if ((unsigned int)Y >= RowCount) return;

	unsigned int const Right = RangeD(0, Width).Constrain(UnclippedRight);
	unsigned int const Left = std::min((unsigned int)RangeD(0, Right).Constrain(UnclippedLeft), Width - 1);

	if (Left == Right) return;

	// Now we know we'll me modifying the data (more or less) so save the original image data to the undo buffer
	if (CurrentUndo->IsClean())
		while (!RedoLevels.empty())
		{
			delete RedoLevels.front();
			RedoLevels.pop_front();
		}

	CurrentUndo->AddLine(*this, Y);

	// Create new row data
	// We'll add at most one run (white black sequence) to the row, so this may waste a run
	// but we'll clean it up next time we modify this row.
	Run *OldRuns = Rows[Y].Runs;
	unsigned int const OldRunCount = Rows[Y].RunCount;

	unsigned int &RunCount = Rows[Y].RunCount;
	RunCount = 0;
	unsigned int const AllocatedRunCount = OldRunCount + 1;
	Run *&Runs = Rows[Y].Runs = new Run[AllocatedRunCount];

	// Do a one-pass re-rowing, and calculate the actually required row length at the same time
	int SourcePosition = 0;
	Run *Current = Runs;
	Current->WhiteLength = 0;
	Current->BlackLength = 0;

	if (Black)
	{
		for (unsigned int OldRun = 0; OldRun < OldRunCount; OldRun++)
		{
			Run const &Source = OldRuns[OldRun];

			{
				// Handle coloring for the area that occupies the source white length
				unsigned int const &SourceStart = SourcePosition;
				unsigned int const SourceEnd = SourcePosition + Source.WhiteLength;

				if ((Right <= SourceStart) || (Left >= SourceEnd))
				{
					// The mark is out of bounds of this white strip
					Current->WhiteLength = Source.WhiteLength;
				}
				else if ((Left <= SourceStart) && (Right >= SourceEnd))
				{
					// The mark subsumes the white strip
					Current->BlackLength += Source.WhiteLength;
				}
				else if (Right >= SourceEnd)
				{
					// The mark starts here and continues to the right
					assert(Left > SourceStart);

					Current->WhiteLength = Left - SourceStart;
					Current->BlackLength = Source.WhiteLength - Current->WhiteLength;
				}
				else if (Left <= SourceStart)
				{
					// The mark started to the left and ends here
					assert(Right < SourceEnd);

					Current->BlackLength += Right - SourceStart;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
					Current->WhiteLength = SourceEnd - Right;
				}
				else
				{
					// The mark starts and stops in this white area
					assert((Left > SourceStart) && (Right < SourceEnd));

					Current->WhiteLength = Left - SourceStart;
					Current->BlackLength = Right - Left;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
					Current->WhiteLength = SourceEnd - Right;
				}
			}

			SourcePosition += Source.WhiteLength;

			{
				// Handle coloring for the area that occupies the source black length
				// If the mark butts up to this section but isn't in it, consider it to be part of
				// this section because the run lengths will run together.
				unsigned int const &SourceStart = SourcePosition;
				unsigned int const SourceEnd = SourceStart + Source.BlackLength;

				if ((Right < SourceStart) || (Left > SourceEnd))
				{
					// The mark is out of bounds (and not touching), so just copy
					Current->BlackLength = Source.BlackLength;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
				}
				else if ((Left < SourceStart) && (Right > SourceEnd))
				{
					// The mark spans this black section
					Current->BlackLength += Source.BlackLength;
				}
				else if (Right > SourceEnd)
				{
					// The mark starts here and continues to the right
					assert(Left >= SourceStart);

					Current->BlackLength = Source.BlackLength;
				}
				else if (Left < SourceStart)
				{
					// The mark started to the left and ends here
					assert(Right <= SourceEnd);

					Current->BlackLength += Source.BlackLength;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
				}
				else
				{
					// The mark starts and stops in this black area
					assert((Left >= SourceStart) && (Right <= SourceEnd));

					Current->BlackLength = Source.BlackLength;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
				}
			}

			SourcePosition += Source.BlackLength;
		}
	}
	else
	{
		bool Leftover = false;

		for (unsigned int OldRun = 0; OldRun < OldRunCount; OldRun++)
		{
			Run &Source = OldRuns[OldRun];

			{
				unsigned int const &SourceStart = SourcePosition;
				unsigned int const SourceEnd = SourcePosition + Source.WhiteLength;

				// The source we're looking at is white
				if ((Right < SourceStart) || (Left > SourceEnd))
				{
					// The marking area is either to the left or right of this area (and it doesn't touch),
					// so just copy the source without modifying it.
					Current->WhiteLength = Source.WhiteLength;
				}
				else if ((Left < SourceStart) && (Right > SourceEnd))
				{
					// The marking region spans this region (the current run started earlier
					// and doesn't end here)
					Current->WhiteLength += Source.WhiteLength;
				}
				else if (Right > SourceEnd)
				{
					// The mark starts here, continuing on
					assert(Left >= SourceStart);

					Current->WhiteLength = Source.WhiteLength;
				}
				else if (Left < SourceStart)
				{
					// The mark ends here
					assert(Right <= SourceEnd);

					Current->WhiteLength += Source.WhiteLength;
				}
				else
				{
					// The mark falls entirely within this white region
					assert((Left >= SourceStart) && (Right <= SourceEnd));

					Current->WhiteLength = Source.WhiteLength;
				}
			}

			SourcePosition += Source.WhiteLength;
			Leftover = false;

			{
				// Now we're considering the black region after the white region.
				unsigned int const &SourceStart = SourcePosition;
				unsigned int const SourceEnd = SourcePosition + Source.BlackLength;
				if ((Right <= SourceStart) || (Left >= SourceEnd))
				{
					// The white mark either ends to the left or starts to the right
					// (no interference, just copy over)
					Current->BlackLength = Source.BlackLength;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
				}
				else if ((Left <= SourceStart) && (Right >= SourceEnd))
				{
					// The white mark started earlier and doesn't stop here
					Current->WhiteLength += Source.BlackLength;
					Leftover = true;
				}
				else if (Right >= SourceEnd)
				{
					// The white mark starts here
					assert(Left > SourceStart);

					Current->BlackLength = Left - SourceStart;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
					Current->WhiteLength = SourceEnd - Left;
					Leftover = true;
				}
				else if (Left <= SourceStart)
				{
					// The white mark started to the left (and stops here)
					assert(Right < SourceEnd);

					Current->WhiteLength += Right - SourceStart;
					Current->BlackLength = SourceEnd - Right;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
				}
				else
				{
					// The white mark starts and stops here
					assert((Left > SourceStart) && (Right < SourceEnd));

					Current->BlackLength = Left - SourceStart;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
					Current->WhiteLength = Right - Left;
					Current->BlackLength = SourceEnd - Right;
					Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
				}
			}

			SourcePosition += Source.BlackLength;
		}

		// End any remaining white areas.  Is this a hack?
		if (Leftover)
		{
			Current->BlackLength = 0;
			Current++; RunCount++; assert(RunCount <= AllocatedRunCount);
		}
	}

#ifndef NDEBUG
	unsigned int BlackZeroCount = 0;
	unsigned int Position = 0;
	for (unsigned int CurrentRun = 0; CurrentRun < RunCount; CurrentRun++)
	{
		Run &Test = Runs[CurrentRun];
		Position += Test.WhiteLength + Test.BlackLength;
		if (Test.BlackLength == 0) BlackZeroCount++;
	}
	assert(Position == Width);
	assert(BlackZeroCount <= 1); // Last run
#endif

	// Delete the old data
	delete [] OldRuns;
}

void RunData::Combine(unsigned int *Buffer, unsigned int const BufferWidth,
	unsigned int const X, unsigned int const Y, unsigned int const Scale)
{
	// As much as possible we're doing everything in image space.
	// Here we convert all positions to image space.
	unsigned int const RowStart = Y * Scale, RowStop = std::min(Y * Scale + Scale, RowCount);
	unsigned int const ColumnStart = X * Scale, ColumnStop = std::min((X + BufferWidth) * Scale, Width);

	assert(ColumnStart < ColumnStop);

	/// Go through each underlying row and shade the buffer with dark pixels
	for (unsigned int CurrentRowIndex = RowStart; CurrentRowIndex < RowStop; CurrentRowIndex++)
	{
		RunData::Row const& CurrentRow = Rows[CurrentRowIndex];

		// Shade the black section of each run in the current row in the image
		unsigned int RunPosition = 0;
		Run const* CurrentRun = CurrentRow.Runs;
		for (unsigned int CurrentRunIndex = 0; CurrentRunIndex < CurrentRow.RunCount; CurrentRunIndex++, CurrentRun++)
		{
			// Figure out where the black starts and stops.
			unsigned int const BlackStart = RunPosition + CurrentRun->WhiteLength,
				BlackStop = BlackStart + CurrentRun->BlackLength;

			if (BlackStart >= ColumnStop) break; // Passed the drawing area
			if (BlackStop < ColumnStart)
			{
				// Not yet at the drawing area
				RunPosition = BlackStop;
				continue;
			}

			unsigned int ScreenStart = BlackStart / Scale,
				ScreenStop = BlackStop / Scale;

			assert(ScreenStop >= X);
			assert(ScreenStart <= ScreenStop);

			// Deal with the partially filled pixels at the beginning and end of the run,
			// and make sure the run coordinates are all within the invalid area.
			// Also, convert the screen coordinates to buffer coordinates.

			if (ScreenStart == ScreenStop)
			{
				// Both stop and start are at the same valid position
				ScreenStart -= X;
				ScreenStop -= X;
				assert(ScreenStart >= 0);
				assert(ScreenStart < BufferWidth);
				Buffer[ScreenStart] += CurrentRun->BlackLength;
				assert(Buffer[ScreenStart] < Scale * Scale + 1);
			}
			else
			{
				// The start must either be before the current area or in the current area.
				// Make it in the current area.
				if (BlackStart >= ColumnStart)
				{
					// Start is in the buffer area.  Darken whatever pixel it's over
					// and move it to the beginning of the assuredly filled region.
					assert(ScreenStart >= X);
					ScreenStart -= X;
					Buffer[ScreenStart] += Scale - BlackStart % Scale;
					assert(Buffer[ScreenStart] < Scale * Scale + 1);
					ScreenStart = ScreenStart + 1;
				}
				else ScreenStart = 0;

				// The stop must be in or past the current area.
				// Make it in the current area or the bound for the current area
				if (BlackStop < ColumnStop)
				{
					// Stop is in the buffer area.  Darken whatever pixel its over.
					// Stop is the exclusive bound for the fill region.
					assert(ScreenStop >= X);
					ScreenStop -= X;
					Buffer[ScreenStop] += BlackStop % Scale;
					assert(Buffer[ScreenStop] < Scale * Scale + 1);
				}
				else ScreenStop = BufferWidth;

				assert(ScreenStop <= BufferWidth);
				assert(ScreenStart <= ScreenStop);

				// Fill in the filled area inbetween the run start and stop
				for (unsigned int BufferPixel = ScreenStart; BufferPixel < ScreenStop; BufferPixel++)
				{
					Buffer[BufferPixel] += Scale;
					assert(Buffer[BufferPixel] < Scale * Scale + 1);
				}
			}

			// Move along
			RunPosition = BlackStop;
		}
	}
}

void RunData::FlipVertically(void)
{
	// Save the undo information
	if (!CurrentUndo->IsClean())
		PushUndo();
	CurrentUndo->SetVertical();
	PushUndo();

	// Do the flip
	InternalFlipVertically();
}

void RunData::FlipHorizontally(void)
{
	// Save the undo information
	if (!CurrentUndo->IsClean())
		PushUndo();
	CurrentUndo->SetHorizontal();
	PushUndo();

	// Do the flip
	InternalFlipHorizontally();
}

bool RunData::IsUndoEmpty(void)
	{ return UndoLevels.empty(); }

void RunData::Undo(bool &FlippedHorizontally, bool &FlippedVertically)
{
	if (!CurrentUndo->IsClean())
		PushUndo();

	if (UndoLevels.empty()) return;

	RedoLevels.push_back(new UndoLevel(RowCount));
	RedoLevels.back()->CopyFrom(*this, *UndoLevels.back());
	UndoLevels.back()->Apply(*this, FlippedHorizontally, FlippedVertically);
	delete UndoLevels.back();
	UndoLevels.pop_back();
}

void RunData::Redo(bool &FlippedHorizontally, bool &FlippedVertically)
{
	assert(!(!CurrentUndo->IsClean() && !RedoLevels.empty()));

	if (RedoLevels.empty()) return;

	UndoLevels.push_back(new UndoLevel(RowCount));
	UndoLevels.back()->CopyFrom(*this, *RedoLevels.back());
	RedoLevels.back()->Apply(*this, FlippedHorizontally, FlippedVertically);
	delete RedoLevels.back();
	RedoLevels.pop_back();
}

void RunData::PushUndo(void)
{
	//assert(!CurrentUndo->IsClean());
	if (CurrentUndo->IsClean()) return;

	UndoLevels.push_back(CurrentUndo);
	CurrentUndo = new UndoLevel(RowCount);

	if (UndoLevels.size() > MaxUndoLevels)
	{
		delete UndoLevels.front();
		UndoLevels.pop_front();
	}
	assert(UndoLevels.size() <= MaxUndoLevels);
}

void RunData::InternalFlipVertically(void)
{
	for (unsigned int CurrentRow = 0; CurrentRow < RowCount / 2; CurrentRow++)
	{
		Row Temp = Rows[CurrentRow];
		Rows[CurrentRow] = Rows[RowCount - CurrentRow - 1];
		Rows[RowCount - CurrentRow - 1] = Temp;
	}
}

void RunData::InternalFlipHorizontally(void)
{
	for (unsigned int CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
	{
		/// Allocate space for the flipped runs
		Run *OldRuns = Rows[CurrentRow].Runs;
		unsigned int const OldRunCount = Rows[CurrentRow].RunCount;

		unsigned int &NewRunCount = Rows[CurrentRow].RunCount;
		NewRunCount = 0;
		unsigned int const AllocatedRunCount = OldRunCount + 1;
		Rows[CurrentRow].Runs = new Run[AllocatedRunCount];

		/// Flip all the runs (offset them 1/2 run to the left, sort of)
		Run *New = Rows[CurrentRow].Runs;
		Run *Old = &OldRuns[OldRunCount - 1];
		New->WhiteLength = 0;

		unsigned int CurrentRun = 0;

		// Set up the starting conditions based on the end of the line
		if (Old->BlackLength == 0)
		{
			New->WhiteLength = Old->WhiteLength;
			Old--;
			CurrentRun++;
		}
		else New->WhiteLength = 0;

		// Go in opposite directions through the old and new runs and copy values over
		for (; CurrentRun < OldRunCount; CurrentRun++)
		{
			assert(Old >= OldRuns);
			New->BlackLength = Old->BlackLength;
			New++; NewRunCount++; assert(NewRunCount <= AllocatedRunCount);
			New->WhiteLength = Old->WhiteLength;
			Old--;
		}

		// Finish the last new run if it needs finishing
		if (New->WhiteLength > 0)
		{
			New->BlackLength = 0;
			New++; NewRunCount++; assert(NewRunCount <= AllocatedRunCount);
		}

		/// Delete the old data
		delete [] OldRuns;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// Undo levels
UndoLevel::UndoLevel(unsigned int const &RowCount) :
	Clean(true), FlippedHorizontally(false), FlippedVertically(false), RowCount(RowCount), Rows(NULL)
	{}

UndoLevel::~UndoLevel(void)
{
	if (Rows == NULL) return;

	for (unsigned int CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
		delete [] Rows[CurrentRow].Runs;
	delete [] Rows;
}

void UndoLevel::CopyFrom(RunData &Source, UndoLevel &Matching)
{
	assert(Clean);
	assert(!Matching.Clean);
	assert(RowCount == Matching.RowCount);
	assert(RowCount == Source.RowCount);

	if (Matching.FlippedHorizontally)
		FlippedHorizontally = true;

	if (Matching.FlippedVertically)
		FlippedVertically = true;

	if (Matching.Rows != NULL)
		for (unsigned int CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
			if (Matching.Rows[CurrentRow].Runs != NULL) AddLine(Source, CurrentRow);

	Clean = false;
}

void UndoLevel::AddLine(RunData &Source, unsigned int const &LineNumber)
{
	assert(LineNumber < Source.RowCount);
	assert(LineNumber < RowCount);

	// Initialize the pixel undo data if this undo data hasn't yet been touched
	if (Clean)
	{
		assert(!FlippedHorizontally);
		assert(!FlippedVertically);
		assert(Rows == NULL);

		Rows = new RunData::Row[RowCount];
		memset(Rows, 0, sizeof(RunData::Row) * RowCount);

		Clean = false;
	}

	// Only add lines if they haven't already been added at this undo level (keep the state at the beginning of the undo)
	if (Rows[LineNumber].Runs != NULL) return;

	RunData::Row &CurrentRow = Rows[LineNumber];
	RunData::Row const &SourceRow = Source.Rows[LineNumber];

	CurrentRow.RunCount = SourceRow.RunCount;
	CurrentRow.Runs = new RunData::Run[CurrentRow.RunCount];
	memcpy(CurrentRow.Runs, SourceRow.Runs, sizeof(RunData::Run) * CurrentRow.RunCount);
}

void UndoLevel::SetHorizontal(void)
{
	assert(Clean);
	FlippedHorizontally = true;
	Clean = false;
}

void UndoLevel::SetVertical(void)
{
	assert(Clean);
	FlippedVertically = true;
	Clean = false;
}

void UndoLevel::Apply(RunData &Destination, bool &OutFlippedHorizontally, bool &OutFlippedVertically)
{
	assert(!Clean);

	OutFlippedHorizontally = false;
	OutFlippedVertically = false;

	if (FlippedHorizontally)
	{
		// The internal function bypasses undo writing (don't modify the undo as we apply)
		OutFlippedHorizontally = true;
		Destination.InternalFlipHorizontally();
		return;
	}

	if (FlippedVertically)
	{
		OutFlippedVertically = true;
		Destination.InternalFlipVertically();
		return;
	}

	assert(RowCount == Destination.RowCount);
	assert(Rows != NULL);
	for (unsigned int CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
		if (Rows[CurrentRow].Runs != NULL)
		{
			RunData::Row const &SourceRow = Rows[CurrentRow];
			RunData::Row &DestinationRow = Destination.Rows[CurrentRow];

			delete [] DestinationRow.Runs;
			DestinationRow.RunCount = SourceRow.RunCount;
			DestinationRow.Runs = new RunData::Run[SourceRow.RunCount];

			memcpy(DestinationRow.Runs, SourceRow.Runs, sizeof(RunData::Run) * SourceRow.RunCount);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////
// Image manipulation/management
Image::Image(SettingsData &Settings) :
	Settings(Settings),
	ImageSpace(FlatVector(), Settings.ImageSize), PixelsBelow(Settings.DisplayScale),
	DisplaySpace(FlatVector(), ImageSpace.Size / (float)PixelsBelow),
	Data(new RunData(ImageSpace.Size)), ModifiedSinceSave(false)
	{}

Image::Image(SettingsData &Settings, String const &Filename) :
	Settings(Settings),
	ImageSpace(FlatVector(), Settings.ImageSize), PixelsBelow(Settings.DisplayScale),
	DisplaySpace(FlatVector(), ImageSpace.Size / (float)PixelsBelow),
	Data(NULL), ModifiedSinceSave(false)
{
	/// Open the file
	FILE *Input = fopen(Filename.c_str(), "rb");
	if (Input == NULL)
	{
		Data = new RunData(ImageSpace.Size);
		return;
	}

	char Identifier1[32];
	memset(Identifier1, 0, 32);
	strncpy(Identifier1, "inscribble v00\n", 32);

	char Identifier2[32];
	memset(Identifier2, 0, 32);
	strncpy(Identifier2, "inscribble v01\n", 32);

	char LoadedIdentifier[32];
	fread(LoadedIdentifier, sizeof(char), 32, Input);

	bool Version1 = false;
	if (strncmp(LoadedIdentifier, Identifier1, 32) == 0)
	{
		Version1 = true;
	}
	else if (strncmp(LoadedIdentifier, Identifier2, 32) != 0)
	{
		std::cerr << Local("The version string in the file is wrong.  Inscribist probably can't open this file.") << std::endl;
		return;
	}

	/// Prepare reading the bz2 data
	int Error;
	BZFILE *CompressInput = BZ2_bzReadOpen(&Error, Input, 0, 0, NULL, 0);
	if (Error != BZ_OK)
	{
		if (Error == BZ_IO_ERROR) std::cerr << Local("bz2 noticed the file had an error after being opened: ") << Filename << std::endl;
		if (Error == BZ_MEM_ERROR) std::cerr << Local("bz2 ran out of memory when opening file for reading: ") << Filename << std::endl;
		BZ2_bzReadClose(&Error, CompressInput);
		fclose(Input);
		return;
	}

	/// If v2 or greater, load image colors
	if (!Version1)
	{
		BZ2_bzRead(&Error, CompressInput, &Settings.DisplayPaper, sizeof(Settings.DisplayPaper));
		BZ2_bzRead(&Error, CompressInput, &Settings.DisplayInk, sizeof(Settings.DisplayInk));
		BZ2_bzRead(&Error, CompressInput, &Settings.ExportPaper, sizeof(Settings.ExportPaper));
		BZ2_bzRead(&Error, CompressInput, &Settings.ExportInk, sizeof(Settings.ExportInk));
	}

	/// Read in the image data
	unsigned int RowCount, Width;
	BZ2_bzRead(&Error, CompressInput, &RowCount, sizeof(RowCount));
	BZ2_bzRead(&Error, CompressInput, &Width, sizeof(Width));

	ImageSpace.Size[0] = Width;
	ImageSpace.Size[1] = RowCount;
	Data = new RunData(ImageSpace.Size);

	Settings.ImageSize = ImageSpace.Size;

	DisplaySpace.Size = ImageSpace.Size / (float)PixelsBelow;

	for (unsigned int CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
	{
		delete [] Data->Rows[CurrentRow].Runs;

		BZ2_bzRead(&Error, CompressInput, &Data->Rows[CurrentRow].RunCount, sizeof(Data->Rows[CurrentRow].RunCount));
		Data->Rows[CurrentRow].Runs = new RunData::Run[Data->Rows[CurrentRow].RunCount];
		BZ2_bzRead(&Error, CompressInput, Data->Rows[CurrentRow].Runs, sizeof(RunData::Run) * Data->Rows[CurrentRow].RunCount);
	}

	/// Close the file and finish up.
	BZ2_bzReadClose(&Error, CompressInput);
	fclose(Input);
}

Image::~Image(void)
{
	delete Data;
}

bool Image::Save(String const &Filename)
{
	/// Open the file
	FILE *Output = fopen(Filename.c_str(), "wb");
	if (Output == NULL)
	{
		std::cerr << Local("Could not open for writing: ") << Filename << std::endl;
		return false;
	}

	/// Add a simple header
	char Identifier[32];
	memset(Identifier, 0, 32);
	strncpy(Identifier, "inscribble v01\n", 32);

	fwrite(Identifier, sizeof(char), 32, Output);

	/// Prepare the bz2 stream
	int Error;
	BZFILE *CompressOutput = BZ2_bzWriteOpen(&Error, Output, 5, 0, 30);
	if (Error != BZ_OK)
	{
		if (Error == BZ_IO_ERROR) std::cerr << Local("bz2 noticed the file had an error after being opened: ") << Filename << std::endl;
		if (Error == BZ_MEM_ERROR) std::cerr << Local("bz2 ran out of memory when opening file for writing: ") << Filename << std::endl;
		BZ2_bzWriteClose(&Error, CompressOutput, 0, NULL, NULL);
		fclose(Output);
		return false;
	}

	/// Write paper/export colors
	Color DisplayPaper = Settings.DisplayPaper, DisplayInk = Settings.DisplayInk,
		ExportPaper = Settings.ExportPaper, ExportInk = Settings.ExportInk;

	BZ2_bzWrite(&Error, CompressOutput, &DisplayPaper, sizeof(DisplayPaper));
	BZ2_bzWrite(&Error, CompressOutput, &DisplayInk, sizeof(DisplayInk));
	BZ2_bzWrite(&Error, CompressOutput, &ExportPaper, sizeof(ExportPaper));
	BZ2_bzWrite(&Error, CompressOutput, &ExportInk, sizeof(ExportInk));

	/// Write the image data
	unsigned int RowCountBuffer = Data->RowCount;
	BZ2_bzWrite(&Error, CompressOutput, &RowCountBuffer, sizeof(RowCountBuffer));
	unsigned int WidthBuffer = Data->Width;
	BZ2_bzWrite(&Error, CompressOutput, &WidthBuffer, sizeof(WidthBuffer));
	for (unsigned int CurrentRow = 0; CurrentRow < Data->RowCount; CurrentRow++)
	{
		unsigned int RunCountBuffer = Data->Rows[CurrentRow].RunCount;
		BZ2_bzWrite(&Error, CompressOutput, &RunCountBuffer, sizeof(RunCountBuffer));

		unsigned int RunBufferSize = sizeof(RunData::Run) * Data->Rows[CurrentRow].RunCount;
		char *RunBuffer = new char[RunBufferSize];
		memcpy(RunBuffer, Data->Rows[CurrentRow].Runs, RunBufferSize);
		BZ2_bzWrite(&Error, CompressOutput, RunBuffer, RunBufferSize);
		delete [] RunBuffer;
	}

	/// Close everything
	BZ2_bzWriteClose(&Error, CompressOutput, 0, NULL, NULL);
	fclose(Output);

	ModifiedSinceSave = false;

	return true;
}

bool Image::Export(String const &Filename)
{
	int const &Scale = Settings.ExportScale;
	FlatVector const ExportSize(floor(ImageSpace.Size[0] / Scale), floor(ImageSpace.Size[1] / Scale));

	// Create an export surface
	cairo_surface_t *ExportSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ExportSize[0], ExportSize[1]);
	if (cairo_surface_status(ExportSurface) != CAIRO_STATUS_SUCCESS)
	{
		std::cerr << Local("Cairo failed when trying to create a temporary surface for exporting: ") <<
			cairo_status_to_string(cairo_surface_status(ExportSurface)) << std::endl;
		cairo_surface_destroy(ExportSurface);
		return false;
	}

	cairo_t *ExportContext = cairo_create(ExportSurface);

	// Scale and draw to the export surface
	if (!RenderInternal(Region(FlatVector(), ExportSize), ExportContext, Scale, Settings.ExportInk, Settings.ExportPaper))
	{
		cairo_destroy(ExportContext);
		cairo_surface_destroy(ExportSurface);
		return false;
	}

	// Save it and clean up
	cairo_status_t Result = cairo_surface_write_to_png(ExportSurface, Filename.c_str());
	if (Result != CAIRO_STATUS_SUCCESS)
	{
		std::cerr << Local("Cairo failed when trying to export the image surface to PNG: ") <<
			cairo_status_to_string(cairo_surface_status(ExportSurface)) << std::endl;
		cairo_destroy(ExportContext);
		cairo_surface_destroy(ExportSurface);
		return false;
	}

	cairo_destroy(ExportContext);
	cairo_surface_destroy(ExportSurface);

	return true;
}

Region Image::Mark(CursorState const &Start, CursorState const &End, bool const &Black)
{
	FlatVector const From(DisplaySpace.Transform(Start.Position, ImageSpace)),
		To(DisplaySpace.Transform(End.Position, ImageSpace));

	FlatVector const Difference = To - From;

	/// Calculate the connecting line parameters
	FlatVector const CirclePointOffset =
		(Difference.SquaredLength() > 1 ? Difference.Normal().QuarterRight() : FlatVector(1, 0));

	struct LineProperties
	{
		RangeF VerticalRange;

		bool IsHorizontal;

		bool StartIsLineUpper;

		float StartHorizontalPosition;
		float MovementPerRow;
		bool FacesLeft;

		LineProperties(FlatVector const &Start, FlatVector const &End, bool const &IsRightHandLine) :
			// The line hits the mark boundaries at these heights
			VerticalRange(Start[1], End[1]),

			// The rest of the values, other than the range, will be ignored
			IsHorizontal(VerticalRange.Length() < 1.0f),

			StartIsLineUpper(Start[1] < End[1]),

			// The line starts at the lower point
			StartHorizontalPosition(StartIsLineUpper ? Start[0] : End[0]),

			// This is the distance the line point moves horizontally between lines
			MovementPerRow(((StartIsLineUpper ? End[0] : Start[0]) - StartHorizontalPosition) / VerticalRange.Length()),

			// The line provides the left boundry
			// if the line is the right line and facing -y, or the line is the left line and facing +y (or something like that)
			FacesLeft(StartIsLineUpper != IsRightHandLine)

			{}

		void ConstrainMarkBounds(int &Left, int &Right, float &LinePosition, int const &CurrentRow) const
		{
			if (!IsHorizontal && VerticalRange.Contains(CurrentRow))
			{
				if (FacesLeft)
					Left = std::max(Left, (int)floor(LinePosition));
				else Right = std::min(Right, (int)ceil(LinePosition));
				LinePosition += MovementPerRow;
			}
		}
	} const
		Line1(
			FlatVector(From + CirclePointOffset * Start.Radius),
			FlatVector(To + CirclePointOffset * End.Radius),
			true),
		Line2(
			FlatVector(From - CirclePointOffset * Start.Radius),
			FlatVector(To - CirclePointOffset * End.Radius),
			false);

	int const
		LineBottom = 1 + std::min(
			Line1.VerticalRange.Min,
			Line2.VerticalRange.Min),
		LineTop = std::max(
			Line1.VerticalRange.Max,
			Line2.VerticalRange.Max),
		LineHorizontalMin = std::min(
			From[0] - fabs(CirclePointOffset[0]) * Start.Radius,
			To[0] - fabs(CirclePointOffset[0]) * End.Radius),
		LineHorizontalMax = std::max(
			From[0] + fabs(CirclePointOffset[0]) * Start.Radius,
			To[0] + fabs(CirclePointOffset[0]) * End.Radius);

	/// Calculate the cap circle parameters
	struct Cap
	{
		float Radius;
		FlatVector Center;

		int Bottom, Top;

		Cap(FlatVector const &Center, float const &Radius) :
			Radius(Radius), Center(Center),
			Bottom(-floor(Radius)), Top(floor(Radius))
			{}

		void ExpandMarkBounds(int &Left, int &Right, int const &CurrentRow) const
		{
			float const RelativeRow = CurrentRow - Center[1];
			if ((RelativeRow >= Bottom) && (RelativeRow <= Top))
			{
				assert(fabs(RelativeRow) <= Radius);
				float const CapWidth = sqrt(Radius * Radius - RelativeRow * RelativeRow);
				Left = std::min(Left, (int)floor(Center[0] - CapWidth));
				Right = std::max(Right, (int)ceil(Center[0] + CapWidth));
			}
		}
	} const
		FromCap(From, Start.Radius),
		ToCap(To, End.Radius);

	int const
		CapBelowStart = std::min(FromCap.Center[1] + FromCap.Bottom, ToCap.Center[1] + ToCap.Bottom),
		CapAboveEnd = std::max(FromCap.Center[1] + FromCap.Top, ToCap.Center[1] + ToCap.Top),
		CapHorizontalMin = std::min(FromCap.Center[0] - FromCap.Radius, ToCap.Center[0] - ToCap.Radius),
		CapHorizontalMax = std::max(FromCap.Center[0] + FromCap.Radius, ToCap.Center[0] + ToCap.Radius);

	/// Do the drawing
	for (int CurrentRow = CapBelowStart; CurrentRow < LineBottom; CurrentRow++)
	{
		int Left = CapHorizontalMax,
			Right = CapHorizontalMin;

		FromCap.ExpandMarkBounds(Left, Right, CurrentRow);
		ToCap.ExpandMarkBounds(Left, Right, CurrentRow);

		Data->Line(Left, Right, CurrentRow, Black);
	}

	float Line1Position = Line1.StartHorizontalPosition,
		Line2Position = Line2.StartHorizontalPosition;
	for (int CurrentRow = LineBottom; CurrentRow < LineTop; CurrentRow++)
	{
		int Left = LineHorizontalMin,
			Right = LineHorizontalMax;

		Line1.ConstrainMarkBounds(Left, Right, Line1Position, CurrentRow);
		Line2.ConstrainMarkBounds(Left, Right, Line2Position, CurrentRow);
		FromCap.ExpandMarkBounds(Left, Right, CurrentRow);
		ToCap.ExpandMarkBounds(Left, Right, CurrentRow);

		assert(Left <= Right);
		Data->Line(Left, Right, CurrentRow, Black);
	}

	for (int CurrentRow = LineTop; CurrentRow < CapAboveEnd; CurrentRow++)
	{
		int Left = CapHorizontalMax,
			Right = CapHorizontalMin;

		FromCap.ExpandMarkBounds(Left, Right, CurrentRow);
		ToCap.ExpandMarkBounds(Left, Right, CurrentRow);

		Data->Line(Left, Right, CurrentRow, Black);
	}

	ModifiedSinceSave = true;

	/// Return the marked area
	return ImageSpace.Transform(
		Region(
			FlatVector(
				std::min(From[0] - Start.Radius, To[0] - End.Radius),
				std::min(From[1] - Start.Radius, To[1] - End.Radius)),
			FlatVector(
				fabs(Difference[0]) + Start.Radius + End.Radius,
				fabs(Difference[1]) + Start.Radius + End.Radius)),
		DisplaySpace);
}

bool Image::Render(Region const &Invalid, cairo_t *Destination)
{
	return RenderInternal(DisplaySpace.Intersect(Region(Invalid.Start, Invalid.Size)),
		Destination, PixelsBelow, Settings.DisplayInk, Settings.DisplayPaper);
}

int Image::Zoom(int Amount)
{
	assert((Amount >= 0) || (fabs(Amount) <= PixelsBelow));
	PixelsBelow = std::max((unsigned int)1, (PixelsBelow + Amount));
	DisplaySpace.Size = ImageSpace.Size / (float)PixelsBelow;
	return PixelsBelow;
}

FlatVector &Image::GetDisplaySize(void)
	{ return DisplaySpace.Size; }

void Image::FlipHorizontally(void)
	{ Data->FlipHorizontally(); ModifiedSinceSave = true; }

void Image::FlipVertically(void)
	{ Data->FlipVertically(); ModifiedSinceSave = true; }

bool Image::HasChanges(void)
	{ return !Data->IsUndoEmpty() && ModifiedSinceSave; }

void Image::PushUndo(void)
	{ Data->PushUndo(); }

void Image::Undo(bool &FlippedHorizontally, bool &FlippedVertically)
	{ Data->Undo(FlippedHorizontally, FlippedVertically); }

void Image::Redo(bool &FlippedHorizontally, bool &FlippedVertically)
	{ Data->Redo(FlippedHorizontally, FlippedVertically); }

bool Image::RenderInternal(Region const &Invalid, cairo_t *Destination, int Scale,
	Color const &Foreground, Color const &Background)
{
	/// Prepare a buffer before we copy to the screen
	if ((Invalid.Size[0] < 1) || (Invalid.Size[1] < 1)) return true;
	unsigned int const
		InvalidX = Invalid.Start[0],
		InvalidY = Invalid.Start[1],
		InvalidWidth = Invalid.Size[0],
		InvalidHeight = Invalid.Size[1];

	int Stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, InvalidWidth);
	unsigned char *Pixels = new unsigned char[Stride * InvalidHeight];

	/// Figure out the shades for drawing the image
	// Every time we zoom out, 4 times the amount of source pixels will be part of one screen pixel,
	// so each pixel contributes less color.
	unsigned int ShadeCount = Scale * Scale + 1;

	float ShadeUnitScale = 1.0f / (float)(ShadeCount - 1);
	uint32_t *Colors = new uint32_t[ShadeCount]; // Shades between background and foreground colors. 0 is bg, Scale * Scale is fg.
	for (unsigned int CurrentColor = 0; CurrentColor < ShadeCount; CurrentColor++)
	{
		// Blend the two colors and cache the color in premultiplied form
		Color const Intermediary(Background, Foreground, (float)CurrentColor * ShadeUnitScale);
		Colors[CurrentColor] =
			(uint32_t)(Intermediary.Alpha * 0xff) << 24 |
			(uint32_t)(Intermediary.Red * Intermediary.Alpha * 0xff) << 16 |
			(uint32_t)(Intermediary.Green * Intermediary.Alpha * 0xff) << 8 |
			(uint32_t)(Intermediary.Blue * Intermediary.Alpha * 0xff) << 0;
	}

	/// Do scaling into the buffer one line at a time
	// Go through each screen row and accumulate shade wherever there is a "black pixel".
	// The shade count then corresponds to colors from the color map.
	unsigned int *LineShades = new unsigned int[InvalidWidth];

	void *CurrentPixelByte = Pixels;
	for (unsigned int CurrentRow = 0; CurrentRow < (unsigned int)InvalidHeight; CurrentRow++)
	{
		// Blank the row
		memset(LineShades, 0, sizeof(unsigned int) * InvalidWidth);

		// Add up underlying image lines
		Data->Combine(LineShades, InvalidWidth, InvalidX, InvalidY + CurrentRow, Scale);

		// Copy the row to the buffer
		uint32_t *CurrentPixel = (uint32_t *)CurrentPixelByte;
		for (unsigned int CurrentColumn = 0; CurrentColumn < InvalidWidth; CurrentColumn++)
		{
			*CurrentPixel = Colors[LineShades[CurrentColumn]];
			CurrentPixel++;
		}

		// Move to the next row
		CurrentPixelByte = (unsigned char *)CurrentPixelByte + Stride;
	}

	delete [] LineShades;
	delete [] Colors;

	/// Copy the buffer to the screen
	cairo_surface_t *CopySurface = cairo_image_surface_create_for_data(
		Pixels, CAIRO_FORMAT_ARGB32, InvalidWidth, InvalidHeight, Stride);
	if (cairo_surface_status(CopySurface) != CAIRO_STATUS_SUCCESS)
	{
		std::cerr << Local("Cairo failed when trying to create a temporary surface for rendering: ") <<
			cairo_status_to_string(cairo_surface_status(CopySurface)) << std::endl;
		cairo_surface_destroy(CopySurface);
		return false;
	}

	cairo_set_source_surface(Destination, CopySurface, InvalidX, InvalidY);
	cairo_paint(Destination);

	cairo_surface_destroy(CopySurface);
	delete [] Pixels;

	return true;
}
