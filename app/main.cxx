// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string>
#include <iostream>
#include <cassert>
#include <cmath>

#include "ren-general/vector.h"
#include "ren-general/range.h"

#include "gtkwrapper.h"
#include "image.h"
#include "settingsdialog.h"
#include "localization.h"

const int Revision = REVISION;
extern const float SliderFactor, InverseSliderFactor;

class MainWindow : public ActionHandler
{
	public:
		// Auxiliary
		void UpdateState(CursorState &State, const FlatVector &NewPosition, GdkDevice *CurrentDevice)
		{
			if (CurrentDevice != State.LastDevice)
			{
				State.LastDevice = CurrentDevice;
				State.Device = &Settings.GetDeviceSettings(gdk_device_get_name(CurrentDevice));
				State.Brush = &Settings.GetBrushSettings(State.Device->Brush);
				gtk_label_set_text(GTK_LABEL(ToolbarSizeIndicator), AsString((int)State.Brush->HeavyRadius).c_str());
				SetBackgroundColor(ToolbarColorIndicator, State.Brush->Black ? Settings.DisplayInk : Settings.DisplayPaper);
			}

			double *AxisRawData = new double[(int const)gdk_device_get_n_axes(CurrentDevice)];
			gdk_device_get_state(CurrentDevice, Canvas->window, AxisRawData, NULL);

			State.Position = NewPosition - ImageOffset;

			double Found;
			if (gdk_device_get_axis(CurrentDevice, AxisRawData, GDK_AXIS_PRESSURE, &Found) && !isinf(Found) && !isnan(Found) && (Found >= 0))
			{
				if (Found < 0.05f) State.Radius = 0.0f;
				else
				{
					float const RadiusDifference = State.Brush->HeavyRadius - State.Brush->LightRadius;
					State.Radius = State.Brush->LightRadius + RadiusDifference * powf(Found, State.Device->Damping);
				}
			}
			else State.Radius = State.Brush->HeavyRadius;

			delete [] AxisRawData;
		}

