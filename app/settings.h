// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#ifndef settings_h
#define settings_h

#include "ren-application/stringtable.h"
#include "ren-application/lightsettings.h"
#include "ren-general/color.h"
#include "ren-general/range.h"
#include "ren-general/filesystem.h"

#include "gtkwrapper.h"

RangeF const DampingRange(0.1, 16);
RangeD const SizeRange(1000, 1000000);
unsigned int const SizeDefault(20000);
RangeF const HeavyRadiusRange(4, SizeRange.Max);
float const HeavyRadiusDefault(20);
RangeF const LightRadiusRange(0, SizeRange.Max);
float const LightRadiusDefault(5);
RangeD const ScaleRange(1, SizeRange.Max / 500); // Allow users to zoom out to around 500x500 px
unsigned int const DisplayScaleDefault = std::max(1u, (SizeDefault / 2000));
unsigned int const ExportScaleDefault = std::max(1u, (SizeDefault / 2000));

DirectoryPath const DataLocation(DATALOCATION);
String const Extension(".inscribble");

float const BackgroundColorScale = 0.85f;

struct BrushSettings
{
	BrushSettings(bool Black, float HeavyRadius, float LightRadius);
	bool Black;
	float HeavyRadius;
	float LightRadius;
};

struct DeviceSettings
{
	DeviceSettings(String const &Name, float Damping, unsigned int Brush);
	String const Name;

	float Damping;
	unsigned int Brush;
};

struct SettingsData : private LightSettings
{
	public:
		SettingsData(void);
		~SettingsData(void);
		void Save(void);

		String Filename; // (Unsaved, but loaded)

		FlatVector ImageSize;
		Color DisplayPaper, DisplayInk;
		Color ExportPaper, ExportInk;
		int DisplayScale, ExportScale;

		DeviceSettings &GetDeviceSettings(String const &Name);

		unsigned int GetBrushCount(void);
		BrushSettings &GetBrushSettings(unsigned int const Index);

	private:
		std::map<String, DeviceSettings *> Devices;
		std::vector<BrushSettings *> Brushes;
};

#endif
