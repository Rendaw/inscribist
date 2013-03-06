// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#include "image.h"

#include <cassert>
#include <stdint.h>
#include <cmath>
#include <iomanip>
#include <bzlib.h>
#include <cstring>

#include "ren-general/endian.h"
#include "ren-translation/translation.h"

unsigned int const MaxUndoLevels = 50;

Change::~Change(void) {}
		
void ChangeManager::AddUndo(Change *Undo)
{
	while (!Redos.empty())
		Redos.pop_back();
	Change::CombineResult CombineResult;
	if (!CanUndo() || ((CombineResult = Undos.back()->Combine(Undo)) == Change::CombineResult::Fail))
	{
		Undos.push_back(Undo);
		if (Undos.size() > MaxUndoLevels)
			Undos.pop_front();
		return;
	}
	switch (CombineResult)
	{
		case Change::CombineResult::Nullify:
			Undos.pop_back();
			delete Undo;
			break;
		case Change::CombineResult::Combine:
			delete Undo;
			break;
		default: assert(false); break;
	}
}

bool ChangeManager::CanUndo(void) { return !Undos.empty(); }

void ChangeManager::Undo(bool &FlippedHorizontally, bool &FlippedVertically)
{
	assert(CanUndo());
	Redos.push_back(Undos.back()->Apply(FlippedHorizontally, FlippedVertically));
	Undos.pop_back();
}

bool ChangeManager::CanRedo(void) { return !Redos.empty(); }

void ChangeManager::Redo(bool &FlippedHorizontally, bool &FlippedVertically)
{
	assert(CanRedo());
	Undos.push_back(Redos.back()->Apply(FlippedHorizontally, FlippedVertically));
	Redos.pop_back();
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
	assert(Width > 0);
	return Width;
}

RunData::RunData(std::vector<std::vector<Run> > const &InitialRows) : 
	Rows(InitialRows), Width(CalculateWidth(Rows))
	{ }

void RunData::Line(int UnclippedLeft, int UnclippedRight, unsigned int const &Y, bool Black)
{
	// Validate parameters
	assert(Y < Rows.size());

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
	// Split is where the end will be after the shift
	unsigned int const Split = Mod(-Columns, Width);
	assert(Split <= Width);
	
	// Shift each row to align with the new split
	for (unsigned int CurrentRow = 0; CurrentRow < Rows.size(); ++CurrentRow)
	{
		RunArray OldRuns; 
		assert(!Rows[CurrentRow].empty());
		Rows[CurrentRow].swap(OldRuns); // Could we just move constructor and clear instead?

		class RunIterator
		{
			public:
				RunIterator(RunArray const &Runs) : Runs(Runs), CurrentRun(0) 
					{ assert(!Runs.empty()); RunRight = Runs[CurrentRun]; }

				void Reset(void) { CurrentRun = 0; RunRight = Runs[CurrentRun]; }

				bool CanAdvance(void) { return CurrentRun + 1 < Runs.size(); }

				void Advance(void)
				{
					++CurrentRun;
					assert(CurrentRun < Runs.size());
					RunRight += Runs[CurrentRun];
				}

				unsigned int Right(void) { return RunRight; }

				unsigned int Index(void) { return CurrentRun; }

				unsigned int Width(void) { return Runs[CurrentRun]; }

				bool IsBlack(void) { return RunData::IsBlack(CurrentRun); }

			private:
				RunArray const &Runs;
				unsigned int CurrentRun, RunRight;

		} OldRun(OldRuns);

		// First, find the row that straddles/hits the division 
		while (OldRun.Right() <= Split)
			OldRun.Advance();

		unsigned int const StraddleRunIndex = OldRun.Index();
		
		Rows[CurrentRow].reserve(OldRuns.size() + 2); // Potential white padding + extra split run

		// Write the runs to the end.
		auto AddPostSplitRun = [&](bool Black, unsigned int Length)
		{
			assert(Length > 0);
			if (Rows[CurrentRow].empty() && Black)
				Rows[CurrentRow].push_back(0);
			Rows[CurrentRow].push_back(Length);
		};

		unsigned int const StraddleRunRemainder = OldRun.Right() - Split;
		AddPostSplitRun(OldRun.IsBlack(), StraddleRunRemainder);

		while (OldRun.CanAdvance())
		{
			OldRun.Advance();
			AddPostSplitRun(OldRun.IsBlack(), OldRun.Width());
		}

		// Start from the beginning, and work back to the split.  Drop the initial padded white if present.
		OldRun.Reset();

		if (OldRun.Width() == 0)
			OldRun.Advance();
		
		auto AddPreSplitRun = [&](bool Black, unsigned int Length)
		{
			assert(Length > 0);
			assert(!Rows[CurrentRow].empty());
			if (IsBlack(Rows[CurrentRow].size() - 1) == Black)
				Rows[CurrentRow].back() += Length;
			else Rows[CurrentRow].push_back(Length);
		};

		while (OldRun.Index() < StraddleRunIndex)
		{
			AddPreSplitRun(OldRun.IsBlack(), OldRun.Width());
			OldRun.Advance();
		}

		// If the run is split, write the opening portion as a final run
		if (StraddleRunRemainder != OldRun.Width())
			AddPreSplitRun(OldRun.IsBlack(), OldRun.Width() - StraddleRunRemainder);

#ifndef NDEBUG
		/*unsigned int TestWidth = 0; 
		for (auto const &Run : Rows[CurrentRow]) TestWidth += Run; 
		assert(TestWidth == Width);*/
#endif
	}
}

