// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#ifndef settingsdialog_h
#define settingsdialog_h

#include "ren-gtk/gtkwrapper.h"
#include "settings.h"

class SettingsDialog : public Dialog
{
	public:
		SettingsDialog(GtkWidget *Window, SettingsData &Settings);
		~SettingsDialog(void);
		operator GtkWidget*();

	private:
		SettingsData &Settings;

		Title SettingsTitle;
		Scroller SettingsScroller;
		Layout SettingsBox;

		LayoutBorder NewImageFrame;
		Layout NewImageBox;
		Wheel NewImageWidth, NewImageHeight;

		LayoutBorder DisplayFrame;
		Layout DisplayBox;
		ColorButton DisplayPaperColor, DisplayInkColor;
		Wheel DisplayScale;

		LayoutBorder ExportFrame;
		Layout ExportBox;
		ColorButton ExportPaperColor, ExportInkColor;
		Wheel ExportScale;

		struct BrushSection
		{
			BrushSection(unsigned int Index, BrushSettings &Settings, const Color &InkColor, const Color &PaperColor);
			LayoutBorder BrushFrame;
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

			LayoutBorder DeviceFrame;
			Layout DeviceBox;
			Slider DampingSlider;
			List BrushSelect;
		};
		std::vector<DeviceSection *> DeviceSections;

		Button Okay;
		Button Cancel;
};

void OpenSettings(GtkWidget *Window, SettingsData &Settings);

#endif
