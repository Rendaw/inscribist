// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#ifndef settings_h
#define settings_h

#include <ren-application/stringtable.h>
#include <ren-application/lightsettings.h>
#include <ren-general/color.h>
#include <ren-general/range.h>

#include "gtkwrapper.h"

RangeF const HeavyRadiusRange(4, 1000);
RangeF const LightRadiusRange(0, 100);
RangeF const DampingRange(0.1, 16);
RangeD const SizeRange(1000, 100000);
RangeD const ScaleRange(1, 100);

#ifdef SETTINGSLOCATION
String const SettingsFilename(SETTINGSLOCATION);
#else
String const SettingsFilename(GetHomeLocation() + "/.inscribist.conf");
#endif
#ifdef DATALOCATION
String const DataLocation(DATALOCATION);
#else
String const DataLocation(GetHomeLocation() + "/.inscribist/");
#endif
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