void RunData::ShiftVertically(int Rows)
{
	unsigned int const Split = Mod(-Rows, this->Rows.size());
	FlipSubsectionVertically(0, Split);
	FlipSubsectionVertically(Split, this->Rows.size());
	FlipSubsectionVertically(0, this->Rows.size());
}
		
void RunData::Add(unsigned int const Left, unsigned int const Right, unsigned int const Up, unsigned int const Down)
{
	Rows.resize(Rows.size() + Up + Down);
	Width += Left + Right;
	for (unsigned int NewRowReverseIndex = 0; NewRowReverseIndex < Rows.size(); NewRowReverseIndex++)
	{
		unsigned int const NewRowIndex = Rows.size() - 1 - NewRowReverseIndex;
		RunArray &NewRow = Rows[NewRowIndex];
		if ((NewRowIndex >= Up) && (NewRowIndex < Rows.size() - Down))
		{
			if (Up > 0)
			{
				assert(NewRow.empty());
				NewRow.swap(Rows[NewRowIndex - Up]);
			}
			NewRow[0] += Left;
			unsigned int const RightColumn = NewRow.size() - 1;
			if ((Right > 0) && IsBlack(RightColumn))
				NewRow.push_back(Right);
			else NewRow[RightColumn] += Right;
		}
		else
		{
			assert(NewRow.empty());
			NewRow.resize(1);
			NewRow[0] = Width;
		}
	}
#ifndef NDEBUG
	for (unsigned int Top = 0; Top < Up; ++Top)
	{
		assert(Rows[Top].size() == 1);
		assert(Rows[Top][0] == Width);
	}
	for (unsigned int Bottom = 0; Bottom < Down; ++Bottom)
	{
		assert(Rows[Rows.size() - 1 - Bottom].size() == 1);
		assert(Rows[Rows.size() - 1 - Bottom][0] == Width);
	}
#endif
}

