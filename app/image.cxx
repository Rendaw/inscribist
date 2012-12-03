// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#include "image.h"

#include <cassert>
#include <stdint.h>
#include <cmath>
#include <iomanip>
#include <bzlib.h>
#include <cstring>

#include "localization.h"

Change::~Change(void) {}
		
void ChangeManager::AddUndo(Change *Undo)
{
	while (!Redos.empty())
		Redos.pop();
	Change::CombineResult CombineResult;
	if (!CanUndo() || ((CombineResult = Undos.top()->Combine(Undo)) == Change::CombineResult::Fail))
	{
		Undos.push(Undo);
		return;
	}
	switch (CombineResult)
	{
		case Change::CombineResult::Nullify:
			Undos.pop();
			delete Undo;
			break;
		case Change::CombineResult::Combine:
			delete Undo;
		default: assert(false); break;
	}
}

bool ChangeManager::CanUndo(void) { return !Undos.empty(); }

void ChangeManager::Undo(bool &FlippedHorizontally, bool &FlippedVertically)
{
	assert(CanUndo());
	Redos.push(Undos.top()->Apply(FlippedHorizontally, FlippedVertically));
	Undos.pop();
}

bool ChangeManager::CanRedo(void) { return !Redos.empty(); }

void ChangeManager::Redo(bool &FlippedHorizontally, bool &FlippedVertically)
{
	assert(CanRedo());
	Undos.push(Redos.top()->Apply(FlippedHorizontally, FlippedVertically));
	Redos.pop();
}

//////////////////////////////////////////////////////////////////////////////////////////
// RLE data methods and storage
RunData::RunData(const FlatVector &Size) : Width(std::max(Size[0], 1.0f))
{
	Rows.resize(Size[1]);
	for (auto &Row : Rows)
	{
		Row.resize(1);
		Row[0] = Width;
	}
}
	
static unsigned int CalculateWidth(RunData::RowArray const &Rows)
{
	unsigned int Width = 0;
	bool WidthUnset = true;
	for (auto const &Row : Rows)
	{
		unsigned int TestWidth = 0;
		for (auto const &Run : Row) TestWidth += Run;
		if (WidthUnset)
		{
			Width = TestWidth;
			WidthUnset = false;
		}
		else
		{
			assert(TestWidth == Width);
		}
	}
	assert(Width >= 0);
	return Width;
}

RunData::RunData(std::vector<std::vector<Run> > const &InitialRows) : 
	Rows(InitialRows), Width(CalculateWidth(Rows))
	{ }

void RunData::Line(int UnclippedLeft, int UnclippedRight, int const Y, bool Black)
{
	// Validate parameters
	if (Y < 0) return;
	if ((unsigned int)Y >= Rows.size()) return;

	unsigned int const Right = RangeD(0, Width).Constrain(UnclippedRight);
	unsigned int const Left = RangeD(0, Right).Constrain(UnclippedLeft);

	if (Left == Right) return;

	// Create new row data
	// We'll add at most two runs to the row, so this may waste two runs
	// but we'll clean it up next time we modify this row.
	class NewRunManager
	{
		public:
			NewRunManager(RunData const &Base, unsigned int const &Y) : 
				MaxCount(Base.Rows[Y].size() + 2),
				MaxWidth(Base.Width), Width(0)
			{
				assert(MaxCount >= 2);
				NewRuns.reserve(MaxCount);
			}

			void Create(unsigned int Length)
			{
				assert((NewRuns.size() == 0) || (Length > 0));
				assert(NewRuns.size() < MaxCount);
				NewRuns.push_back(Length);
				assert(Width + Length <= MaxWidth);
				Width += Length;
			}

			unsigned int Right(void) const { return Width; }

			RunArray &&GetFinalRuns(void) { return std::move(NewRuns); }
		private:
			unsigned int const MaxCount;
			RunArray NewRuns;
			unsigned int const &MaxWidth;
			unsigned int Width;
	} NewRuns(*this, Y);

	{
		class OldRunManager
		{
			public:
				OldRunManager(RunData const &Base, unsigned int const &Y) : 
					OldRuns(Base.Rows[Y]), Count(0), Width(0)
				{ 
					Advance();
				}

				~OldRunManager(void)
				{
					assert(Count == OldRuns.size());
				}

				unsigned int Right(void) const { return Width; }

				unsigned int RunLength(void) const { return Length; }

				bool IsBlack(void) const { return RunData::IsBlack(Count - 1); }

				void Advance(void) 
				{
					assert(Count < OldRuns.size());
					Length = OldRuns[Count];
					Width += Length;
					++Count;
				}
			private:
				RunArray const &OldRuns;
				unsigned int Count;
				unsigned int Width;
				unsigned int Length;
		} OldRun(*this, Y);

		// Find the run, within or immediately after which the line starts.  Copy earlier runs over.
		while (OldRun.Right() < Left)
		{
			NewRuns.Create(OldRun.RunLength());
			OldRun.Advance();
		}

		// If the run doesn't match the line color, cut it short.  Mark the start of our line.
		int ExtendedLeft;
		if (OldRun.IsBlack() == Black)
			ExtendedLeft = NewRuns.Right();
		else 
		{
			ExtendedLeft = Left;
			NewRuns.Create(Left - NewRuns.Right());
		}
		
		// Skip over old runs that fall entirely behind the new runs.
		while ((OldRun.Right() < Width) && (OldRun.Right() <= Right))
			OldRun.Advance();

		// If the next run matches the new line color, incorporate it and finish the line.  Otherwise, finish the new line and shorten the next run.
		if (OldRun.IsBlack() == Black)
		{
			NewRuns.Create(OldRun.Right() - ExtendedLeft);
		}
		else
		{
			NewRuns.Create(Right - ExtendedLeft);
			if (Right < OldRun.Right())
				NewRuns.Create(OldRun.Right() - Right);
		}

		if (OldRun.Right() < Width)
		{
			// Advance past the run that touched the modified region
			OldRun.Advance();

			// Copy the remaining old runs over
			while (true)
			{
				NewRuns.Create(OldRun.RunLength());
				if (OldRun.Right() == Width) break;
				OldRun.Advance();
			}
		}
		assert(OldRun.Right() == Width);
	}

	assert(NewRuns.Right() == Width);
		
	Rows[Y] = NewRuns.GetFinalRuns();
}

