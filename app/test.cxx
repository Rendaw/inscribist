// Copyright 2013 Rendaw, under the FreeBSD license (See included license.txt)

#include "image.h"

void CompareInternal(int Line, RunData const &GotData, RunData const &ExpectedData)
{
	RunData::RowArray const &Got = GotData.Rows;
	RunData::RowArray const &Expected = ExpectedData.Rows;
	bool FailedRunCount = false;
	bool FailedRuns = false;

	if (Got.size() != Expected.size())
	{
		std::cout << "Line " << Line << ": Failed to match row counts (" << Got.size() << ", expected " << Expected.size() << ")." << std::endl;
		assert(false);
	}

	for (unsigned int RowIndex = 0; RowIndex < Got.size(); ++RowIndex)
	{
		if (Got[RowIndex].size() != Expected[RowIndex].size())
		{
			std::cout << "Line " << Line << ": Failed to match run counts (row " << RowIndex << ")." << std::endl;
			FailedRunCount = true;
			break;
		}
	}

	if (!FailedRunCount)
	{
		for (unsigned int RowIndex = 0; RowIndex < Got.size(); ++RowIndex)
		{
			for (unsigned int RunIndex = 0; RunIndex < Got[RowIndex].size(); ++RunIndex)
			{
				if (Got[RowIndex][RunIndex] != Expected[RowIndex][RunIndex])
				{
					std::cout << "Line " << Line << ": Failed to match run values (row " << RowIndex << ", run " << RunIndex << ")." << std::endl;
					FailedRuns = true;
				}
			}
		}
	}

	if (!FailedRunCount && !FailedRuns) 
	{
		std::cout << "Line " << Line << ": SUCCESS" << std::endl;
		return;
	}

	for (unsigned int RowIndex = 0; RowIndex < Got.size(); ++RowIndex)
	{
		{
			std::cout << "\tRow " << RowIndex << " got      " << Got[RowIndex].size() << " runs: ";
			unsigned int Width = 0;
			for (auto const &Run : Got[RowIndex])
			{
				Width += Run;
				std::cout << Run << " ";
			}
			std::cout << ": Width " << Width << std::endl;
		}
		{
			std::cout << "\tRow " << RowIndex << " expected " << Expected[RowIndex].size() << " runs: ";
			unsigned int Width = 0;
			for (auto const &Run : Expected[RowIndex])
			{
				Width += Run;
				std::cout << Run << " ";
			}
			std::cout << ": Width " << Width << std::endl;
		}
	}
	assert(false);
}

void CompareInternal(int Line, std::vector<unsigned int> const &Got, std::vector<unsigned int> const &Expected)
{
	assert(Got.size() == Expected.size()); // Explicitly set in tests
	bool FailedPixels = false;
	for (unsigned int PixelIndex = 0; PixelIndex < Got.size(); ++PixelIndex)
	{
		if (Got[PixelIndex] != Expected[PixelIndex])
		{
			std::cout << "Line " << Line << ": Failed to match count values (pixel " << PixelIndex << ")." << std::endl;
			FailedPixels = true;
		}
	}
	
	if (!FailedPixels) 
	{
		std::cout << "Line " << Line << ": SUCCESS" << std::endl;
		return;
	}

	{
		std::cout << "\tGot pixels      " << Got.size() << ": ";
		unsigned int Total = 0;
		for (auto const &Count : Got)
		{
			Total += Count;
			std::cout << Count << " ";
		}
		std::cout << ": Total " << Total << std::endl;
	}
	{
		std::cout << "\tExpected pixels " << Expected.size() << ": ";
		unsigned int Total = 0;
		for (auto const &Count : Expected)
		{
			Total += Count;
			std::cout << Count << " ";
		}
		std::cout << ": Total " << Total << std::endl;
	}
	assert(false);
}

#define Compare(...) CompareInternal(__LINE__, __VA_ARGS__)

