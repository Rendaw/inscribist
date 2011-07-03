// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#ifndef settingsdialog_h
#define settingsdialog_h

#include "gtkwrapper.h"
#include "settings.h"

class SettingsDialog : public ActionHandler
{
	public:
		SettingsDialog(GtkWidget *Window, SettingsData &Settings);
		~SettingsDialog(void);
		operator GtkWidget*();

		void Act(void *Instigator);

	private:
		GtkWidget *Dialog;
		SettingsData &Settings;

		GtkWidget *Title;
		GtkWidget *SettingsScroller;
		Layout SettingsBox;

		GtkWidget *NewImageFrame;
		Layout NewImageBox;
		Spinner NewImageWidth, NewImageHeight;

		GtkWidget *DisplayFrame;
		Layout DisplayBox;
		ColorButton DisplayPaperColor, DisplayInkColor;
		Spinner DisplayScale;
		GtkWidget *DisplaySeparator1, *DisplaySeparator2;

		GtkWidget *ExportFrame;
		Layout ExportBox;
		ColorButton ExportPaperColor, ExportInkColor;
		Spinner ExportScale;
		GtkWidget *ExportSeparator1, *ExportSeparator2;

		struct BrushSection
		{
			BrushSection(unsigned int Index, BrushSettings &Settings, const Color &InkColor, const Color &PaperColor);
			GtkWidget *BrushFrame;
			Layout BrushBox;
			ColorToggleButton BlackToggle;
			Layout SliderBox;
			Slider HeavyRadiusSlider;
			Slider LightRadiusSlider;
		};
		std::vector<BrushSection *> BrushSections;

		struct DeviceSection
		{
			DeviceSection(DeviceSettings &Settings, unsigned int BrushCount);
			String const Name;

			GtkWidget *DeviceFrame;
			Layout DeviceBox;
			Slider DampingSlider;
			Dropdown BrushSelect;
		};
		std::vector<DeviceSection *> DeviceSections;

		Button Okay;
		Button Cancel;
};

void OpenSettings(GtkWidget *Window, SettingsData &Settings);

#endif
