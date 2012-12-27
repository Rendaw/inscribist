// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#include "settingsdialog.h"

#include "localization.h"

SettingsDialog::BrushSection::BrushSection(unsigned int Index, BrushSettings &Settings, const Color &InkColor, const Color &PaperColor) :
	BrushFrame(Local("Brush " + AsString(Index))),
	BrushBox(true, 3, 6),
	BlackToggle(Settings.Black, InkColor, PaperColor),
	SliderBox(false, 0, 3),
	HeavyRadiusSlider(Local("Size: "), HeavyRadiusRange, Settings.HeavyRadius),
	LightRadiusSlider(Local("Light size: "), LightRadiusRange, Settings.LightRadius)
{
	BrushBox.Add(BlackToggle);
	SliderBox.Add(HeavyRadiusSlider);
	SliderBox.Add(LightRadiusSlider);
	BrushBox.AddFill(SliderBox);
	BrushFrame.Set(BrushBox);
}

SettingsDialog::DeviceSection::DeviceSection(DeviceSettings &Settings, unsigned int BrushCount) :
	Name(Settings.Name),
	DeviceFrame(Local("Device: ") + Settings.Name),
	DeviceBox(false, 0, 3),
	DampingSlider(Local("Damping: "), DampingRange, Settings.Damping),
	BrushSelect(Local("Brush: "))
{
	for (unsigned int CurrentBrush = 0; CurrentBrush < BrushCount; CurrentBrush++)
		BrushSelect.Add(Local("Brush " + AsString(CurrentBrush)));
	BrushSelect.Select(Settings.Brush);

	DeviceBox.Add(DampingSlider);
	DeviceBox.Add(BrushSelect);
	DeviceFrame.Set(DeviceBox);
}

