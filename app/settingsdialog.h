// Copyright 2013 Rendaw, under the FreeBSD license (See included license.txt)

#ifndef settingsdialog_h
#define settingsdialog_h

#include "ren-gtk/gtkwrapper.h"
#include "settings.h"

class SettingsDialog : public Dialog
{
	public:
		SettingsDialog(GtkWidget *Window, SettingsData &Settings, FlatVector const &CurrentSize);
		~SettingsDialog(void);

	private:
		SettingsData &Settings;

		Title SettingsTitle;
		Scroller SettingsScroller;
		Layout SettingsBox;

		Layout DefaultDirectoryLayout;
		CheckButton EnableDefaultDirectory;
		DirectorySelect SelectDefaultDirectory;

		LayoutBorder NewImageFrame;
		Layout NewImageBox;
		Wheel NewImageWidth, NewImageHeight;

		LayoutBorder DisplayFrame;
		Layout DisplayBox;
		ColorButton DisplayPaperColor, DisplayInkColor;
		Wheel DisplayScale;

		LayoutBorder ExportFrame;
		Layout ExportBox;
		Layout ExportPaperPadding, ExportInkPadding;
		ColorButton ExportPaperColor, ExportInkColor;
		Layout ExportScaleBox;
		Wheel ExportScale;
		Label ExportSizePreview;

		struct BrushSection
		{
			BrushSection(unsigned int Index, BrushSettings &Settings, const Color &InkColor, const Color &PaperColor);
			LayoutBorder BrushFrame;
			Layout BrushBox;
			ColorToggleButton BlackToggle;
			Layout SliderBox;
			Slider HeavyDiameterSlider;
			Slider LightDiameterSlider;
		};
		std::vector<BrushSection *> BrushSections;

		struct DeviceSection : public LayoutBorder
		{
			DeviceSection(DeviceSettings &Settings, unsigned int BrushCount);
			String const Name;

			Slider DampingSlider;
			List BrushSelect;
		};
		std::vector<DeviceSection *> DeviceSections;

		Button Okay;
		Button Cancel;
};

void OpenSettings(GtkWidget *Window, SettingsData &Settings, FlatVector const &CurrentSize);

#endif