void RunData::Combine(unsigned int *Buffer, unsigned int const BufferWidth,
	unsigned int const X, unsigned int const Y, unsigned int const Scale)
{
	// As much as possible we're doing everything in image space.
	// Here we convert all positions to image space.
	unsigned int const 
		RowStart = Y * Scale, 
		RowStop = std::min(Y * Scale + Scale, (unsigned int)Rows.size());
	unsigned int const 
		BufferLeft = X * Scale,
		BufferRight = std::min(BufferLeft + BufferWidth * Scale, Width); 

	/// Go through each underlying row and shade the buffer with dark pixels
	for (unsigned int CurrentRowIndex = RowStart; CurrentRowIndex < RowStop; CurrentRowIndex++)
	{
		assert(BufferWidth >= 1);
		unsigned int 
			BufferColumn = 0, 
			BufferColumnLeft = BufferLeft, // Inclusive
			BufferColumnRight = BufferColumnLeft + Scale; // Exclusive

		RunArray const &CurrentRow = Rows[CurrentRowIndex];
		assert(CurrentRow.size() >= 1);
		unsigned int 
			RunIndex = 0,
			RunLeft = 0, // Inclusive
			RunRight = CurrentRow[RunIndex]; // Exclusive
		while (true)
		{
			if (RunLeft >= BufferRight) break;
			if (IsBlack(RunIndex))
			{
				while (BufferColumnLeft < RunRight)
				{
					if (BufferColumnRight > RunLeft)
					{
						Buffer[BufferColumn] += std::min(RunRight, BufferColumnRight) - std::max(RunLeft, BufferColumnLeft);
					}
					++BufferColumn;
					if (BufferColumn >= BufferWidth) goto RowEndMark;
					BufferColumnLeft += Scale;
					BufferColumnRight += Scale;
				}
			}
			++RunIndex;
			if (RunIndex >= CurrentRow.size()) break;
			RunLeft = RunRight;
			RunRight += CurrentRow[RunIndex];
		}
RowEndMark:;
	}
}

void RunData::FlipVertically(void) { FlipSubsectionVertically(0, Rows.size()); }

void RunData::FlipHorizontally(void)
{
	for (unsigned int CurrentRow = 0; CurrentRow < Rows.size(); CurrentRow++)
	{
		RunArray OldRuns; 
		Rows[CurrentRow].swap(OldRuns); // Could we just move constructor and clear instead?
		
		// If the last element was black, add a 0 width white to start the new row
		unsigned int const WriteStartOffset = IsBlack(OldRuns.size() - 1) ? 1 : 0;

		// If the first element of the old is 0, skip it
		unsigned int const ReadStartOffset = (OldRuns[0] == 0) ? 1 : 0;

		Rows[CurrentRow].resize(OldRuns.size() + WriteStartOffset - ReadStartOffset);

		if (WriteStartOffset == 1)
			Rows[CurrentRow][0] = 0;

		auto SetNewRun = [&](unsigned int const &NewIndex, unsigned int const &OldIndex)
		{
			assert(NewIndex < Rows[CurrentRow].size());
			assert(NewIndex >= WriteStartOffset);
			assert(OldIndex < OldRuns.size());
			assert(OldIndex >= ReadStartOffset);
			Rows[CurrentRow][NewIndex] = OldRuns[OldIndex];
		};
		for (unsigned int NewRun = 0; NewRun < Rows[CurrentRow].size() - WriteStartOffset; ++NewRun)
			SetNewRun(WriteStartOffset + NewRun, OldRuns.size() - 1 - NewRun);
	}
}

