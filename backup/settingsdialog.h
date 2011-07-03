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

		struct DeviceSection
		{
			DeviceSection(DeviceSettings &Settings, const Color &InkColor, const Color &PaperColor);
			const String Name;

			GtkWidget *DeviceFrame;
			Layout DeviceBox;
			ColorToggleButton BlackToggle;
			Layout SliderBox;
			Slider RadiusSlider;
			Slider DampingSlider;
		};
		std::vector<DeviceSection *> DeviceSections;

		Button Okay;
		Button Cancel;
};

void OpenSettings(GtkWidget *Window, SettingsData &Settings);

#endif