SettingsDialog::SettingsDialog(GtkWidget *Window, SettingsData &Settings) :
	Dialog(Window, Local("Settings"), {0, 500}), Settings(Settings),
	SettingsTitle(Local("Settings")),
	SettingsBox(false, 6, 2),

	NewImageFrame(Local("New image settings")),
	NewImageBox(true, 3, 16),
	NewImageWidth(Local("Width"), SizeRange.Including(Settings.ImageSize[0]), Settings.ImageSize[0]),
	NewImageHeight(Local("Height"), SizeRange.Including(Settings.ImageSize[1]), Settings.ImageSize[1]),

	DisplayFrame(Local("Display settings")),
	DisplayBox(true, 3, 16),
	DisplayPaperColor(Local("Paper color: "), Settings.DisplayPaper, false),
	DisplayInkColor(Local("Ink color: "), Settings.DisplayInk, false),
	DisplayScale(Local("New image downscale: "), ScaleRange, ScaleRange.Constrain(Settings.DisplayScale)),

	ExportFrame(Local("Export settings")),
	ExportBox(true, 3, 16),
	ExportPaperColor(Local("Paper color: "), Settings.ExportPaper, true),
	ExportInkColor(Local("Ink color: "), Settings.ExportInk, true),
	ExportScale(Local("Downscale: "), ScaleRange, ScaleRange.Constrain(Settings.ExportScale)),

	Okay(Local("Okay"), diSave),
	Cancel(Local("Cancel"), diClose)
{
	//g_signal_connect_swapped(Dialog, "response", G_CALLBACK(gtk_widget_destroy), Dialog); // Is this necessary?  Not found in ren-gtk

	Add(SettingsTitle);

	/// New image settings
	NewImageBox.AddFill(NewImageWidth);
	NewImageBox.AddFill(NewImageHeight);
	NewImageFrame.Set(NewImageBox);
	SettingsBox.Add(NewImageFrame);

	/// Display settings
	DisplayPaperColor.SetAction([&]()
	{
		for (std::vector<BrushSection *>::iterator CurrentSection = BrushSections.begin();
			CurrentSection != BrushSections.end(); CurrentSection++)
			(*CurrentSection)->BlackToggle.SetBackgroundColor(DisplayPaperColor.GetColor());
	});
	DisplayBox.AddFill(DisplayPaperColor);

	DisplayBox.AddSpace(); DisplayBox.AddSpacer(); DisplayBox.AddSpace();

	DisplayInkColor.SetAction([&]()
	{
		for (std::vector<BrushSection *>::iterator CurrentSection = BrushSections.begin();
			CurrentSection != BrushSections.end(); CurrentSection++)
			(*CurrentSection)->BlackToggle.SetForegroundColor(DisplayInkColor.GetColor());
	});
	DisplayBox.AddFill(DisplayInkColor);

	DisplayBox.AddSpace(); DisplayBox.AddSpacer(); DisplayBox.AddSpace();
	DisplayBox.AddFill(DisplayScale);
	DisplayFrame.Set(DisplayBox);
	SettingsBox.Add(DisplayFrame);

	/// Export settings
	ExportBox.AddFill(ExportPaperColor);
	ExportBox.AddSpace(); ExportBox.AddSpacer(); ExportBox.AddSpace();
	ExportBox.AddFill(ExportInkColor);
	ExportBox.AddSpace(); ExportBox.AddSpacer(); ExportBox.AddSpace();
	ExportBox.AddFill(ExportScale);
	ExportFrame.Set(ExportBox);
	SettingsBox.Add(ExportFrame);

	/// Brush settings
	for (unsigned int CurrentBrush = 0; CurrentBrush < Settings.GetBrushCount(); CurrentBrush++)
	{
		unsigned int ActualCurrentBrush = (CurrentBrush + 1) % 10;
		BrushSections.push_back(new BrushSection(ActualCurrentBrush,
			Settings.GetBrushSettings(ActualCurrentBrush), DisplayInkColor.GetColor(), DisplayPaperColor.GetColor()));
		SettingsBox.Add(BrushSections.back()->BrushFrame);
	}

	/// Device settings
	GList *InputDevices = gdk_devices_list();
	for (GList *DeviceIterator = InputDevices; DeviceIterator != NULL; DeviceIterator = DeviceIterator->next)
	{
		GdkDevice *SystemDevice = (GdkDevice*)DeviceIterator->data;

		DeviceSections.push_back(new DeviceSection(
			Settings.GetDeviceSettings(gdk_device_get_name(SystemDevice)), Settings.GetBrushCount()));

		SettingsBox.Add(DeviceSections.back()->DeviceFrame);
	}

	SettingsScroller.Set(SettingsBox);
	AddFill(SettingsScroller);

	/// Populate the dialog actions
	Okay.SetAction([&]()
	{
		Settings.ImageSize[0] = NewImageWidth.GetValue();
		Settings.ImageSize[1] = NewImageHeight.GetValue();
		Settings.DisplayPaper = DisplayPaperColor.GetColor();
		Settings.DisplayInk = DisplayInkColor.GetColor();
		Settings.DisplayScale = DisplayScale.GetValue();
		Settings.ExportPaper = ExportPaperColor.GetColor();
		Settings.ExportInk = ExportInkColor.GetColor();
		Settings.ExportScale = ExportScale.GetValue();

		for (unsigned int CurrentBrush = 0; CurrentBrush < BrushSections.size(); CurrentBrush++)
		{
			BrushSettings &BrushSettings = Settings.GetBrushSettings((CurrentBrush + 1) % 10);
			BrushSettings.Black = BrushSections[CurrentBrush]->BlackToggle.GetState();
			BrushSettings.HeavyRadius = BrushSections[CurrentBrush]->HeavyRadiusSlider.GetValue();
			BrushSettings.LightRadius = BrushSections[CurrentBrush]->LightRadiusSlider.GetValue();
		}

		for (std::vector<DeviceSection *>::iterator CurrentDeviceSection = DeviceSections.begin();
			CurrentDeviceSection != DeviceSections.end(); CurrentDeviceSection++)
		{
			DeviceSettings &DeviceSettings = Settings.GetDeviceSettings((*CurrentDeviceSection)->Name);
			DeviceSettings.Damping = (*CurrentDeviceSection)->DampingSlider.GetValue();
			DeviceSettings.Brush = (*CurrentDeviceSection)->BrushSelect.GetSelection();
		}

		Settings.Save();
		Close();
	});
	AddAction(Okay);
	
	Cancel.SetAction([&]() { Close(); });
	AddAction(Cancel);
}

SettingsDialog::~SettingsDialog(void)
{
	for (std::vector<DeviceSection *>::iterator CurrentDeviceSection = DeviceSections.begin();
		CurrentDeviceSection != DeviceSections.end(); CurrentDeviceSection++)
		delete *CurrentDeviceSection;

	for (std::vector<BrushSection *>::iterator CurrentBrushSection = BrushSections.begin();
		CurrentBrushSection != BrushSections.end(); CurrentBrushSection++)
		delete *CurrentBrushSection;
}

void OpenSettings(GtkWidget *Window, SettingsData &Settings)
{
	SettingsDialog Dialog(Window, Settings);
	Dialog.Run();
}
