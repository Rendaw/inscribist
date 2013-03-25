// Copyright 2013 Rendaw, under the FreeBSD license (See included license.txt)

#include "settingsdialog.h"

#include "ren-translation/translation.h"

SettingsDialog::BrushSection::BrushSection(unsigned int Index, BrushSettings &Settings, const Color &InkColor, const Color &PaperColor) :
	BrushFrame(Local("Brush " + AsString(Index))),
	BrushBox(true, 3, 6),
	BlackToggle(Settings.Black, InkColor, PaperColor),
	SliderBox(false, 0, 3),
	HeavyDiameterSlider(Local("Heavy diameter (%): "), HeavyDiameterRange, Settings.HeavyDiameter),
	LightDiameterSlider(Local("Light diameter (%): "), LightDiameterRange, Settings.LightDiameter)
{
	BrushBox.Add(BlackToggle);
	SliderBox.Add(HeavyDiameterSlider);
	SliderBox.Add(LightDiameterSlider);
	BrushBox.AddFill(SliderBox);
	BrushFrame.Set(BrushBox);
}

SettingsDialog::DeviceSection::DeviceSection(DeviceSettings &Settings, unsigned int BrushCount) :
	LayoutBorder(Local("Device: ") + Settings.Name),
	Name(Settings.Name),
	DampingSlider(Local("Damping: "), DampingRange, Settings.Damping),
	BrushSelect(Local("Brush: "))
{
	Layout DeviceBox(false, 0, 3);
	DeviceBox.Add(DampingSlider);
	for (unsigned int CurrentBrush = 0; CurrentBrush < BrushCount; CurrentBrush++)
		BrushSelect.Add(Local("Brush " + AsString(CurrentBrush)));
	BrushSelect.Select(Settings.Brush);
	DeviceBox.Add(BrushSelect);
	Set(DeviceBox);
}

SettingsDialog::SettingsDialog(GtkWidget *Window, SettingsData &Settings, FlatVector const &CurrentSize) :
	Dialog(Window, Local("Settings"), {0, 500}), Settings(Settings),
	SettingsTitle(Local("Settings")),
	SettingsBox(false, 6, 2),

	DefaultDirectoryLayout(true),
	EnableDefaultDirectory(Local("Default directory for new images"), !Settings.DefaultDirectory.empty()),
	SelectDefaultDirectory("", Settings.DefaultDirectory.empty() ? 
		(String)LocateWorkingDirectory() : Settings.DefaultDirectory),

	NewImageFrame(Local("New image settings")),
	NewImageBox(true, 3, 16),
	NewImageWidth(Local("Width"), SizeRange.Including(Settings.ImageSize[0]), Settings.ImageSize[0]),
	NewImageHeight(Local("Height"), SizeRange.Including(Settings.ImageSize[1]), Settings.ImageSize[1]),

	DisplayFrame(Local("Display settings")),
	DisplayBox(true, 3, 16),
	DisplayPaperColor(Local("Paper color: "), Settings.DisplayPaper, false),
	DisplayInkColor(Local("Ink color: "), Settings.DisplayInk, false),
	DisplayScale(Local("Downscale: "), ScaleRange, ScaleRange.Constrain(Settings.DisplayScale)),

	ExportFrame(Local("Export settings")),
	ExportBox(true, 3, 16),
	ExportPaperPadding(false), ExportInkPadding(false),
	ExportPaperColor(Local("Paper color: "), Settings.ExportPaper, true),
	ExportInkColor(Local("Ink color: "), Settings.ExportInk, true),
	ExportScaleBox(false),
	ExportScale(Local("Downscale: "), ScaleRange, ScaleRange.Constrain(Settings.ExportScale)),
	ExportWidth(""), ExportHeight(""),

	Okay(Local("Okay"), diSave),
	Cancel(Local("Cancel"), diClose)
{
	//g_signal_connect_swapped((GtkWidget *)*this, "response", G_CALLBACK(gtk_widget_destroy), (GtkWidget *)*this); // Is this necessary?  Not found in ren-gtk

	Add(SettingsTitle);

	// Working directory settings
	DefaultDirectoryLayout.Add(EnableDefaultDirectory);
	SelectDefaultDirectory.SetAction([this]()
	{
		EnableDefaultDirectory.SetValue(true);
	});
	DefaultDirectoryLayout.AddFill(SelectDefaultDirectory);
	SettingsBox.Add(DefaultDirectoryLayout);

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
	auto UpdateExportLabels = [this, CurrentSize]()
	{
		ExportWidth.SetText(Local("Width: ^0", CurrentSize[0] / ExportScale.GetValue()));
		ExportHeight.SetText(Local("Height: ^0", CurrentSize[1] / ExportScale.GetValue()));
	};
	UpdateExportLabels();
	ExportScale.SetInputHandler(UpdateExportLabels);
	
	ExportPaperPadding.Add(ExportPaperColor);
	ExportBox.AddFill(ExportPaperPadding);
	ExportBox.AddSpace(); ExportBox.AddSpacer(); ExportBox.AddSpace();
	ExportInkPadding.Add(ExportInkColor);
	ExportBox.AddFill(ExportInkPadding);
	ExportBox.AddSpace(); ExportBox.AddSpacer(); ExportBox.AddSpace();
	ExportScaleBox.Add(ExportScale);
	ExportScaleBox.Add(ExportWidth);
	ExportScaleBox.Add(ExportHeight);
	ExportBox.AddFill(ExportScaleBox);
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

		SettingsBox.Add(*DeviceSections.back());
	}

	SettingsScroller.Set(SettingsBox);
	AddFill(SettingsScroller);

	/// Populate the dialog actions
	Okay.SetAction([&]()
	{
		Settings.DefaultDirectory = EnableDefaultDirectory.GetValue() ?  SelectDefaultDirectory.GetValue() : String();
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
			BrushSettings.HeavyDiameter = BrushSections[CurrentBrush]->HeavyDiameterSlider.GetValue();
			BrushSettings.LightDiameter = BrushSections[CurrentBrush]->LightDiameterSlider.GetValue();
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

void OpenSettings(GtkWidget *Window, SettingsData &Settings, FlatVector const &CurrentSize)
{
	SettingsDialog Dialog(Window, Settings, CurrentSize);
	Dialog.Run();
}