int main(int, char **)
{
	// Test adding lines to rundata
	{
		RunData Test {{{ {{{ 100 }}} }}};
		RunData const Expected {{{ {{{ 100 }}} }}};
		Test.Line(0, 0, 1, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 100 }} }};
		Test.Line(100, 200, 0, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 100 }} }};
		Test.Line(-100, 0, 0, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 100 }} }};
		Test.Line(100, 0, 0, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 100 }} }};
		Test.Line(0, -100, 0, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 0, 100 }} }};
		Test.Line(0, 100, 0, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 50, 50 }} }};
		Test.Line(50, 100, 0, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 0, 50, 50 }} }};
		Test.Line(0, 50, 0, true);
		Compare(Test, Expected);
	}
	
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 25, 50, 25 }} }};
		Test.Line(25, 75, 0, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 100 }} }};
		Test.Line(25, 75, 0, true);
		Test.Line(25, 75, 0, false);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 50, 25, 25 }} }};
		Test.Line(25, 75, 0, true);
		Test.Line(25, 50, 0, false);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 25, 25, 50 }} }};
		Test.Line(25, 75, 0, true);
		Test.Line(50, 75, 0, false);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 25, 50, 25 }} }};
		Test.Line(25, 75, 0, true);
		Test.Line(25, 50, 0, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 25, 50, 25 }} }};
		Test.Line(25, 75, 0, true);
		Test.Line(50, 75, 0, true);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 25, 1, 74 }} }};
		Test.Line(25, 75, 0, true);
		Test.Line(26, 76, 0, false);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{ 100 }} }};
		RunData const Expected { RunData::RowArray { {{ 25, 50, 25 }} }};
		Test.Line(25, 50, 0, true);
		Test.Line(50, 75, 0, true);
		Compare(Test, Expected);
	}
	
	// Test rendering lines of rundata
	{
		RunData Test { RunData::RowArray { {{4}}, {{4}} }};
		std::vector<unsigned int> Buffer = {0, 0};
		Test.Combine(&Buffer[0], 2, 0, 0, 2);
		std::vector<unsigned int> const Expected = {0, 0};
		Compare(Buffer, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{4}}, {{2, 2}} }};
		std::vector<unsigned int> Buffer = {0, 0};
		Test.Combine(&Buffer[0], 2, 0, 0, 2);
		std::vector<unsigned int> const Expected = {0, 2};
		Compare(Buffer, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 4}}, {{0, 4}} }};
		std::vector<unsigned int> Buffer = {0, 0};
		Test.Combine(&Buffer[0], 2, 0, 0, 2);
		std::vector<unsigned int> const Expected = {4, 4};
		Compare(Buffer, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 4}}, {{0, 4}} }};
		std::vector<unsigned int> Buffer = {0, 0};
		Test.Combine(&Buffer[0], 2, 1, 0, 2);
		std::vector<unsigned int> const Expected = {4, 0};
		Compare(Buffer, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 4}} }};
		std::vector<unsigned int> Buffer = {0, 0};
		Test.Combine(&Buffer[0], 2, 0, 0, 2);
		std::vector<unsigned int> const Expected = {2, 2};
		Compare(Buffer, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 3, 1}} }};
		std::vector<unsigned int> Buffer = {0, 0};
		Test.Combine(&Buffer[0], 2, 0, 0, 2);
		std::vector<unsigned int> const Expected = {2, 1};
		Compare(Buffer, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{1, 3}} }};
		std::vector<unsigned int> Buffer = {0, 0};
		Test.Combine(&Buffer[0], 2, 0, 0, 2);
		std::vector<unsigned int> const Expected = {1, 2};
		Compare(Buffer, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{2}} }};
		std::vector<unsigned int> Buffer = {0};
		Test.Combine(&Buffer[0], 1, 1, 0, 2);
		std::vector<unsigned int> const Expected = {0};
		Compare(Buffer, Expected);
	}
	
	// Test flipping rundata
	{
		RunData Test { RunData::RowArray { {{1}}, {{0, 1}} } };
		RunData Expected { RunData::RowArray { {{0, 1}}, {{1}} } };
		Test.FlipVertically();
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{2}}, {{1, 1}}, {{0, 2}} } };
		RunData Expected { RunData::RowArray { {{0, 2}}, {{1, 1}}, {{2}} } };
		Test.FlipVertically();
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{2}}, {{1, 1}}, {{0, 1, 1}}, {{0, 2}} } };
		RunData Expected { RunData::RowArray { {{2}}, {{0, 1, 1}}, {{1, 1}}, {{0, 2}} } };
		Test.FlipHorizontally();
		Compare(Test, Expected);
	}

	// Test shifting rundata
	{
		RunData Test { RunData::RowArray { {{1}}, {{0, 1}} } };
		RunData Expected { RunData::RowArray { {{0, 1}}, {{1}} } };
		Test.ShiftVertically(1);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{1}}, {{0, 1}} } };
		RunData Expected { RunData::RowArray { {{1}}, {{0, 1}} } };
		Test.ShiftVertically(2);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{1}}, {{0, 1}} } };
		RunData Expected { RunData::RowArray { {{0, 1}}, {{1}} } };
		Test.ShiftVertically(-1);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{1}}, {{0, 1}}, {{1}} } };
		RunData Expected { RunData::RowArray { {{1}}, {{1}}, {{0, 1}} } };
		Test.ShiftVertically(1);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{1}}, {{0, 1}}, {{1}} } };
		RunData Expected { RunData::RowArray { {{1}}, {{1}}, {{0, 1}} } };
		Test.ShiftVertically(-2);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{10}} } };
		RunData Expected { RunData::RowArray { {{10}} } };
		Test.ShiftHorizontally(5);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{10}} } };
		RunData Expected { RunData::RowArray { {{10}} } };
		Test.ShiftHorizontally(10);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 10}} } };
		RunData Expected { RunData::RowArray { {{0, 10}} } };
		Test.ShiftHorizontally(5);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 10}} } };
		RunData Expected { RunData::RowArray { {{0, 10}} } };
		Test.ShiftHorizontally(10);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{10, 10}} } };
		RunData Expected { RunData::RowArray { {{0, 10, 10}} } };
		Test.ShiftHorizontally(10);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{10, 10}} } };
		RunData Expected { RunData::RowArray { {{0, 10, 10}} } };
		Test.ShiftHorizontally(-10);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{10, 10}} } };
		RunData Expected { RunData::RowArray { {{5, 10, 5}} } };
		Test.ShiftHorizontally(-5);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{10, 10}} } };
		RunData Expected { RunData::RowArray { {{0, 5, 10, 5}} } };
		Test.ShiftHorizontally(5);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{10, 10, 10}} } };
		RunData Expected { RunData::RowArray { {{20, 10}} } };
		Test.ShiftHorizontally(10);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{10, 10, 10}} } };
		RunData Expected { RunData::RowArray { {{0, 10, 20}} } };
		Test.ShiftHorizontally(20);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{0, 10, 20}} } };
		RunData Expected { RunData::RowArray { {{10, 10, 10}} } };
		Test.ShiftHorizontally(10);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{20, 10}} } };
		RunData Expected { RunData::RowArray { {{10, 10, 10}} } };
		Test.ShiftHorizontally(20);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{0, 5, 10, 5}} } };
		RunData Expected { RunData::RowArray { {{0, 10, 10}} } };
		Test.ShiftHorizontally(5);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 10, 10}} } };
		RunData Expected { RunData::RowArray { {{0, 5, 10, 5}} } };
		Test.ShiftHorizontally(15);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{0, 5, 10, 5}} } };
		RunData Expected { RunData::RowArray { {{10, 10}} } };
		Test.ShiftHorizontally(15);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{10, 10}} } };
		RunData Expected { RunData::RowArray { {{0, 5, 10, 5}} } };
		Test.ShiftHorizontally(5);
		Compare(Test, Expected);
	}

	// Add
	{
		RunData Test { RunData::RowArray { {{100}} } };
		RunData Expected { RunData::RowArray { {{110}} } };
		Test.Add(10, 0, 0, 0);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{100}} } };
		RunData Expected { RunData::RowArray { {{110}} } };
		Test.Add(0, 10, 0, 0);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{100}} } };
		RunData Expected { RunData::RowArray { {{100}}, {{100}} } };
		Test.Add(0, 0, 1, 0);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{100}} } };
		RunData Expected { RunData::RowArray { {{100}}, {{100}} } };
		Test.Add(0, 0, 0, 1);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{100}} } };
		RunData Expected { RunData::RowArray { {{120}}, {{120}}, {{120}} } };
		Test.Add(10, 10, 1, 1);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 100}} } };
		RunData Expected { RunData::RowArray { {{10, 100}} } };
		Test.Add(10, 0, 0, 0);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 100}} } };
		RunData Expected { RunData::RowArray { {{0, 100, 10}} } };
		Test.Add(0, 10, 0, 0);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 100}} } };
		RunData Expected { RunData::RowArray { {{100}}, {{0, 100}} } };
		Test.Add(0, 0, 1, 0);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 100}} } };
		RunData Expected { RunData::RowArray { {{0, 100}}, {{100}} } };
		Test.Add(0, 0, 0, 1);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{0, 100}} } };
		RunData Expected { RunData::RowArray { {{120}}, {{10, 100, 10}}, {{120}} } };
		Test.Add(10, 10, 1, 1);
		Compare(Test, Expected);
	}

	// Remove
	{
		RunData Test { RunData::RowArray { {{110}} } };
		RunData Expected { RunData::RowArray { {{100}} } };
		Test.Remove(10, 0, 0, 0);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{110}} } };
		RunData Expected { RunData::RowArray { {{100}} } };
		Test.Remove(0, 10, 0, 0);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{100}}, {{100}} } };
		RunData Expected { RunData::RowArray { {{100}} } };
		Test.Remove(0, 0, 1, 0);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{100}}, {{100}} } };
		RunData Expected { RunData::RowArray { {{100}} } };
		Test.Remove(0, 0, 0, 1);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{120}}, {{120}}, {{120}} } };
		RunData Expected { RunData::RowArray { {{100}} } };
		Test.Remove(10, 10, 1, 1);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{10, 100}} } };
		RunData Expected { RunData::RowArray { {{0, 100}} } };
		Test.Remove(10, 0, 0, 0);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 100, 10}} } };
		RunData Expected { RunData::RowArray { {{0, 100}} } };
		Test.Remove(0, 10, 0, 0);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{100}}, {{0, 100}} } };
		RunData Expected { RunData::RowArray { {{0, 100}} } };
		Test.Remove(0, 0, 1, 0);
		Compare(Test, Expected);
	}
	
	{
		RunData Test { RunData::RowArray { {{0, 100}}, {{100}} } };
		RunData Expected { RunData::RowArray { {{0, 100}} } };
		Test.Remove(0, 0, 0, 1);
		Compare(Test, Expected);
	}

	{
		RunData Test { RunData::RowArray { {{120}}, {{10, 100, 10}}, {{120}} } };
		RunData Expected { RunData::RowArray { {{0, 100}} } };
		Test.Remove(10, 10, 1, 1);
		Compare(Test, Expected);
	}

	// Enlarge
	{
		RunData Test { RunData::RowArray { {{5, 3}}, {{5, 3}}, {{3, 5}} } };
		RunData Expected { RunData::RowArray { {{15, 9}}, {{15, 9}}, {{15, 9}}, {{15, 9}}, {{15, 9}}, {{15, 9}}, {{9, 15}}, {{9, 15}}, {{9, 15}} } };
		Test.Enlarge(3);
		Compare(Test, Expected);
	}

	// Shrink
	{
		RunData Test { RunData::RowArray { {{15, 9}}, {{15, 9}}, {{15, 9}}, {{15, 9}}, {{15, 9}}, {{15, 9}}, {{9, 15}}, {{9, 15}}, {{9, 15}} } };
		RunData Expected { RunData::RowArray { {{5, 3}}, {{5, 3}}, {{3, 5}} } };
		Test.Shrink(3);
		Compare(Test, Expected);
	}

	return 0;
}
