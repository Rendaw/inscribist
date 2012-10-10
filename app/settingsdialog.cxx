// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#include "settingsdialog.h"

#include "localization.h"

SettingsDialog::BrushSection::BrushSection(unsigned int Index, BrushSettings &Settings, const Color &InkColor, const Color &PaperColor) :
	BrushFrame(gtk_frame_new(Local("Brush " + AsString(Index)).c_str())),
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

	gtk_container_add(GTK_CONTAINER(BrushFrame), BrushBox);
	gtk_widget_show(BrushBox);
}

SettingsDialog::DeviceSection::DeviceSection(DeviceSettings &Settings, unsigned int BrushCount) :
	Name(Settings.Name),
	DeviceFrame(gtk_frame_new((Local("Device: ") + Settings.Name).c_str())),
	DeviceBox(false, 0, 3),
	DampingSlider(Local("Damping: "), DampingRange, Settings.Damping),
	BrushSelect(Local("Brush: "))
{
	for (unsigned int CurrentBrush = 0; CurrentBrush < BrushCount; CurrentBrush++)
		BrushSelect.Add(Local("Brush " + AsString(CurrentBrush)));
	BrushSelect.SetValue(Settings.Brush);

	DeviceBox.Add(DampingSlider);
	DeviceBox.Add(BrushSelect);
	gtk_container_add(GTK_CONTAINER(DeviceFrame), DeviceBox);
	gtk_widget_show(DeviceBox);
}

SettingsDialog::SettingsDialog(GtkWidget *Window, SettingsData &Settings) :
	Dialog(gtk_dialog_new()), Settings(Settings),
	Title(gtk_label_new(NULL)),
	SettingsScroller(gtk_scrolled_window_new(NULL, NULL)),
	SettingsBox(false, 6, 2),

	NewImageFrame(gtk_frame_new(Local("New image settings").c_str())),
	NewImageBox(true, 3, 16),
	NewImageWidth(Local("Width"), SizeRange.Including(Settings.ImageSize[0]), Settings.ImageSize[0]),
	NewImageHeight(Local("Height"), SizeRange.Including(Settings.ImageSize[1]), Settings.ImageSize[1]),

	DisplayFrame(gtk_frame_new(Local("Display settings").c_str())),
	DisplayBox(true, 3, 16),
	DisplayPaperColor(Local("Paper color: "), Settings.DisplayPaper, this),
	DisplayInkColor(Local("Ink color: "), Settings.DisplayInk, this),
	DisplayScale(Local("New image downscale: "), ScaleRange, ScaleRange.Constrain(Settings.DisplayScale)),
	DisplaySeparator1(gtk_vseparator_new()), DisplaySeparator2(gtk_vseparator_new()),

	ExportFrame(gtk_frame_new(Local("Export settings").c_str())),
	ExportBox(true, 3, 16),
	ExportPaperColor(Local("Paper color: "), Settings.ExportPaper, NULL),
	ExportInkColor(Local("Ink color: "), Settings.ExportInk, NULL),
	ExportScale(Local("Downscale: "), ScaleRange, ScaleRange.Constrain(Settings.ExportScale)),
	ExportSeparator1(gtk_vseparator_new()), ExportSeparator2(gtk_vseparator_new()),

	Okay(Local("Okay"), GTK_STOCK_SAVE, this),
	Cancel(Local("Cancel"), GTK_STOCK_CLOSE, this)
{
	/// Set up the dialog widget
	gtk_window_set_default_size(GTK_WINDOW(Dialog), 0, 500);
	gtk_window_set_title(GTK_WINDOW(Dialog), Local("Settings").c_str());
	gtk_container_set_reallocate_redraws(GTK_CONTAINER(Dialog), true);
	gtk_window_set_transient_for(GTK_WINDOW(Dialog), GTK_WINDOW(Window));
	g_signal_connect_swapped(Dialog, "response", G_CALLBACK(gtk_widget_destroy), Dialog);

	/// Populate the main dialog area
	char *MarkupString = g_markup_printf_escaped("<span size=\"large\" weight=\"bold\">%s</span>", Local("Settings").c_str());
	gtk_label_set_markup(GTK_LABEL(Title), MarkupString);
	g_free(MarkupString);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(Dialog))), Title, false, true, 3);
	gtk_widget_show(Title);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(SettingsScroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	/// New image settings
	NewImageBox.AddFill(NewImageWidth);
	NewImageBox.AddFill(NewImageHeight);
	gtk_container_add(GTK_CONTAINER(NewImageFrame), NewImageBox);
	gtk_widget_show(NewImageBox);
	SettingsBox.Add(NewImageFrame);

	/// Display settings
	DisplayBox.AddFill(DisplayPaperColor);
	DisplayBox.AddFill(DisplaySeparator1);
	DisplayBox.AddFill(DisplayInkColor);
	DisplayBox.AddFill(DisplaySeparator2);
	DisplayBox.AddFill(DisplayScale);
	gtk_container_add(GTK_CONTAINER(DisplayFrame), DisplayBox);
	gtk_widget_show(DisplayBox);
	SettingsBox.Add(DisplayFrame);

	/// Export settings
	ExportBox.AddFill(ExportPaperColor);
	ExportBox.AddFill(ExportSeparator1);
	ExportBox.AddFill(ExportInkColor);
	ExportBox.AddFill(ExportSeparator2);
	ExportBox.AddFill(ExportScale);
	gtk_container_add(GTK_CONTAINER(ExportFrame), ExportBox);
	gtk_widget_show(ExportBox);
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

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(SettingsScroller), SettingsBox);
	gtk_widget_show(SettingsBox);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(Dialog))), SettingsScroller, true, true, 0);
	gtk_widget_show(SettingsScroller);

	/// Populate the dialog actions
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(Dialog))), Okay, false, true, 0);
	gtk_widget_show(Okay);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(Dialog))), Cancel, false, true, 0);
	gtk_widget_show(Cancel);
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

void SettingsDialog::Act(void *Instigator)
{
	if (Instigator == &DisplayPaperColor)
	{
		for (std::vector<BrushSection *>::iterator CurrentSection = BrushSections.begin();
			CurrentSection != BrushSections.end(); CurrentSection++)
			(*CurrentSection)->BlackToggle.SetBackgroundColor(DisplayPaperColor.GetColor());
	}

	if (Instigator == &DisplayInkColor)
	{
		for (std::vector<BrushSection *>::iterator CurrentSection = BrushSections.begin();
			CurrentSection != BrushSections.end(); CurrentSection++)
			(*CurrentSection)->BlackToggle.SetForegroundColor(DisplayInkColor.GetColor());
	}

	if (Instigator == &Okay)
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
			DeviceSettings.Brush = (*CurrentDeviceSection)->BrushSelect.GetValue();
		}

		Settings.Save();
		gtk_dialog_response(GTK_DIALOG(Dialog), GTK_RESPONSE_NONE);
		return;
	}

	if (Instigator == &Cancel)
	{
		gtk_dialog_response(GTK_DIALOG(Dialog), GTK_RESPONSE_NONE);
		return;
	}
}

SettingsDialog::operator GtkWidget*()
	{ return Dialog; }

void OpenSettings(GtkWidget *Window, SettingsData &Settings)
{
	SettingsDialog Dialog(Window, Settings);
	gtk_dialog_run(GTK_DIALOG((GtkWidget *)Dialog));
}
