#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string>
#include <iostream>
#include <cassert>
#include <cmath>

#include <general/vector.h>
#include <general/range.h>

#include "gtkwrapper.h"
#include "drawinginterface.h"
#include "settingsdialog.h"
#include "localization.h"

extern const float SliderFactor, InverseSliderFactor;

#include "runsketcher.h"
#include "simplesketcher.h"
DrawingInterface *CreateSketcher(const FlatVector &Size, int Scale)
{
	return new RunSketcher(Size, Scale);
}

DrawingInterface *CreateSketcher(const String &Filename, const FlatVector &Size, int Scale)
{
	return new RunSketcher(Filename, Size, Scale);
}

class MainWindow : public ActionHandler
{
	public:
		// Auxiliary
		void UpdateState(CursorState &State, const FlatVector &NewPosition, GdkDevice *CurrentDevice)
		{
			if (CurrentDevice != State.LastDevice)
			{
				//State.Mode = CursorState::mFree;
				State.LastDevice = CurrentDevice;
				State.Settings = &Settings.GetDeviceSettings(gdk_device_get_name(CurrentDevice));
			}

			double AxisRawData[gdk_device_get_n_axes(CurrentDevice)];
			gdk_device_get_state(CurrentDevice, Canvas->window, AxisRawData, NULL);

			double Found;
			State.Position = NewPosition - ImageOffset;
			if (gdk_device_get_axis(CurrentDevice, AxisRawData, GDK_AXIS_PRESSURE, &Found))
				if (Found < 0.05f) State.Radius = 0.0f;
				else State.Radius = State.Settings->Radius * std::max(0.04f, powf(Found, State.Settings->Damping));
			else State.Radius = State.Settings->Radius;
		}

		void SizeCanvasAppropriately(void)
		{
			// Size the window so that we can in any direction until the corner of the canvas
			// is in the center of the display.
			GtkWidget *CanvasViewport = gtk_bin_get_child(GTK_BIN(Scroller));
			assert(CanvasViewport != NULL);
			ImageOffset[0] = CanvasViewport->allocation.width * 0.5f;
			ImageOffset[1] = CanvasViewport->allocation.height * 0.5f;

			const FlatVector ImageSize = Sketcher->GetDisplaySize();

			gtk_widget_set_size_request(Canvas,
				ImageOffset[0] * 2.0f + ImageSize[0], ImageOffset[1] * 2.0f + ImageSize[1]);
		}