void RunData::Remove(unsigned int const Left, unsigned int const Right, unsigned int const Up, unsigned int const Down)
{
#ifndef NDEBUG
	for (unsigned int Top = 0; Top < Up; ++Top)
	{
		assert(Rows[Top].size() == 1);
		assert(Rows[Top][0] == Width);
	}
	for (unsigned int Bottom = 0; Bottom < Down; ++Bottom)
	{
		assert(Rows[Rows.size() - 1 - Bottom].size() == 1);
		assert(Rows[Rows.size() - 1 - Bottom][0] == Width);
	}
#endif
	Width -= Left + Right;
	unsigned int const RemainingRows = Rows.size() - Up - Down;
	for (unsigned int RemainingRow = 0; RemainingRow < RemainingRows; ++RemainingRow)
	{
		if (Up > 0)
			Rows[RemainingRow].swap(Rows[RemainingRow + Up]);
		assert(Rows[RemainingRow][0] >= Left);
		Rows[RemainingRow][0] -= Left;
		if (Right > 0)
		{
			unsigned int RightColumn = Rows[RemainingRow].size() - 1;
			assert(!IsBlack(RightColumn));
			assert(Rows[RemainingRow][RightColumn] >= Right);
			if (Rows[RemainingRow][RightColumn] == Right)
				Rows[RemainingRow].pop_back();
			else Rows[RemainingRow][Rows[RemainingRow].size() - 1] -= Right;
		}
	}
	Rows.resize(RemainingRows);
}

void RunData::Enlarge(unsigned int const Factor)
{
	assert(Factor >= 1);
	unsigned int const OriginalHeight = Rows.size();
	Rows.resize(OriginalHeight * Factor);
	Width *= Factor;
	for (unsigned int NewRowReverseIndex = 0; NewRowReverseIndex < OriginalHeight; ++NewRowReverseIndex)
	{
		unsigned int const NewRowIndex = Rows.size() - 1 - NewRowReverseIndex * Factor;
		unsigned int const OldRowIndex = OriginalHeight - 1 - NewRowReverseIndex;
		Rows[NewRowIndex].swap(Rows[OldRowIndex]);
		assert(Rows[OldRowIndex].size() == 0);
		for (auto &Run : Rows[NewRowIndex])
			Run *= Factor;
		for (unsigned int FactorStep = 1; FactorStep < Factor; ++FactorStep)
			Rows[NewRowIndex - FactorStep] = Rows[NewRowIndex];
	}
}
		
void RunData::Shrink(unsigned int const Factor)
{
	assert(Width % Factor == 0);
	assert(Rows.size() % Factor == 0);
	unsigned int const NewHeight = Rows.size() / Factor;
	Width /= Factor;
	for (unsigned int RowIndex = 0; RowIndex < NewHeight; ++RowIndex)
	{
		auto &Row = Rows[RowIndex];
		if (RowIndex > 1)
			Row.swap(Rows[RowIndex * Factor]);
		for (auto &Run : Row)
			Run /= Factor;
	}
	Rows.resize(NewHeight);
}
		
bool RunData::IsBlack(unsigned int const &Index) { return Index & 1; }
		