		void SizeCanvasAppropriately(void)
		{
			// Invalidate everything first, so that we definitelyish refresh after resizing
			if (GDK_IS_WINDOW(Canvas->window))
			{
				GdkRectangle Region = {0, 0, Canvas->allocation.width, Canvas->allocation.height};
				gdk_window_invalidate_rect(Canvas->window, &Region, false);
			}

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
				if (State.Brush != NULL)
				{
					State.Brush->Black = !State.Brush->Black;

					SetBackgroundColor(ToolbarColorIndicator, State.Brush->Black ? Settings.DisplayInk : Settings.DisplayPaper);
				}
			}
			else if (RangeD(GDK_KEY_0, GDK_KEY_9).Contains(Event->keyval))
			{
				/// Change brush size key pressed
				if (State.Device != NULL)
				{
					State.Device->Brush = Event->keyval - GDK_KEY_0;
					State.Brush = &Settings.GetBrushSettings(State.Device->Brush);

					gtk_label_set_text(GTK_LABEL(ToolbarSizeIndicator), AsString((int)State.Brush->HeavyRadius).c_str());
					SetBackgroundColor(ToolbarColorIndicator, State.Brush->Black ? Settings.DisplayInk : Settings.DisplayPaper);
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
					Sketcher->Zoom(1);
				else Sketcher->Zoom(-1);

				// Resize the window
				SizeCanvasAppropriately();
			}
			else if ((Event->keyval == GDK_KEY_s) && (Event->state & GDK_CONTROL_MASK))
			{
				/// Save combo
				// If using a default file name, show a prompt
				if (SaveFilename == NewFilename)
					Act(SaveAction);
				else // Otherwise, just save
					Sketcher->Save(SaveFilename);
			}
			else if ((Event->keyval == GDK_KEY_S) && (Event->state & GDK_CONTROL_MASK))
				/// Save-as combo
				Act(SaveAction);
			else if ((Event->keyval == GDK_KEY_v) || (Event->keyval == GDK_KEY_h))
			{
				/// Vertical and horizontal flipping
				GdkRectangle Region = {0, 0, Canvas->allocation.width, Canvas->allocation.height};
				gdk_window_invalidate_rect(Canvas->window, &Region, false);

				if (Event->keyval == GDK_KEY_h)
				{
					Sketcher->FlipHorizontally();
					SetAdjustment(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Scroller)),
						1.0f - GetImageFocusPercent()[0],
						Sketcher->GetDisplaySize()[0], ImageOffset[0], Sketcher->GetDisplaySize()[0] + 2.0f * ImageOffset[0]);
				}
				else
				{
					Sketcher->FlipVertically();
					SetAdjustment(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Scroller)),
						1.0f - GetImageFocusPercent()[1],
						Sketcher->GetDisplaySize()[1], ImageOffset[1], Sketcher->GetDisplaySize()[1] + 2.0f * ImageOffset[1]);
				}

				gdk_window_process_updates(Canvas->window, false); // Unnecessary as long as adjustment updates do it
			}
			else if (((Event->keyval == GDK_KEY_Z) || (Event->keyval == GDK_KEY_y) ||
				(Event->keyval == GDK_KEY_z)) && (Event->state & GDK_CONTROL_MASK))
			{
				/// Undo, redo
				// May have to move the screen if it un-flips

				GdkRectangle Region = {0, 0, Canvas->allocation.width, Canvas->allocation.height};
				gdk_window_invalidate_rect(Canvas->window, &Region, false);

				bool FlippedHorizontally, FlippedVertically;
				if ((Event->keyval == GDK_KEY_Z) || (Event->keyval == GDK_KEY_y))
					Sketcher->Redo(FlippedHorizontally, FlippedVertically);
				else Sketcher->Undo(FlippedHorizontally, FlippedVertically);

				if (FlippedHorizontally)
					SetAdjustment(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Scroller)),
						1.0f - GetImageFocusPercent()[0],
						Sketcher->GetDisplaySize()[0], ImageOffset[0], Sketcher->GetDisplaySize()[0] + 2.0f * ImageOffset[0]);

				if (FlippedVertically)
					SetAdjustment(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Scroller)),
						1.0f - GetImageFocusPercent()[1],
						Sketcher->GetDisplaySize()[1], ImageOffset[1], Sketcher->GetDisplaySize()[1] + 2.0f * ImageOffset[1]);

				gdk_window_process_updates(Canvas->window, false);
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
			Region const RenderRegion(
				FlatVector((int)(Event->area.x - ImageOffset[0]), (int)(Event->area.y - ImageOffset[1])),
				FlatVector((int)Event->area.width + 1, (int)Event->area.height + 1));
			Sketcher->Render(RenderRegion, CairoContext);

			cairo_destroy(CairoContext);
		}

		void Click(GdkEventButton *Event)
		{
			if (Event->type != GDK_BUTTON_PRESS) return; // Ignore double, triple-click extra events

			LastState = State;
			UpdateState(State, FlatVector(Event->x, Event->y), Event->device);

			if (Event->button == 2) State.Mode = CursorState::mPanning;
			else if ((Event->button == 0) || (Event->button = 1))
			{
				LastState = State;
				State.Mode = CursorState::mMarking;

				const Region Marked = Sketcher->Mark(LastState, State, State.Brush->Black);

				// Refresh the marked area
				const GdkRectangle Region =
				{
					static_cast<gint>(Marked.Start[0] + ImageOffset[0] - 2),
					static_cast<gint>(Marked.Start[1] + ImageOffset[1] - 2),
					static_cast<gint>(Marked.Size[0] + 4),
					static_cast<gint>(Marked.Size[1] + 4)
				};

				gdk_window_invalidate_rect(Canvas->window, &Region, false);
			}
		}

		void Declick(GdkEventButton *Event)
		{
			LastState = State;
			UpdateState(State, FlatVector(Event->x, Event->y), Event->device);

			if (State.Mode == CursorState::mMarking)
				Sketcher->FinishMark();
			State.Mode = CursorState::mFree;
		}

		void Move(GdkEventMotion *Event)
		{
			LastState = State;
			UpdateState(State, FlatVector(Event->x, Event->y), Event->device);

			if ((State.Mode == CursorState::mMarking) && (State.Radius > 0.0f))
			{
				// Mark
				const Region Marked = Sketcher->Mark(LastState, State, State.Brush->Black);

				// Refresh the marked area
				const GdkRectangle Region =
				{
					static_cast<gint>(Marked.Start[0] + ImageOffset[0] - 2),
					static_cast<gint>(Marked.Start[1] + ImageOffset[1] - 2),
					static_cast<gint>(Marked.Size[0] + 4),
					static_cast<gint>(Marked.Size[1] + 4)
				};

				gdk_window_invalidate_rect(Canvas->window, &Region, false);
			}
			else if (State.Mode == CursorState::mPanning)
			{
				/// Scroll the scroller
				// Or, at least, set to scroll next idle cycle
				PanOffset += -(State.Position - LastState.Position);
				if (!PanOffsetSet)
				{
					PanOffsetSet = true;
					g_idle_add((gboolean (*)(void*))&IdlePanCallback, this);
				}
			}
		}

		void ResizeImage(GdkEventConfigure *Event)
		{
			// Position the canvas appropriately in the window
			const FlatVector NewArea(Event->width, Event->height),
				CanvasSize(Sketcher->GetDisplaySize());
			ImageOffset = NewArea * 0.5f - CanvasSize * 0.5f;

			// Scroll so that we're looking at the same thing we used to be looking at
			if (LookingAtSet)
			{
				GtkAdjustment *VAdjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Scroller)),
					*HAdjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Scroller));

				SetAdjustment(HAdjustment, LookingAt[0], CanvasSize[0], ImageOffset[0], NewArea[0]);
				SetAdjustment(VAdjustment, LookingAt[1], CanvasSize[1], ImageOffset[1], NewArea[1]);

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

		bool PanUpdate(void)
		{
			// This is basically a nest of hacks, happily all in one function!
			assert(PanOffsetSet);

			// NOTE manually clips value becase gtk is broken
			// NOTE If panning x and y, suppresses one of the value-change signals because multi-adjustment updates are broken (also gtk)
			GtkAdjustment *Adjustment;

			bool SuppressOne = ((int)PanOffset[0] != 0) && ((int)PanOffset[1] != 0);

			Adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Scroller));
			if (SuppressOne) gtk_signal_handler_block(Adjustment, ViewportUpdateSignalHandler);
			gtk_adjustment_set_value(Adjustment,
				RangeF(0, gtk_adjustment_get_upper(Adjustment) - gtk_adjustment_get_page_size(Adjustment)).Constrain(
					gtk_adjustment_get_value(Adjustment) + PanOffset[0]));
			if (SuppressOne) gtk_signal_handler_unblock(Adjustment, ViewportUpdateSignalHandler);

			Adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Scroller));
			gtk_adjustment_set_value(Adjustment,
				RangeF(0, gtk_adjustment_get_upper(Adjustment) - gtk_adjustment_get_page_size(Adjustment)).Constrain(
					gtk_adjustment_get_value(Adjustment) + PanOffset[1]));

			State.Position += PanOffset;
			PanOffset = FlatVector();
			PanOffsetSet = false;

			return false;
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

		static bool IdlePanCallback(MainWindow *This)
			{ return This->PanUpdate(); }

		static gboolean DeleteCallback(GtkWidget *Source, GdkEvent *Event, MainWindow *This)
			{ return !This->ConfirmClose(); }

		static void DestroyCallback(GtkWidget *Source, MainWindow *This)
			{ This->Close(); }

		// Constructor, the meat of our salad
		MainWindow(SettingsData &Settings, const String &Filename) :
			Settings(Settings), SaveFilename(Filename),
			Window(gtk_window_new(GTK_WINDOW_TOPLEVEL)),

			EverythingBox(false, 0, 0),

			ToolbarBox(true, 0, 0),
			MainToolbar(),
			NewAction(MainToolbar.Add(Local("New"), GTK_STOCK_CLEAR, this)),
			OpenAction(MainToolbar.Add(Local("Open"), GTK_STOCK_OPEN, this)),
			SaveAction(MainToolbar.Add(Local("Save"), GTK_STOCK_SAVE, this)),
			ConfigureAction(MainToolbar.Add(Local("Settings"), GTK_STOCK_PREFERENCES, this)),
			ToolbarIndicatorToolbar(gtk_toolbar_new()),
			ToolbarSizeIndicator(gtk_label_new(NULL)),
			ToolbarColorIndicator(gtk_drawing_area_new()),

			Scroller(gtk_scrolled_window_new(NULL, NULL)),
			Canvas(gtk_drawing_area_new()),

			Sketcher(new Image(Settings, SaveFilename)),
			LookingAtSet(false),

			PanOffsetSet(false), ViewportUpdateSignalHandler(0)
		{
			// Construct the window
			gtk_window_set_default_size(GTK_WINDOW(Window), 400, 440);
			gtk_window_set_icon_from_file(GTK_WINDOW(Window), (DataLocation.Select("/icon32.png")).AsAbsoluteString().c_str(), NULL);
			gtk_window_set_title(GTK_WINDOW(Window), Local("Inscribist").c_str());
			gtk_container_set_reallocate_redraws(GTK_CONTAINER(Window), true);
			g_signal_connect(G_OBJECT(Window), "configure-event", G_CALLBACK(ResizeViewportCallback), this);
			g_signal_connect(G_OBJECT(Window), "delete-event", G_CALLBACK(DeleteCallback), this);
			g_signal_connect(G_OBJECT(Window), "destroy", G_CALLBACK(DestroyCallback), this);
			gtk_widget_add_events(Window, GDK_KEY_PRESS);
			g_signal_connect(G_OBJECT(Window), "key-press-event", G_CALLBACK(KeyCallback), this);

			// Set up toolbar area
			ToolbarBox.AddFill(MainToolbar);

			gtk_toolbar_set_show_arrow(GTK_TOOLBAR(ToolbarIndicatorToolbar), false);

			GtkToolItem *ToolbarSizeIndicatorItem = gtk_tool_item_new();
			gtk_container_set_border_width(GTK_CONTAINER(ToolbarSizeIndicatorItem), 4);
			gtk_container_add(GTK_CONTAINER(ToolbarSizeIndicatorItem), ToolbarSizeIndicator);
			gtk_widget_show(ToolbarSizeIndicator);
			gtk_toolbar_insert(GTK_TOOLBAR(ToolbarIndicatorToolbar), ToolbarSizeIndicatorItem, -1);
			gtk_widget_show(GTK_WIDGET(ToolbarSizeIndicatorItem));

			int IconWidth, IconHeight;
			gtk_icon_size_lookup(gtk_toolbar_get_icon_size(GTK_TOOLBAR((GtkWidget*)MainToolbar)), &IconWidth, &IconHeight);
			gtk_widget_set_size_request(ToolbarColorIndicator, IconHeight + 4, IconHeight);
			GtkToolItem *ToolbarColorIndicatorItem = gtk_tool_item_new();
			gtk_container_set_border_width(GTK_CONTAINER(ToolbarColorIndicatorItem), 4);
			gtk_container_add(GTK_CONTAINER(ToolbarColorIndicatorItem), ToolbarColorIndicator);
			gtk_widget_show(ToolbarColorIndicator);
			gtk_toolbar_insert(GTK_TOOLBAR(ToolbarIndicatorToolbar), ToolbarColorIndicatorItem, -1);
			gtk_widget_show(GTK_WIDGET(ToolbarColorIndicatorItem));

			ToolbarBox.Add(ToolbarIndicatorToolbar);

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

			{
				// Save the value for the update suppression hack for panning
				GtkAdjustment *Adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Scroller));
				ViewportUpdateSignalHandler = g_signal_handler_find(Adjustment, G_SIGNAL_MATCH_DATA,
					g_signal_lookup("value-changed", G_TYPE_FROM_INSTANCE(Adjustment)),
					0, NULL, NULL, GTK_VIEWPORT(gtk_bin_get_child(GTK_BIN(Scroller))));
			}

			LookingAt = FlatVector(0.5f, 0.5f);
			LookingAtSet = true;
			SizeCanvasAppropriately();

			// Assemble the window
			EverythingBox.Add(ToolbarBox);
			EverythingBox.AddFill(Scroller);
			gtk_container_add(GTK_CONTAINER(Window), EverythingBox);
			gtk_widget_show(EverythingBox);

			gtk_widget_show(Window);

			// Forcibly enable all devices.  Forcible because we don't ask the user.
			GList *InputDevices = gdk_devices_list();
			for (GList *DeviceIterator = InputDevices; DeviceIterator != NULL; DeviceIterator = DeviceIterator->next)
			{
				GdkDevice *CurrentDevice = (GdkDevice*)DeviceIterator->data;

				gdk_device_set_mode(CurrentDevice, GDK_MODE_SCREEN);
			}
		}

		~MainWindow(void)
		{
			delete Sketcher;
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
					SaveFilename = NewFilename;

					delete Sketcher;
					Sketcher = new Image(Settings);

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
				if (!SaveFilename.empty())
					gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Dialog), SaveFilename.c_str());

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
					SaveFilename = Out;

					// Load
					delete Sketcher;
					Sketcher = new Image(Settings, SaveFilename);

					// Resize + refresh the canvas for the new image
					SetBackgroundColor(Canvas, Color(Settings.DisplayPaper * BackgroundColorScale,
						Settings.DisplayPaper.Alpha * BackgroundColorScale + (1.0f - BackgroundColorScale)));
					SetBackgroundColor(ToolbarColorIndicator, State.Brush->Black ? Settings.DisplayInk : Settings.DisplayPaper);
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
				gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Dialog), SaveFilename.c_str());

				GtkFileFilter *NativeFilter = gtk_file_filter_new();
				gtk_file_filter_set_name(NativeFilter, (Local("Inscribist images") + " (*" + Extension + ")").c_str());
				gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(Dialog), NativeFilter);
				gtk_file_filter_add_pattern(NativeFilter, ("*" + Extension).c_str());

				String ExportExtension(".png");
				GtkFileFilter *ExportFilter = gtk_file_filter_new();
				gtk_file_filter_set_name(ExportFilter, (Local("PNG image (export)") + " (*" + ExportExtension + ")").c_str());
				gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(Dialog), ExportFilter);
				gtk_file_filter_add_pattern(ExportFilter, ("*" + ExportExtension).c_str());

				if (gtk_dialog_run(GTK_DIALOG(Dialog)) == GTK_RESPONSE_ACCEPT)
				{
					if (gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(Dialog)) == NativeFilter)
					{
						char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(Dialog));
						SaveFilename = PreOut;
						g_free(PreOut);

						if ((SaveFilename.length() < Extension.size()) ||  (SaveFilename.substr(SaveFilename.length() - Extension.size(), String::npos) != Extension))
							SaveFilename += Extension;

						/// Save the image
						Sketcher->Save(SaveFilename);
					}
					else
					{
						char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(Dialog));
						String Out = PreOut;
						g_free(PreOut);

						if ((Out.length() < ExportExtension.size()) ||  (Out.substr(Out.length() - ExportExtension.size(), String::npos) != ExportExtension))
							Out += ExportExtension;

						/// Export the image
						Sketcher->Export(Out);
					}
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

				// Refresh setting stats in corner
				if (State.Brush != NULL)
				{
					gtk_label_set_text(GTK_LABEL(ToolbarSizeIndicator), AsString((int)State.Brush->HeavyRadius).c_str());
					SetBackgroundColor(ToolbarColorIndicator, State.Brush->Black ? Settings.DisplayInk : Settings.DisplayPaper);
				}
			}
		}
	private:
		SettingsData &Settings;
		String SaveFilename;

		/// Interface
		GtkWidget *Window;

		Layout EverythingBox;

		Layout ToolbarBox;
		Toolbar MainToolbar;
		void *NewAction, *OpenAction, *SaveAction, *ConfigureAction;
		GtkWidget *ToolbarIndicatorToolbar, *ToolbarSizeIndicator, *ToolbarColorIndicator;

		GtkWidget *Scroller, *Canvas;

		/// State
		Image *Sketcher;

		CursorState State, LastState;

		// Offset of corner of image from corner of canvas
		FlatVector ImageOffset;

		// Center of the viewport, used for fixing centering after resizing
		FlatVector LookingAt;
		bool LookingAtSet;

		// For panning updates when the app's idle
		FlatVector PanOffset;
		bool PanOffsetSet;
		gulong ViewportUpdateSignalHandler;
};