		FlatVector GetImageFocusPercent(void)
		{
			GtkAdjustment *VAdjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Scroller)),
				*HAdjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Scroller));

			const FlatVector
				AdjustmentCenter(
					gtk_adjustment_get_value(HAdjustment) + gtk_adjustment_get_page_size(HAdjustment) * 0.5f,
					gtk_adjustment_get_value(VAdjustment) + gtk_adjustment_get_page_size(VAdjustment) * 0.5f),
				AdjustmentUpper(gtk_adjustment_get_upper(HAdjustment), gtk_adjustment_get_upper(VAdjustment)),
				AdjustmentLower(gtk_adjustment_get_lower(HAdjustment), gtk_adjustment_get_lower(VAdjustment)),
				CanvasSize(Canvas->allocation.width, Canvas->allocation.height),
				ImageSize = Sketcher->GetDisplaySize();

			FlatVector Out = (((AdjustmentCenter - AdjustmentLower) / (AdjustmentUpper - AdjustmentLower)) *
				CanvasSize - ImageOffset) / ImageSize;
			Out[0] = RangeF(0, 1).Constrain(Out[0]);
			Out[1] = RangeF(0, 1).Constrain(Out[1]);

			return Out;
		}

		// Event handlers
		gboolean Type(GdkEventKey *Event)
		{
			if ((Event->keyval == GDK_KEY_Tab) ||
				(Event->keyval == GDK_KEY_plus) ||
				(Event->keyval == GDK_KEY_space))
			{
				/// Toggle draw mode key pressed
				if (State.Settings != NULL)
					State.Settings->Black = !State.Settings->Black;
			}
			else if (RangeD(GDK_KEY_0, GDK_KEY_9).Contains(Event->keyval))
			{
				/// Change brush size key pressed
				if (State.Settings != NULL)
				{
					int Base = Event->keyval - GDK_KEY_0 - 1;
					if (Base == -1) Base = 9;
					State.Settings->Radius = RadiusRange.Constrain(RadiusRange.Max *
						powf((float)Base / 9.0f, 2));
				}
			}
			else if ((Event->keyval == GDK_bracketleft) || (Event->keyval == GDK_bracketright))
			{
				/// Zoom key pressed
				// Save the focus
				if (!LookingAtSet)
					LookingAt = GetImageFocusPercent();
				LookingAtSet = true;

				// Do the zoom
				if (Event->keyval == GDK_bracketleft)
					Settings.DisplayScale = Sketcher->Zoom(1);
				else Settings.DisplayScale = Sketcher->Zoom(-1);

				// Resize the window
				SizeCanvasAppropriately();
			}
			else if ((Event->keyval == GDK_KEY_s) && (Event->state & GDK_CONTROL_MASK))
			{
				/// Save combo
				// If using a default file name, show a prompt
				if (Settings.Filename == NewFilename)
					Act(SaveAction);
				else // Otherwise, just save
					Sketcher->Save(Settings.Filename);
			}
			else return TRUE; // We didn't eat the key

			return FALSE;
		}

		void Draw(GdkEventExpose *Event)
		{
			cairo_t *CairoContext = gdk_cairo_create(Event->window);

			cairo_rectangle(CairoContext, (int)Event->area.x, (int)Event->area.y,
				(int)Event->area.width + 1, (int)Event->area.height + 1);
			cairo_clip(CairoContext);

			cairo_translate(CairoContext, (int)ImageOffset[0], (int)ImageOffset[1]);

			/// Paint the image
			Sketcher->Render(
				Region(FlatVector(Event->area.x, Event->area.y) - ImageOffset, FlatVector(Event->area.width, Event->area.height)),
				CairoContext, Settings.DisplayInk, Settings.DisplayPaper);

			cairo_destroy(CairoContext);
		}

		void Click(GdkEventButton *Event)
		{
			if (Event->type != GDK_BUTTON_PRESS) return; // Ignore double, triple-click extra events

			LastState = State;
			UpdateState(State, FlatVector(Event->x, Event->y), Event->device);

			if (Event->button == 2) State.Mode = CursorState::mPanning;
			else State.Mode = CursorState::mMarking;
		}

		void Declick(GdkEventButton *Event)
		{
			LastState = State;
			UpdateState(State, FlatVector(Event->x, Event->y), Event->device);

			State.Mode = CursorState::mFree;
		}

		void Move(GdkEventMotion *Event)
		{
			LastState = State;
			UpdateState(State, FlatVector(Event->x, Event->y), Event->device);

			if ((State.Mode == CursorState::mMarking) && (State.Radius > 0.0f))
			{
				// Mark
				const Region Marked = Sketcher->Mark(LastState, State, State.Settings->Black);

				// Refresh the marked area
				const GdkRectangle Region =
				{
					Marked.Start[0] + ImageOffset[0] - 2,
					Marked.Start[1] + ImageOffset[1] - 2,
					Marked.Size[0] + 4,
					Marked.Size[1] + 4
				};

				gdk_window_invalidate_rect(Canvas->window, &Region, false);
				//gdk_window_process_updates(Canvas->window, false); // Preserves events when laggy, bad idea.
			}
			else if (State.Mode == CursorState::mPanning)
			{
				/// Scroll the scroller
				const FlatVector Offset = -(State.Position - LastState.Position);

				// NOTE manually clips value becase gtk is broken
				GtkAdjustment *Adjustment;

				Adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Scroller));
				gtk_adjustment_set_value(Adjustment,
					RangeF(0, gtk_adjustment_get_upper(Adjustment) - gtk_adjustment_get_page_size(Adjustment)).Constrain(
						gtk_adjustment_get_value(Adjustment) + Offset[0]));

				Adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Scroller));
				gtk_adjustment_set_value(Adjustment,
					RangeF(0, gtk_adjustment_get_upper(Adjustment) - gtk_adjustment_get_page_size(Adjustment)).Constrain(
						gtk_adjustment_get_value(Adjustment) + Offset[1]));

				State.Position = LastState.Position;
			}
		}

		void ResizeImage(GdkEventConfigure *Event)
		{
			// Position the canvas appropriately in the window
			const FlatVector NewArea(Event->width, Event->height),
				CanvasSize(Sketcher->GetDisplaySize());
			ImageOffset = NewArea * 0.5f - CanvasSize * 0.5f;

			// Scroll so that we're looking at the same thing we used to be looking at
			//assert(LookingAtSet);
			if (LookingAtSet)
			{
				GtkAdjustment *VAdjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Scroller)),
					*HAdjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Scroller));

				LookingAt = (LookingAt * CanvasSize + ImageOffset) / NewArea *
					FlatVector(
						gtk_adjustment_get_upper(HAdjustment) - gtk_adjustment_get_lower(HAdjustment),
						gtk_adjustment_get_upper(VAdjustment) - gtk_adjustment_get_lower(VAdjustment)) +
					FlatVector(
						gtk_adjustment_get_lower(HAdjustment) - gtk_adjustment_get_page_size(HAdjustment) * 0.5f,
						gtk_adjustment_get_lower(VAdjustment) - gtk_adjustment_get_page_size(VAdjustment) * 0.5f);

				gtk_adjustment_set_value(HAdjustment, LookingAt[0]);
				gtk_adjustment_set_value(VAdjustment, LookingAt[1]);

				LookingAtSet = false;
			}
		}

		void ResizeImageViewport(GdkEventConfigure *Event)
		{
			if (!LookingAtSet)
				LookingAt = GetImageFocusPercent();
			LookingAtSet = true;
			SizeCanvasAppropriately();
		}

		gboolean ConfirmClose(void)
		{
			return !Sketcher->HasChanges() || Confirm(Window, Local("Quit Inscribist"),
				Local("Are you sure you wish to close Inscribist?  Any unsaved changes will be lost."));
		}

		void Close(void)
		{
			gtk_main_quit();
		}

		// Static callback farm
		static gboolean KeyCallback(GtkWidget *Widget, GdkEventKey *Event, MainWindow *This)
			{ return This->Type(Event); }

		static gboolean DrawCallback(GtkWidget *Widget, GdkEventExpose *Event, MainWindow *This)
			{ This->Draw(Event); return TRUE; }

		static gboolean ClickCallback(GtkWidget *Widget, GdkEventButton *Event, MainWindow *This)
			{ This->Click(Event); return FALSE; }

		static gboolean DeclickCallback(GtkWidget *Widget, GdkEventButton *Event, MainWindow *This)
			{ This->Declick(Event); return FALSE; }

		static gboolean MoveCallback(GtkWidget *Widget, GdkEventMotion *Event, MainWindow *This)
			{ This->Move(Event); return FALSE; }

		static gboolean ResizeCanvasCallback(GtkWidget *Widget, GdkEventConfigure *Event, MainWindow *This)
			{ This->ResizeImage(Event); return FALSE; }

		static gboolean ResizeViewportCallback(GtkWidget *Widget, GdkEventConfigure *Event, MainWindow *This)
			{ This->ResizeImageViewport(Event); return FALSE; }

		static gboolean DeleteCallback(GtkWidget *Source, GdkEvent *Event, MainWindow *This)
			{ return !This->ConfirmClose(); }

		static void DestroyCallback(GtkWidget *Source, MainWindow *This)
			{ This->Close(); }

		// Constructor, the meat of our salad
		MainWindow(int ArgumentCount, char **Arguments) :
			Window(gtk_window_new(GTK_WINDOW_TOPLEVEL)),

			EverythingBox(false, 0, 0),

			MainToolbar(),
			NewAction(MainToolbar.Add(Local("New"), GTK_STOCK_CLEAR, this)),
			OpenAction(MainToolbar.Add(Local("Open"), GTK_STOCK_OPEN, this)),
			SaveAction(MainToolbar.Add(Local("Save"), GTK_STOCK_SAVE, this)),
			ExportAction(MainToolbar.Add(Local("Export"), NULL, this)),
			ConfigureAction(MainToolbar.Add(Local("Settings"), GTK_STOCK_PREFERENCES, this)),

			Scroller(gtk_scrolled_window_new(NULL, NULL)),
			Canvas(gtk_drawing_area_new()),

			Sketcher(CreateSketcher(Settings.ImageSize, Settings.DisplayScale)),
			LookingAtSet(false)
		{
			// Construct the window
			gtk_window_set_default_size(GTK_WINDOW(Window), 400, 440);
			gtk_window_set_title(GTK_WINDOW(Window), Local("Inscribist").c_str());
			gtk_container_set_reallocate_redraws(GTK_CONTAINER(Window), true);
			g_signal_connect(G_OBJECT(Window), "configure-event", G_CALLBACK(ResizeViewportCallback), this);
			g_signal_connect(G_OBJECT(Window), "delete-event", G_CALLBACK(DeleteCallback), this);
			g_signal_connect(G_OBJECT(Window), "destroy", G_CALLBACK(DestroyCallback), this);
			gtk_widget_add_events(Window, GDK_KEY_PRESS);
			g_signal_connect(G_OBJECT(Window), "key-press-event", G_CALLBACK(KeyCallback), this);

			// Create the canvas area and canvas scroller
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Scroller), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

			SetBackgroundColor(Canvas, Color(Settings.DisplayPaper * BackgroundColorScale,
				Settings.DisplayPaper.Alpha * BackgroundColorScale + (1.0f - BackgroundColorScale)));
			gtk_widget_set_extension_events(Canvas, GDK_EXTENSION_EVENTS_CURSOR);
			gtk_widget_add_events(Canvas, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
			g_signal_connect(G_OBJECT(Canvas), "configure-event", G_CALLBACK(ResizeCanvasCallback), this);
			g_signal_connect(G_OBJECT(Canvas), "expose-event", G_CALLBACK(DrawCallback), this);
			g_signal_connect(G_OBJECT(Canvas), "button-press-event", G_CALLBACK(ClickCallback), this);
			g_signal_connect(G_OBJECT(Canvas), "button-release-event", G_CALLBACK(DeclickCallback), this);
			g_signal_connect(G_OBJECT(Canvas), "motion-notify-event", G_CALLBACK(MoveCallback), this);

			gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Scroller), Canvas);
			gtk_widget_show(Canvas);

			LookingAt = FlatVector(0.5f, 0.5f);
			LookingAtSet = true;
			SizeCanvasAppropriately();

			// Assemble the window
			EverythingBox.Add(MainToolbar);
			EverythingBox.AddFill(Scroller);
			gtk_container_add(GTK_CONTAINER(Window), EverythingBox);
			gtk_widget_show(EverythingBox);

			gtk_widget_show(Window);

			// Fix up devices
			GList *InputDevices = gdk_devices_list();
			for (GList *DeviceIterator = InputDevices; DeviceIterator != NULL; DeviceIterator = DeviceIterator->next)
			{
				GdkDevice *CurrentDevice = (GdkDevice*)DeviceIterator->data;

				// Forcibly enable all devices.  Fuck choice!
				bool Result = gdk_device_set_mode(CurrentDevice, GDK_MODE_SCREEN);

				int AxisCount = gdk_device_get_n_axes(CurrentDevice);
				for (int CurrentAxis = 0; CurrentAxis < AxisCount; CurrentAxis++)
					GdkAxisUse Use = gdk_device_get_axis_use(CurrentDevice, CurrentAxis);
			}
		}

		void Run(void)
		{
			gtk_main();
		}

		void Act(void *Source)
		{
			if (Source == NewAction)
			{
				bool DecidedTo = !Sketcher->HasChanges() || Confirm(Window, Local("New Image"),
					Local("Are you sure you wish to clear the image?  Any unsaved changes will be lost."));
				if (DecidedTo)
				{
					// Save the focus position for the canvas resize later
					if (!LookingAtSet)
						LookingAt = GetImageFocusPercent();
					LookingAtSet = true;

					// Clear and update settings
					Settings.Filename = NewFilename;

					delete Sketcher;
					Sketcher = CreateSketcher(Settings.ImageSize, Settings.DisplayScale);

					// Resize + refresh the canvas for the new image
					SizeCanvasAppropriately();
				}
			}

			if (Source == OpenAction)
			{
				GtkWidget *Dialog = gtk_file_chooser_dialog_new(Local("Open...").c_str(),
					GTK_WINDOW(Window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);
				gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Dialog), Settings.Filename.c_str());

				GtkFileFilter *SingleFilter = gtk_file_filter_new();
				gtk_file_filter_set_name(SingleFilter, (Local("Inscribist images") + " (*" + Extension + ")").c_str());
				gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(Dialog), SingleFilter);
				gtk_file_filter_add_pattern(SingleFilter, ("*" + Extension).c_str());

				String Out;
				if (gtk_dialog_run(GTK_DIALOG(Dialog)) == GTK_RESPONSE_ACCEPT)
				{
					char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(Dialog));
					Out = PreOut;
					g_free(PreOut);

					/// Load the image
					// Save the focus, even though it doesn't mean much
					if (!LookingAtSet)
						LookingAt = GetImageFocusPercent();
					LookingAtSet = true;

					// Clear and update settings
					Settings.Filename = Out;

					// Load
					delete Sketcher;
					Sketcher = CreateSketcher(Settings.Filename, Settings.ImageSize, Settings.DisplayScale);

					// Resize + refresh the canvas for the new image
					SizeCanvasAppropriately();
				}

				gtk_widget_destroy(Dialog);
			}

			if (Source == SaveAction)
			{
				GtkWidget *Dialog = gtk_file_chooser_dialog_new(Local("Save...").c_str(),
					GTK_WINDOW(Window),
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);
				gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(Dialog), true);
				gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Dialog), Settings.Filename.c_str());

				GtkFileFilter *SingleFilter = gtk_file_filter_new();
				gtk_file_filter_set_name(SingleFilter, (Local("Inscribist images") + " (*" + Extension + ")").c_str());
				gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(Dialog), SingleFilter);
				gtk_file_filter_add_pattern(SingleFilter, ("*" + Extension).c_str());

				String Out;
				if (gtk_dialog_run(GTK_DIALOG(Dialog)) == GTK_RESPONSE_ACCEPT)
				{
					char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(Dialog));
					Out = PreOut;
					g_free(PreOut);

					if (Right(Out, Extension.size()) != Extension)
						Out += Extension;

					/// Save the image
					Sketcher->Save(Out);
				}

				gtk_widget_destroy(Dialog);
			}

			if (Source == ExportAction)
			{
				const String ExportExtension(".png");

				GtkWidget *Dialog = gtk_file_chooser_dialog_new(Local("Export...").c_str(),
					GTK_WINDOW(Window),
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);
				gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(Dialog), true);
				gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Dialog), Settings.Filename.c_str());

				GtkFileFilter *SingleFilter = gtk_file_filter_new();
				gtk_file_filter_set_name(SingleFilter, (Local("PNG images") + " (*" + ExportExtension + ")").c_str());
				gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(Dialog), SingleFilter);
				gtk_file_filter_add_pattern(SingleFilter, ("*" + ExportExtension).c_str());

				String Out;
				if (gtk_dialog_run(GTK_DIALOG(Dialog)) == GTK_RESPONSE_ACCEPT)
				{
					char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(Dialog));
					Out = PreOut;
					g_free(PreOut);

					if (Right(Out, ExportExtension.size()) != ExportExtension)
						Out += ExportExtension;

					/// Export the image
					Sketcher->Export(Out, Settings.ExportInk, Settings.ExportPaper, Settings.ExportScale);
				}

				gtk_widget_destroy(Dialog);
			}

			if (Source == ConfigureAction)
			{
				OpenSettings(Window, Settings);

				SetBackgroundColor(Canvas, Color(Settings.DisplayPaper * BackgroundColorScale,
					Settings.DisplayPaper.Alpha * BackgroundColorScale + (1.0f - BackgroundColorScale)));

				// Refresh after changing settings, since colors might have changed
				if (!GDK_IS_WINDOW(Canvas->window)) return;
				GdkRectangle Region = {0, 0, Canvas->allocation.width, Canvas->allocation.height};
				gdk_window_invalidate_rect(Canvas->window, &Region, false);
				gdk_window_process_updates(Canvas->window, false);
			}
		}
	private:
		/// Interface
		SettingsData Settings;
		GtkWidget *Window;

		Layout EverythingBox;

		Toolbar MainToolbar;
		void *NewAction, *OpenAction, *SaveAction, *ExportAction, *ConfigureAction;

		GtkWidget *Scroller, *Canvas;

		/// State
		DrawingInterface *Sketcher;

		CursorState State, LastState;

		// Offset of corner of image from corner of canvas
		FlatVector ImageOffset;

		// Center of the viewport, used for fixing centering after resizing
		FlatVector LookingAt;
		bool LookingAtSet;
};

//
int main(int ArgumentCount, char **Arguments)
{
	gtk_init(&ArgumentCount, &Arguments);

	/// Create the window
	MainWindow MainWindow(ArgumentCount, Arguments);
	MainWindow.Run();

	return 0;
}
