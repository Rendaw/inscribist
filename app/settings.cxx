// Copyright 2013 Rendaw, under the FreeBSD license (See included license.txt)

#include "settings.h"

#include <cassert>
#include <iostream>

#include "ren-script/script.h"
#include "ren-script/databuilder.h"

LightSettings::LightSettings(const String &Location) :
	Filename(Location)
	{ Refresh(); }

void LightSettings::Save(void)
{
	FileOutput Out(Filename.c_str(), FileOutput::Modes::Erase);
	Out << "return {\n";

	ScriptDataBuilder ScriptOut(Out, 1);

	for (auto const &CurrentValue : Values)
		ScriptOut.Key(CurrentValue.first).Value(CurrentValue.second);

	Out << "\n}\n";
}

void LightSettings::Refresh(void)
{
	Values.clear();

	Script In;
	In.Do(Filename, true);

	if ((In.Height() > 0) && In.IsTable())
		In.Iterate([this](Script &State) -> bool
		{
			String Value = State.GetString();
			State.Duplicate(-1);
			Values[State.GetString()] = Value;
			return true;
		});
}

void LightSettings::Unset(const String &Value)
{
	int Count = Values.erase(Value);
	assert(Count == 1);
}

BrushSettings::BrushSettings(bool Black, float HeavyDiameter, float LightDiameter) :
	Black(Black), HeavyDiameter(HeavyDiameter), LightDiameter(LightDiameter)
	{}

DeviceSettings::DeviceSettings(String const &Name, float Damping, unsigned int Brush) :
	Name(Name), Damping(Damping), Brush(Brush)
	{}

SettingsData::SettingsData(void) :
	LightSettings(LocateUserConfigFile("inscribist.conf"))
{
	DefaultDirectory = Get("DefaultDirectory", String());

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
		float const HeavyDiameterDefault = 0.7964f;
		Brushes.push_back(new BrushSettings(
			Get("Brush" + AsString(CurrentBrush) + "_Black", true),
			HeavyDiameterRange.Constrain(Get("Brush" + AsString(CurrentBrush) + "_HeavyDiameter", HeavyDiameterDefault)),
			LightDiameterRange.Constrain(Get("Brush" + AsString(CurrentBrush) + "_LightDiameter", LightDiameterDefault))));
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
	Set("DefaultDirectory", DefaultDirectory);

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
		Set("Brush" + AsString(CurrentBrush) + "_HeavyDiameter", Brushes[CurrentBrush]->HeavyDiameter);
		Set("Brush" + AsString(CurrentBrush) + "_LightDiameter", Brushes[CurrentBrush]->LightDiameter);
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
			RangeD(0, 9).Constrain(Get(Name + "_Brush", 1)));
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