void RunData::ShiftHorizontally(int Columns)
{
	/*unsigned int const Split = Columns % Width;
	for (unsigned int CurrentRow = 0; CurrentRow < Rows.size(); CurrentRow++)
	{
		Run *OldRuns = Rows[CurrentRow].Runs;
		unsigned int const OldRuns.size() = Rows[CurrentRow].RunCount;
	}*/
}

void RunData::ShiftVertically(int Rows)
{
	/*unsigned int const Split = Rows % Rows.size();
	Reverse(0, Split);
	Reverse(Split, Rows.size());
	Reverse(0, Rows.size());*/
}
		
bool RunData::IsBlack(unsigned int const &Index) { return Index & 1; }
		
void RunData::FlipSubsectionVertically(unsigned int const &Start, unsigned int const &End)
{
	assert(Start <= Rows.size());
	assert(End <= Rows.size());
	assert(Start <= End);

	unsigned int Half = (Start + End) / 2;
	for (unsigned int CurrentRow = Start; CurrentRow < Half; ++CurrentRow)
	{ 
		// Can I use std::move instead?  Extra allocations?
		RunArray Temp;
		Temp.swap(Rows[CurrentRow]);
		Rows[CurrentRow].swap(Rows[End - CurrentRow - 1]);
		Rows[End - CurrentRow - 1].swap(Temp);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// Undo levels

Mark::Mark(RunData &Base) : Base(Base)
{
	Rows.resize(Base.Rows.size());
}

Change *Mark::Apply(bool &FlippedHorizontally, bool &FlippedVertically)
{
	assert(Rows.size() == Base.Rows.size());
	FlippedHorizontally = false;
	FlippedVertically = false;
	Mark *Out = new Mark(Base);
	
	for (unsigned int CurrentRow = 0; CurrentRow < Rows.size(); ++CurrentRow)
		if (Rows[CurrentRow].size() > 0)
		{
			Out->AddLine(CurrentRow);
			std::swap(Base.Rows[CurrentRow], Rows[CurrentRow]);
		}

	return Out;
}

Change::CombineResult Mark::Combine(Change *Other) { return Change::CombineResult::Fail; }

void Mark::AddLine(unsigned int const &LineNumber)
{
	assert(LineNumber < Base.Rows.size());
	assert(LineNumber < Rows.size());

	// Only add lines if they haven't already been added at this undo level (keep the state at the beginning of the undo)
	if (Rows[LineNumber].size() > 0) return;

	Rows[LineNumber] = Base.Rows[LineNumber];
	assert(Rows[LineNumber].size() > 0);
}

HorizontalFlip::HorizontalFlip(RunData &Base) : Base(Base) { }

Change *HorizontalFlip::Apply(bool &FlippedHorizontally, bool &FlippedVertically)
{
	FlippedHorizontally = true;
	FlippedVertically = false;
	Base.FlipHorizontally();
	return new HorizontalFlip(Base);
}

Change::CombineResult HorizontalFlip::Combine(Change *Other) 
{ 
	if (typeid(*this) == typeid(*Other))
		return Change::CombineResult::Nullify;
	return Change::CombineResult::Fail;
}

VerticalFlip::VerticalFlip(RunData &Base) : Base(Base) { }

Change *VerticalFlip::Apply(bool &FlippedHorizontally, bool &FlippedVertically)
{
	FlippedHorizontally = true;
	FlippedVertically = false;
	Base.FlipVertically();
	return new VerticalFlip(Base);
}

Change::CombineResult VerticalFlip::Combine(Change *Other) 
{ 
	if (typeid(*this) == typeid(*Other))
		return Change::CombineResult::Nullify;
	return Change::CombineResult::Fail;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Image manipulation/management
Image::Image(SettingsData &Settings) :
	Settings(Settings),
	ImageSpace(FlatVector(), Settings.ImageSize), PixelsBelow(Settings.DisplayScale),
	DisplaySpace(FlatVector(), ImageSpace.Size / (float)PixelsBelow),
	Data(new RunData(ImageSpace.Size)), CurrentMarkUndo(nullptr), ModifiedSinceSave(false)
	{}

Image::Image(SettingsData &Settings, String const &Filename) :
	Settings(Settings),
	ImageSpace(FlatVector(), Settings.ImageSize), PixelsBelow(Settings.DisplayScale),
	DisplaySpace(FlatVector(), ImageSpace.Size / (float)PixelsBelow),
	Data(nullptr), CurrentMarkUndo(nullptr), ModifiedSinceSave(false)
{
	/// Open the file
	FILE *Input = fopen(Filename.c_str(), "rb");
	if (Input == nullptr)
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
	BZFILE *CompressInput = BZ2_bzReadOpen(&Error, Input, 0, 0, nullptr, 0);
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
	uint32_t RowCount, Width;
	BZ2_bzRead(&Error, CompressInput, &RowCount, sizeof(RowCount));
	BZ2_bzRead(&Error, CompressInput, &Width, sizeof(Width));

	ImageSpace.Size[0] = Width;
	ImageSpace.Size[1] = RowCount;
	Data = new RunData(ImageSpace.Size);

	Settings.ImageSize = ImageSpace.Size;

	DisplaySpace.Size = ImageSpace.Size / (float)PixelsBelow;

	for (uint32_t CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
	{
		uint32_t RunCount;
		BZ2_bzRead(&Error, CompressInput, &RunCount, sizeof(RunCount));
		Data->Rows[CurrentRow].resize(RunCount);
		BZ2_bzRead(&Error, CompressInput, &Data->Rows[CurrentRow][0], sizeof(RunData::Run) * RunCount);
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
	if (Output == nullptr)
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
		BZ2_bzWriteClose(&Error, CompressOutput, 0, nullptr, nullptr);
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
	uint32_t RowCountBuffer = Data->Rows.size();
	BZ2_bzWrite(&Error, CompressOutput, &RowCountBuffer, sizeof(RowCountBuffer));
	uint32_t WidthBuffer = Data->Width;
	BZ2_bzWrite(&Error, CompressOutput, &WidthBuffer, sizeof(WidthBuffer));
	for (uint32_t CurrentRow = 0; CurrentRow < Data->Rows.size(); CurrentRow++)
	{
		uint32_t RunCountBuffer = Data->Rows[CurrentRow].size();
		BZ2_bzWrite(&Error, CompressOutput, &RunCountBuffer, sizeof(RunCountBuffer));

		uint32_t RunBufferSize = sizeof(RunData::Run) * Data->Rows[CurrentRow].size();
		BZ2_bzWrite(&Error, CompressOutput, &Data->Rows[CurrentRow][0], RunBufferSize);
	}

	/// Close everything
	BZ2_bzWriteClose(&Error, CompressOutput, 0, nullptr, nullptr);
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
	if (CurrentMarkUndo == nullptr)
		CurrentMarkUndo = new ::Mark(*Data);
	auto const Line = [&](int Left, int Right, int Row, bool Black)
	{
		CurrentMarkUndo->AddLine(Row);
		Data->Line(Left, Right, Row, Black);
	};

	for (int CurrentRow = CapBelowStart; CurrentRow < LineBottom; CurrentRow++)
	{
		int Left = CapHorizontalMax,
			Right = CapHorizontalMin;

		FromCap.ExpandMarkBounds(Left, Right, CurrentRow);
		ToCap.ExpandMarkBounds(Left, Right, CurrentRow);

		Line(Left, Right, CurrentRow, Black);
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
		Line(Left, Right, CurrentRow, Black);
	}

	for (int CurrentRow = LineTop; CurrentRow < CapAboveEnd; CurrentRow++)
	{
		int Left = CapHorizontalMax,
			Right = CapHorizontalMin;

		FromCap.ExpandMarkBounds(Left, Right, CurrentRow);
		ToCap.ExpandMarkBounds(Left, Right, CurrentRow);

		Line(Left, Right, CurrentRow, Black);
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

void Image::FinishMark(void)
{
	if (CurrentMarkUndo != nullptr)
	{
		Changes.AddUndo(CurrentMarkUndo);
		CurrentMarkUndo = nullptr;
	}
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
{
	FinishMark();
	Data->FlipHorizontally(); 
	ModifiedSinceSave = true; 
}

void Image::FlipVertically(void)
{ 
	FinishMark();
	Data->FlipVertically(); 
	ModifiedSinceSave = true; 
}

bool Image::HasChanges(void)
	{ return Changes.CanUndo() && ModifiedSinceSave; }

void Image::Undo(bool &FlippedHorizontally, bool &FlippedVertically)
	{ if (Changes.CanUndo()) Changes.Undo(FlippedHorizontally, FlippedVertically); }

void Image::Redo(bool &FlippedHorizontally, bool &FlippedVertically)
	{ if (Changes.CanRedo()) Changes.Redo(FlippedHorizontally, FlippedVertically); }

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