void RunData::FlipSubsectionVertically(unsigned int const &Start, unsigned int const &End)
{
	assert(Start <= Rows.size());
	assert(End <= Rows.size());
	assert(Start <= End);

	unsigned int const Half = (End - Start) / 2;
	for (unsigned int CurrentRow = 0; CurrentRow < Half; ++CurrentRow)
		Rows[Start + CurrentRow].swap(Rows[End - 1 - CurrentRow]);
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

Change::CombineResult Mark::Combine(Change *) { return Change::CombineResult::Fail; }

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

Shift::Shift(RunData &Base, int Right, int Down) : Base(Base), Right(Right), Down(Down) { }

Change *Shift::Apply(bool &, bool &)
{
	if (Right != 0) Base.ShiftHorizontally(Right);
	if (Down != 0) Base.ShiftVertically(Down);
	return new Shift(Base, -Right, -Down);
}

Change::CombineResult Shift::Combine(Change *Other)
{
	if (typeid(*this) == typeid(*Other))
	{
		Right += static_cast<Shift *>(Other)->Right;
		Down += static_cast<Shift *>(Other)->Down;
		return Change::CombineResult::Combine;
	}
	return Change::CombineResult::Fail;
}

Add::Add(RunData &Base, unsigned int Left, unsigned int Right, unsigned int Up, unsigned int Down) : Base(Base), 
	Left(Left), Right(Right), Up(Up), Down(Down)
	{ }

Change *Add::Apply(bool &, bool &)
{
	Base.Add(Left, Right, Up, Down);
	return new Remove(Base, Left, Right, Up, Down);
}

Change::CombineResult Add::Combine(Change *Other)
{
	if (typeid(*this) == typeid(*Other))
	{
		Left += static_cast<Add *>(Other)->Left;
		Right += static_cast<Add *>(Other)->Right;
		Up += static_cast<Add *>(Other)->Up;
		Down += static_cast<Add *>(Other)->Down;
		return Change::CombineResult::Combine;
	}
	return Change::CombineResult::Fail;
}

Remove::Remove(RunData &Base, unsigned int Left, unsigned int Right, unsigned int Up, unsigned int Down) : Base(Base), 
	Left(Left), Right(Right), Up(Up), Down(Down)
	{ }

Change *Remove::Apply(bool &, bool &)
{
	Base.Remove(Left, Right, Up, Down);
	return new Remove(Base, Left, Right, Up, Down);
}

Change::CombineResult Remove::Combine(Change *Other)
{
	if (typeid(*this) == typeid(*Other))
	{
		Left += static_cast<Remove *>(Other)->Left;
		Right += static_cast<Remove *>(Other)->Right;
		Up += static_cast<Remove *>(Other)->Up;
		Down += static_cast<Remove *>(Other)->Down;
		return Change::CombineResult::Combine;
	}
	return Change::CombineResult::Fail;
}

Enlarge::Enlarge(RunData &Base, unsigned int const &Factor) : Base(Base), Factor(Factor)
	{}

Change *Enlarge::Apply(bool &, bool &)
{
	Base.Enlarge(Factor);
	return new Shrink(Base, Factor);
}

Change::CombineResult Enlarge::Combine(Change *Other)
{
	if (typeid(*this) == typeid(*Other))
	{
		Factor *= static_cast<Enlarge *>(Other)->Factor;
		return Change::CombineResult::Combine;
	}
	return Change::CombineResult::Fail;
}

Shrink::Shrink(RunData &Base, unsigned int const &Factor) : Base(Base), Factor(Factor)
	{}

Change *Shrink::Apply(bool &, bool &)
{
	Base.Shrink(Factor);
	return new Shrink(Base, Factor);
}

Change::CombineResult Shrink::Combine(Change *Other)
{
	if (typeid(*this) == typeid(*Other))
	{
		Factor *= static_cast<Shrink *>(Other)->Factor;
		return Change::CombineResult::Combine;
	}
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
	
	char Identifier3[32];
	memset(Identifier3, 0, 32);
	strncpy(Identifier3, "inscribble v02\n", 32);

	char LoadedIdentifier[32];
	size_t Result = fread(LoadedIdentifier, sizeof(char), 32, Input);
	if (Result < 32)
	{
		StandardErrorStream << Local("There was some error while trying to read the header of the file.  Inscribist files begin with 32 bytes of text.") << "\n" << OutputStream::Flush();
	}

	unsigned int Version = 2;
	if (strncmp(LoadedIdentifier, Identifier1, 32) == 0)
	{
		Version = 0;
	}
	else if (strncmp(LoadedIdentifier, Identifier2, 32) == 0)
	{
		Version = 1;
	}
	else if (strncmp(LoadedIdentifier, Identifier3, 32) != 0)
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

	if (Version >= 2)
	{
		LittleEndian<Color> DisplayPaper, DisplayInk, ExportPaper, ExportInk;
		BZ2_bzRead(&Error, CompressInput, &DisplayPaper, sizeof(DisplayPaper));
		BZ2_bzRead(&Error, CompressInput, &DisplayInk, sizeof(DisplayInk));
		BZ2_bzRead(&Error, CompressInput, &ExportPaper, sizeof(ExportPaper));
		BZ2_bzRead(&Error, CompressInput, &ExportInk, sizeof(ExportInk));
		Settings.DisplayPaper = DisplayPaper;
		Settings.DisplayInk = DisplayInk;
		Settings.ExportPaper = ExportPaper;
		Settings.ExportInk = ExportInk;

		/// Read in the image data
		LittleEndian<uint32_t> StandardizedRowCount, StandardizedWidth;
		BZ2_bzRead(&Error, CompressInput, &StandardizedRowCount, sizeof(StandardizedRowCount));
		BZ2_bzRead(&Error, CompressInput, &StandardizedWidth, sizeof(StandardizedWidth));
		uint32_t RowCount = StandardizedRowCount;

		ImageSpace.Size[0] = StandardizedWidth;
		ImageSpace.Size[1] = StandardizedRowCount;
		Data = new RunData(ImageSpace.Size);

		Settings.ImageSize = ImageSpace.Size;

		DisplaySpace.Size = ImageSpace.Size / (float)PixelsBelow;

		for (uint32_t CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
		{
			LittleEndian<uint32_t> StandardizedRunCount;
			BZ2_bzRead(&Error, CompressInput, &StandardizedRunCount, sizeof(StandardizedRunCount));
			uint32_t RunCount = StandardizedRunCount;

			std::vector<LittleEndian<RunData::Run> > Runs;
			Runs.resize(RunCount);
			BZ2_bzRead(&Error, CompressInput, &Runs[0], sizeof(LittleEndian<RunData::Run>) * RunCount);
			Data->Rows[CurrentRow].clear();
			Data->Rows[CurrentRow].reserve(RunCount);
			for (auto const &Run : Runs) Data->Rows[CurrentRow].push_back(Run);
		}
	}
	else
	{
		if (Version >= 1)
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

		unsigned int TotalLengths = 0;
		for (unsigned int CurrentRow = 0; CurrentRow < RowCount; CurrentRow++)
		{
			unsigned int RunCount;
			BZ2_bzRead(&Error, CompressInput, &RunCount, sizeof(RunCount));

			std::vector<unsigned int> Runs;
			Runs.resize(RunCount * 2);
			BZ2_bzRead(&Error, CompressInput, &Runs[0], sizeof(unsigned int) * RunCount * 2);
			
			Data->Rows[CurrentRow].clear();
			Data->Rows[CurrentRow].reserve(RunCount * 2);
			bool First = true;
			unsigned int LineWidth = 0;
			for (auto const &Run : Runs) 
			{
				if (First) First = false;
				else if (Run == 0) continue;
				Data->Rows[CurrentRow].push_back(Run);
				LineWidth += Run;
				TotalLengths += Run;
			}
			assert(LineWidth == Width);
		}
		assert(TotalLengths == Width * RowCount);
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
	strncpy(Identifier, "inscribble v02\n", 32);

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
	LittleEndian<Color> DisplayPaper = Settings.DisplayPaper, DisplayInk = Settings.DisplayInk,
		ExportPaper = Settings.ExportPaper, ExportInk = Settings.ExportInk;

	BZ2_bzWrite(&Error, CompressOutput, &DisplayPaper, sizeof(DisplayPaper));
	BZ2_bzWrite(&Error, CompressOutput, &DisplayInk, sizeof(DisplayInk));
	BZ2_bzWrite(&Error, CompressOutput, &ExportPaper, sizeof(ExportPaper));
	BZ2_bzWrite(&Error, CompressOutput, &ExportInk, sizeof(ExportInk));

	/// Write the image data
	LittleEndian<uint32_t> RowCountBuffer = Data->Rows.size();
	BZ2_bzWrite(&Error, CompressOutput, &RowCountBuffer, sizeof(RowCountBuffer));
	LittleEndian<uint32_t> WidthBuffer = Data->Width;
	BZ2_bzWrite(&Error, CompressOutput, &WidthBuffer, sizeof(WidthBuffer));
	
	std::vector<LittleEndian<uint32_t> > Runs;
	for (uint32_t CurrentRow = 0; CurrentRow < Data->Rows.size(); CurrentRow++)
	{
		uint32_t NativeRunCount = Data->Rows[CurrentRow].size();
		LittleEndian<uint32_t> RunCount = NativeRunCount;
		BZ2_bzWrite(&Error, CompressOutput, &RunCount, sizeof(RunCount));

		Runs.clear();
		Runs.reserve(Data->Rows[CurrentRow].size());
		for (auto const &Run : Data->Rows[CurrentRow]) Runs.push_back(Run);
		BZ2_bzWrite(&Error, CompressOutput, &Runs[0], sizeof(LittleEndian<uint32_t>) * Runs.size());
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

	// Fill in the lower cap region
	for (int CurrentRow = CapBelowStart; CurrentRow < LineBottom; CurrentRow++)
	{
		if (CurrentRow < 0) continue;
		if ((unsigned int)CurrentRow >= Data->Rows.size()) break;
		int Left = CapHorizontalMax,
			Right = CapHorizontalMin;

		FromCap.ExpandMarkBounds(Left, Right, CurrentRow);
		ToCap.ExpandMarkBounds(Left, Right, CurrentRow);

		Line(Left, Right, CurrentRow, Black);
	}

	// Fill in the area within the line
	float Line1Position = Line1.StartHorizontalPosition,
		Line2Position = Line2.StartHorizontalPosition;
	for (int CurrentRow = LineBottom; CurrentRow < LineTop; CurrentRow++)
	{
		if (CurrentRow < 0) continue;
		if ((unsigned int)CurrentRow >= Data->Rows.size()) break;

		int Left = LineHorizontalMin,
			Right = LineHorizontalMax;

		Line1.ConstrainMarkBounds(Left, Right, Line1Position, CurrentRow);
		Line2.ConstrainMarkBounds(Left, Right, Line2Position, CurrentRow);
		FromCap.ExpandMarkBounds(Left, Right, CurrentRow);
		ToCap.ExpandMarkBounds(Left, Right, CurrentRow);

		assert(Left <= Right);
		Line(Left, Right, CurrentRow, Black);
	}

	// Fill in the upper cap region
	for (int CurrentRow = LineTop; CurrentRow < CapAboveEnd; CurrentRow++)
	{
		if (CurrentRow < 0) continue;
		if ((unsigned int)CurrentRow >= Data->Rows.size()) break;

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
		
FlatVector &Image::GetSize(void)
	{ return ImageSpace.Size; }

FlatVector &Image::GetDisplaySize(void)
	{ return DisplaySpace.Size; }

void Image::FlipHorizontally(void)
{
	FinishMark();
	::HorizontalFlip FlipChange(*Data);
	bool Unused1, Unused2;
	Changes.AddUndo(FlipChange.Apply(Unused1, Unused2));
	ModifiedSinceSave = true; 
}

void Image::FlipVertically(void)
{ 
	FinishMark();
	::VerticalFlip FlipChange(*Data);
	bool Unused1, Unused2;
	Changes.AddUndo(FlipChange.Apply(Unused1, Unused2));
	ModifiedSinceSave = true; 
}
		
void Image::Shift(bool Large, int Right, int Down)
{
	FinishMark();
	assert((Right != 0) || (Down != 0));
	::Shift ShiftChange(*Data, 
		Right * PixelsBelow * (Large ? 50 : 1),
		Down * PixelsBelow * (Large ? 50 : 1));
	bool Unused1, Unused2;
	Changes.AddUndo(ShiftChange.Apply(Unused1, Unused2));
	ModifiedSinceSave = true;
}
		
void Image::Scale(unsigned int const &Factor)
{
	FinishMark();
	assert(Factor >= 1);
	::Enlarge ScaleChange(*Data, Factor);
	bool Unused1, Unused2;
	Changes.AddUndo(ScaleChange.Apply(Unused1, Unused2));
	ModifiedSinceSave = true;
}

void Image::Add(unsigned int const &Left, unsigned int const &Right, unsigned int const &Up, unsigned int const &Down)
{
	FinishMark();
	::Add AddChange(*Data, Left, Right, Up, Down);
	bool Unused1, Unused2;
	Changes.AddUndo(AddChange.Apply(Unused1, Unused2));
	ModifiedSinceSave = true;
}

bool Image::HasChanges(void)
	{ return ModifiedSinceSave; }

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
