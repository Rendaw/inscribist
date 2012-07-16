// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#include "settings.h"

#include <cassert>
#include <iostream>

extern StringTable const Local(DataLocation.Select("inscribist.strings"));
extern String const NewFilename(Local("fresh" + Extension));

BrushSettings::BrushSettings(bool Black, float HeavyRadius, float LightRadius) :
	Black(Black), HeavyRadius(HeavyRadius), LightRadius(LightRadius)
	{}

DeviceSettings::DeviceSettings(String const &Name, float Damping, unsigned int Brush) :
	Name(Name), Damping(Damping), Brush(Brush)
	{}

SettingsData::SettingsData(void) :
	LightSettings(LocateUserConfigFile("inscribist.conf"))
{
	ImageSize[0] = Get("ImageSizeX", SizeDefault);
	ImageSize[1] = Get("ImageSizeY", SizeDefault);

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

	DisplayScale = ScaleRange.Constrain(Get("DisplayScale", DisplayScaleDefault));

	ExportInk.Red = Get("ExportInkRed", 0.0f);
	ExportInk.Green = Get("ExportInkGreen", 0.0f);
	ExportInk.Blue = Get("ExportInkBlue", 0.0f);
	ExportInk.Alpha = Get("ExportInkAlpha", 1.0f);

	ExportScale = ScaleRange.Constrain(Get("ExportScale", ExportScaleDefault));

	for (unsigned int CurrentBrush = 0; CurrentBrush < 10; CurrentBrush++)
	{
		Brushes.push_back(new BrushSettings(
			Get("Brush" + AsString(CurrentBrush) + "_Black", true),
			HeavyRadiusRange.Constrain(Get("Brush" + AsString(CurrentBrush) + "_HeavyRadius", HeavyRadiusDefault)),
			LightRadiusRange.Constrain(Get("Brush" + AsString(CurrentBrush) + "_LightRadius", LightRadiusDefault))));
	}
}

SettingsData::~SettingsData(void)
{
	for (std::map<String, DeviceSettings *>::iterator CurrentDevice = Devices.begin();
		CurrentDevice != Devices.end(); CurrentDevice++)
		delete CurrentDevice->second;

	for (std::vector<BrushSettings *>::iterator CurrentBrush = Brushes.begin();
		CurrentBrush != Brushes.end(); CurrentBrush++)
		delete *CurrentBrush;
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
		Set(CurrentDevice->second->Name + "_Damping", CurrentDevice->second->Damping);
		Set(CurrentDevice->second->Name + "_Brush", CurrentDevice->second->Brush);
	}

	for (unsigned int CurrentBrush = 0; CurrentBrush < Brushes.size(); CurrentBrush++)
	{
		Set("Brush" + AsString(CurrentBrush) + "_Black", Brushes[CurrentBrush]->Black);
		Set("Brush" + AsString(CurrentBrush) + "_HeavyRadius", Brushes[CurrentBrush]->HeavyRadius);
		Set("Brush" + AsString(CurrentBrush) + "_LightRadius", Brushes[CurrentBrush]->LightRadius);
	}

	LightSettings::Save();
}

DeviceSettings &SettingsData::GetDeviceSettings(String const &Name)
{
	// Load/default the device information if we haven't already loaded it
	if (Devices.find(Name) == Devices.end())
	{
		Devices[Name] = new DeviceSettings(Name,
			Get(Name + "_Damping", 0.45f),
			RangeD(0, 9).Constrain(Get(Name + "_Brush", 0)));
	}

	// Return the found/created device info
	return *Devices[Name];
}

unsigned int SettingsData::GetBrushCount(void)
	{ return Brushes.size(); }

BrushSettings &SettingsData::GetBrushSettings(unsigned int const Index)
{
	assert(Index < Brushes.size());
	return *Brushes[Index];
}