//
int main(int ArgumentCount, char **Arguments)
{
	gtk_init(&ArgumentCount, &Arguments);

	/// Load the settings
	SettingsData Settings;

	/// Parse the arguments to override settings and stuff
	String Filename = NewFilename;
	if (ArgumentCount >= 2)
	{
		Filename = Arguments[1];

		if ((Filename == "-v") || (Filename == "--version"))
		{
			std::cout << "Inscribist, revision " << Revision << std::endl;
#ifdef SETTINGSLOCATION
			std::cout << "Compile-configured settings filename: " << SETTINGSLOCATION << std::endl;
#endif
#ifdef DATALOCATION
			std::cout << "Compile-configured data location: " << DATALOCATION << std::endl;
#endif
#ifndef NDEBUG
			std::cout << "Assertions are enabled." << std::endl;
#endif

			return 0;
		}

		std::cout << "Opening " << Filename << std::endl;
	}

	if (ArgumentCount > 4)
	{
		MemoryStream(Arguments[2]) >> Settings.ImageSize[0];
		MemoryStream(Arguments[3]) >> Settings.ImageSize[1];
		std::cout << "If creating a new file, use size " << Settings.ImageSize.AsString() << std::endl;
	}

	/// Create the window
	MainWindow MainWindow(Settings, Filename);
	MainWindow.Run();

	return 0;
}
