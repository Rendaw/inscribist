#ifndef settings_h
#define settings_h

#include <application/stringtable.h>
#include <application/lightsettings.h>
#include <general/color.h>
#include <general/range.h>

const RangeF RadiusRange(8, 400);
const RangeF DampingRange(0.1, 16);
const RangeD SizeRange(800, 20000);
const RangeD ScaleRange(1, 15);
const String SettingsFilename(".inscribist.conf");
const String Extension(".inscribble");

const float BackgroundColorScale = 0.85f;

struct DeviceSettings
{
	DeviceSettings(const String &Name, bool Black, float Radius, float Damping);
	const String Name;

	bool Black;
	float Radius;
	float Damping;
};

struct SettingsData : private LightSettings
{
	public:
		SettingsData(void);
		~SettingsData(void);
		void Save(void);

		String Filename;

		FlatVector ImageSize;
		Color DisplayPaper, DisplayInk;
		Color ExportPaper, ExportInk;
		int DisplayScale, ExportScale;

		DeviceSettings &GetDeviceSettings(const String &Name);

	private:
		std::map<String, DeviceSettings *> Devices;
};

#endif
