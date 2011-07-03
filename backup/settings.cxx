#include "settings.h"

#include <iostream>

const String DataLocation("data/");
extern const StringTable Local(DataLocation + "inscribist.strings");
extern const String NewFilename(Local("fresh" + Extension));

DeviceSettings::DeviceSettings(const String &Name, bool Black, float Radius, float Damping) :
	Name(Name), Black(Black), Radius(Radius), Damping(Damping)
	{}

SettingsData::SettingsData(void) :
	LightSettings(SettingsFilename)
{
	Filename = Get("Filename", NewFilename);

	ImageSize[0] = Get("ImageSizeX", 800);
	ImageSize[1] = Get("ImageSizeY", 800);

	DisplayPaper.Red = Get("DisplayPaperRed", 0.5f);
	DisplayPaper.Green = Get("DisplayPaperGreen", 0.5f);
	DisplayPaper.Blue = Get("DisplayPaperBlue", 0.5f);
	DisplayPaper.Alpha = Get("DisplayPaperAlpha", 1.0f);

	DisplayInk.Red = Get("DisplayInkRed", 0.0f);
	DisplayInk.Green = Get("DisplayInkGreen", 0.0f);
	DisplayInk.Blue = Get("DisplayInkBlue", 0.0f);
	DisplayInk.Alpha = Get("DisplayInkAlpha", 1.0f);

	ExportPaper.Red = Get("ExportPaperRed", 0.0f);
	ExportPaper.Green = Get("ExportPaperGreen", 0.0f);
	ExportPaper.Blue = Get("ExportPaperBlue", 0.0f);
	ExportPaper.Alpha = Get("ExportPaperAlpha", 0.0f);

	DisplayScale = ScaleRange.Constrain(Get("DisplayScale", 1));

	ExportInk.Red = Get("ExportInkRed", 0.0f);
	ExportInk.Green = Get("ExportInkGreen", 0.0f);
	ExportInk.Blue = Get("ExportInkBlue", 0.0f);
	ExportInk.Alpha = Get("ExportInkAlpha", 1.0f);

	ExportScale = ScaleRange.Constrain(Get("ExportScale", 1));
}

SettingsData::~SettingsData(void)
{
	for (std::map<String, DeviceSettings *>::iterator CurrentDevice = Devices.begin();
		CurrentDevice != Devices.end(); CurrentDevice++)
		delete CurrentDevice->second;
}

void SettingsData::Save(void)
{
	Set("ImageSizeX", ImageSize[0]);
	Set("ImageSizeY", ImageSize[1]);

	Set("DisplayPaperRed", DisplayPaper.Red);
	Set("DisplayPaperGreen", DisplayPaper.Green);
	Set("DisplayPaperBlue", DisplayPaper.Blue);
	Set("DisplayPaperAlpha", DisplayPaper.Alpha);

	Set("DisplayInkRed", DisplayInk.Red);
	Set("DisplayInkGreen", DisplayInk.Green);
	Set("DisplayInkBlue", DisplayInk.Blue);
	Set("DisplayInkAlpha", DisplayInk.Alpha);

	Set("DisplayScale", DisplayScale);

	Set("ExportPaperRed", ExportPaper.Red);
	Set("ExportPaperGreen", ExportPaper.Green);
	Set("ExportPaperBlue", ExportPaper.Blue);
	Set("ExportPaperAlpha", ExportPaper.Alpha);

	Set("ExportInkRed", ExportInk.Red);
	Set("ExportInkGreen", ExportInk.Green);
	Set("ExportInkBlue", ExportInk.Blue);
	Set("ExportInkAlpha", ExportInk.Alpha);

	Set("ExportScale", ExportScale);

	for (std::map<String, DeviceSettings *>::iterator CurrentDevice = Devices.begin();
		CurrentDevice != Devices.end(); CurrentDevice++)
	{
		Set(CurrentDevice->second->Name + "_Black", CurrentDevice->second->Black);
		Set(CurrentDevice->second->Name + "_Damping", CurrentDevice->second->Damping);
		Set(CurrentDevice->second->Name + "_Radius", CurrentDevice->second->Radius);
	}

	LightSettings::Save();
}

DeviceSettings &SettingsData::GetDeviceSettings(const String &Name)
{
	// Load/default the device information if we haven't already loaded it
	if (Devices.find(Name) == Devices.end())
	{
		Devices[Name] = new DeviceSettings(Name,
			Get(Name + "_Black", true),
			Get(Name + "_Radius", 40.0f),
			Get(Name + "_Damping", 8.0f));
	}

	// Return the found/created device info
	return *Devices[Name];
}
