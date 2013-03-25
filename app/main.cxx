// Copyright 2013 Rendaw, under the FreeBSD license (See included license.txt)

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <string>
#include <iostream>
#include <cassert>
#include <cmath>
#include <tuple>

#include "ren-general/vector.h"
#include "ren-general/range.h"
#include "ren-gtk/gtkwrapper.h"
#include "ren-translation/translation.h"

#include "image.h"
#include "settingsdialog.h"
#include "expanddialog.h"

const int Revision = REVISION;

const float ColorMax = 65535;

String const NewFilename("fresh" + Extension);

void SetBackgroundColor(GtkWidget *Widget, const Color &NewColor)
{
	GdkColor SetColor;
	SetColor.red = ColorMax * NewColor.Red;
	SetColor.green = ColorMax * NewColor.Green;
	SetColor.blue = ColorMax * NewColor.Blue;
	gtk_widget_modify_bg(Widget, GTK_STATE_NORMAL, &SetColor);
}

void SetAdjustment(GtkAdjustment *Adjustment, float CanvasPercent, float CanvasSize, float Padding, float ScreenSize)
{
	gtk_adjustment_set_value(Adjustment,
		(CanvasPercent * CanvasSize + Padding) / ScreenSize *
			(gtk_adjustment_get_upper(Adjustment) - gtk_adjustment_get_lower(Adjustment))  +
			(gtk_adjustment_get_lower(Adjustment) - gtk_adjustment_get_page_size(Adjustment)) * 0.5f);
}

class MainWindow : public Window
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
				UpdateBrushSizeIndicator();
				SetBackgroundColor(ToolbarColorIndicator, State.Brush->Black ? Settings.DisplayInk : Settings.DisplayPaper);
			}

			double *AxisRawData = new double[(int const)gdk_device_get_n_axes(CurrentDevice)];
			gdk_device_get_state(CurrentDevice, Canvas->window, AxisRawData, NULL);

			State.Position = NewPosition - ImageOffset;

			unsigned int ShortestAxisSize = std::min(Sketcher->GetSize()[0], Sketcher->GetSize()[1]);
			double Found;
			if (gdk_device_get_axis(CurrentDevice, AxisRawData, GDK_AXIS_PRESSURE, &Found) && !std::isinf(Found) && !std::isnan(Found) && (Found >= 0))
			{
				if (Found < 0.05f) State.Radius = 0.0f;
				else
				{
					float const DiameterDifference = State.Brush->HeavyDiameter - State.Brush->LightDiameter;
					State.Radius = ShortestAxisSize *
						0.005 * (State.Brush->LightDiameter + DiameterDifference * powf(Found, State.Device->Damping));
				}
			}
			else State.Radius = ShortestAxisSize * 0.005 * State.Brush->HeavyDiameter;

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
		void ToggleBrushColor(void)
		{
			if (State.Brush != NULL)
			{
				State.Brush->Black = !State.Brush->Black;

				SetBackgroundColor(ToolbarColorIndicator, State.Brush->Black ? Settings.DisplayInk : Settings.DisplayPaper);
			}
		}

		void SelectBrush(unsigned int Index)
		{
			if (State.Device != NULL)
			{
				State.Device->Brush = Index;
				State.Brush = &Settings.GetBrushSettings(State.Device->Brush);

				UpdateBrushSizeIndicator();
				SetBackgroundColor(ToolbarColorIndicator, State.Brush->Black ? Settings.DisplayInk : Settings.DisplayPaper);
			}
		}

		void Zoom(int Change)
		{
			/// Zoom key pressed
			// Save the focus
			if (!LookingAtSet)
				LookingAt = GetImageFocusPercent();
			LookingAtSet = true;

			// Do the zoom
			Sketcher->Zoom(Change);

			// Resize the window
			SizeCanvasAppropriately();
		}

		void Flip(bool Horizontal)
		{
			GdkRectangle Region = {0, 0, Canvas->allocation.width, Canvas->allocation.height};
			gdk_window_invalidate_rect(Canvas->window, &Region, false);

			if (Horizontal)
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

		void Roll(bool Careful, int Horizontal, int Vertical)
		{
			GdkRectangle Region = {0, 0, Canvas->allocation.width, Canvas->allocation.height};
			gdk_window_invalidate_rect(Canvas->window, &Region, false);
			Sketcher->Shift(!Careful, Horizontal, Vertical);
			gdk_window_process_updates(Canvas->window, false);
		}

		void UndoRedo(bool Undo)
		{
			GdkRectangle Region = {0, 0, Canvas->allocation.width, Canvas->allocation.height};
			gdk_window_invalidate_rect(Canvas->window, &Region, false);

			bool FlippedHorizontally, FlippedVertically;
			if (Undo) Sketcher->Undo(FlippedHorizontally, FlippedVertically);
			else Sketcher->Redo(FlippedHorizontally, FlippedVertically);

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

		void Draw(GdkEventExpose *Event)
		{
			if (FirstDraw)
			{
				gtk_widget_grab_focus(Canvas);
				FirstDraw = false;
			}
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

		void ResizeImageViewport(void)
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
			return !Sketcher->HasChanges() || GTK::Confirm(*this, Local("Quit Inscribist"),
				Local("Are you sure you wish to close Inscribist?  Any unsaved changes will be lost."));
		}

		// Static callback farm
		static gboolean DrawCallback(GtkWidget *, GdkEventExpose *Event, MainWindow *This)
			{ This->Draw(Event); return TRUE; }

		static gboolean ClickCallback(GtkWidget *, GdkEventButton *Event, MainWindow *This)
			{ This->Click(Event); return FALSE; }

		static gboolean DeclickCallback(GtkWidget *, GdkEventButton *Event, MainWindow *This)
			{ This->Declick(Event); return FALSE; }

		static gboolean MoveCallback(GtkWidget *, GdkEventMotion *Event, MainWindow *This)
			{ This->Move(Event); return FALSE; }

		static gboolean ResizeCanvasCallback(GtkWidget *, GdkEventConfigure *Event, MainWindow *This)
			{ This->ResizeImage(Event); return FALSE; }

		static bool IdlePanCallback(MainWindow *This)
			{ return This->PanUpdate(); }

		// Constructor, the meat of our salad
		MainWindow(SettingsData &Settings, const String &Filename) :
			Window(Local("Inscribist"), 0),
			WindowKeys(*this),
			Settings(Settings), SaveFilename(Filename),

			EverythingBox(false, 0, 0),

			ToolbarBox(true, 0, 0),
			MainToolbar(),
			NewButton(Local("New"), diClear),
			OpenButton(Local("Open"), diOpen),
			SaveButton(Local("Save"), diSave),
			ExpandButton(Local("Expand"), diAdd),
			ConfigureButton(Local("Settings"), diConfigure),
			ToolbarIndicatorToolbar(gtk_toolbar_new()),
			ToolbarColorIndicator(gtk_drawing_area_new()),
			ToolbarSizeIndicator(""),

			Scroller(gtk_scrolled_window_new(NULL, NULL)),
			Canvas(gtk_drawing_area_new()),

			Sketcher(new Image(Settings, SaveFilename)),
			FirstDraw(true),
			LookingAtSet(false),

			PanOffsetSet(false), ViewportUpdateSignalHandler(0)
		{
			// Setup key callbacks
			for (auto const &Key : std::list<unsigned int>{
					GDK_KEY_Tab, 
					GDK_KEY_space, 
					GDK_KEY_KP_Enter,
					GDK_KEY_Return})
				KeyCallbacks[std::make_tuple(Key, false)] = [this]() { ToggleBrushColor(); };
			KeyCallbacks[std::make_tuple(GDK_KEY_plus, false)] = [this]() { ToggleBrushColor(); };
			KeyCallbacks[std::make_tuple(GDK_KEY_space, false)] = [this]() { ToggleBrushColor(); };
			for (unsigned int Index = 0; Index < 10; ++Index)
			{
				KeyCallbacks[std::make_tuple(GDK_KEY_0 + Index, false)] = [this, Index]() { SelectBrush(Index); };
				KeyCallbacks[std::make_tuple(GDK_KEY_KP_0 + Index, false)] = [this, Index]() { SelectBrush(Index); };
			}
			KeyCallbacks[std::make_tuple(GDK_bracketleft, false)] = [this]() { Zoom(-1); };
			KeyCallbacks[std::make_tuple(GDK_KEY_KP_Add, false)] = [this]() { Zoom(-1); };
			KeyCallbacks[std::make_tuple(GDK_bracketright, false)] = [this]() { Zoom(1); };
			KeyCallbacks[std::make_tuple(GDK_KEY_KP_Subtract, false)] = [this]() { Zoom(1); };
			KeyCallbacks[std::make_tuple(GDK_KEY_s, true)] = [this]() 
			{ 
				if (SaveFilename.empty()) SaveAs();
				else Sketcher->Save(SaveFilename);
			};
			KeyCallbacks[std::make_tuple(GDK_KEY_S, true)] = [this]() { SaveAs(); };
			for (auto const &Key : std::list<unsigned int>{GDK_KEY_v, GDK_KEY_KP_Divide})
				KeyCallbacks[std::make_tuple(Key, false)] = [this]() { Flip(false); };
			for (auto const &Key : std::list<unsigned int>{GDK_KEY_h, GDK_KEY_KP_Multiply})
				KeyCallbacks[std::make_tuple(Key, false)] = [this]() { Flip(true); };
			for (auto const &Key : std::list<unsigned int>{GDK_KEY_Left, GDK_KEY_KP_Left})
				for (auto const &Careful : std::list<unsigned int>{false, true})
					KeyCallbacks[std::make_tuple(Key, Careful)] = [this, Careful]() { Roll(Careful, -1, 0); };
			for (auto const &Key : std::list<unsigned int>{GDK_KEY_Right, GDK_KEY_KP_Right})
				for (auto const &Careful : std::list<unsigned int>{false, true})
					KeyCallbacks[std::make_tuple(Key, Careful)] = [this, Careful]() { Roll(Careful, 1, 0); };
			for (auto const &Key : std::list<unsigned int>{GDK_KEY_Up, GDK_KEY_KP_Up})
				for (auto const &Careful : std::list<unsigned int>{false, true})
					KeyCallbacks[std::make_tuple(Key, Careful)] = [this, Careful]() { Roll(Careful, 0, -1); };
			for (auto const &Key : std::list<unsigned int>{GDK_KEY_Down, GDK_KEY_KP_Down})
				for (auto const &Careful : std::list<unsigned int>{false, true})
					KeyCallbacks[std::make_tuple(Key, Careful)] = [this, Careful]() { Roll(Careful, 0, 1); };
			KeyCallbacks[std::make_tuple(GDK_KEY_z, true)] = [this]() { UndoRedo(true); };
			for (auto const &Key : std::list<unsigned int>{GDK_KEY_Z, GDK_KEY_y})
				KeyCallbacks[std::make_tuple(Key, true)] = [this]() { UndoRedo(false); };

			// Construct the window
			SetDefaultSize({400, 440});
			SetIcon(LocateDataDirectory().Select("icon32.png"));
			SetAttemptCloseHandler([&](void) -> bool
			{
				if (!ConfirmClose()) return false;
				return true;
			});
			SetCloseHandler([](void)
			{
				gtk_main_quit();
			});
			SetResizeHandler([&]() { ResizeImageViewport(); });
			WindowKeys.SetHandler(
				[&](unsigned int KeyCode, unsigned int Modifier)
				{ 
					auto Found = KeyCallbacks.find(std::make_tuple(KeyCode, Modifier & GDK_CONTROL_MASK));
					if (Found == KeyCallbacks.end()) return false;
					Found->second();
					return true;
				});

			// Set up toolbar area
			NewButton.SetAction([&]() { New(); });
			MainToolbar.Add(NewButton);
			OpenButton.SetAction([&]() { Open(); });
			MainToolbar.Add(OpenButton);
			SaveButton.SetAction([&]() { SaveAs(); });
			MainToolbar.Add(SaveButton);
			ExpandButton.SetAction([&]() { Expand(); });
			MainToolbar.Add(ExpandButton);
			ConfigureButton.SetAction([&]() { Configure(); });
			MainToolbar.Add(ConfigureButton);

			ToolbarBox.AddFill(MainToolbar);
			gtk_toolbar_set_style(GTK_TOOLBAR((GtkWidget *)MainToolbar), GTK_TOOLBAR_BOTH_HORIZ);

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
			g_signal_connect(Canvas, "configure-event", G_CALLBACK(ResizeCanvasCallback), this);
			g_signal_connect(Canvas, "expose-event", G_CALLBACK(DrawCallback), this);
			g_signal_connect(Canvas, "button-press-event", G_CALLBACK(ClickCallback), this);
			g_signal_connect(Canvas, "button-release-event", G_CALLBACK(DeclickCallback), this);
			g_signal_connect(Canvas, "motion-notify-event", G_CALLBACK(MoveCallback), this);
			gtk_widget_set_can_focus(Canvas, true);
			gtk_widget_set_can_default(Canvas, true);

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
			Set(EverythingBox);

			Show();

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

		void UpdateBrushSizeIndicator(void)
		{
			MemoryStream Text;
			Text << OutputStream::Float(State.Brush->HeavyDiameter).MaxFractionalDigits(2);
			ToolbarSizeIndicator.SetText(Text);
		}

		void New(void)
		{
			bool DecidedTo = !Sketcher->HasChanges() || GTK::Confirm(*this, Local("New Image"),
				Local("Are you sure you wish to clear the image?  Any unsaved changes will be lost."));
			if (DecidedTo)
			{
				// Save the focus position for the canvas resize later
				if (!LookingAtSet)
					LookingAt = GetImageFocusPercent();
				LookingAtSet = true;

				// Clear and update settings
				SaveFilename = String();

				delete Sketcher;
				Sketcher = new Image(Settings);

				// Resize + refresh the canvas for the new image
				SizeCanvasAppropriately();
			}
		}

		void Open(void)
		{
			FileDialog OpenDialog(Local("Open..."), Local("Inscribist images") + " (*" + Extension + ")", this, false);
			if (!SaveFilename.empty()) OpenDialog.SetFile(SaveFilename);
			else if (!Settings.DefaultDirectory.empty()) OpenDialog.SetDirectory(DirectoryPath::Qualify(Settings.DefaultDirectory));
			OpenDialog.AddFilterPass("*" + Extension);
			OpenDialog.SetDefaultSuffix(Extension);

			String Out = OpenDialog.Run();
			if (!Out.empty())
			{
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
		}

		void SaveAs(void)
		{

			GtkWidget *Dialog = gtk_file_chooser_dialog_new(Local("Save...").c_str(),
				GTK_WINDOW((GtkWidget *)*this),
				GTK_FILE_CHOOSER_ACTION_SAVE,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				NULL);
			gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(Dialog), true);

			String Filename = SaveFilename;
			if (Filename.empty())
			{
				if (Settings.DefaultDirectory.empty())
					Filename = NewFilename;
				else Filename = DirectoryPath::Qualify(Settings.DefaultDirectory).Select(NewFilename);
			}
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Dialog), Filename.c_str());

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

		void Expand(void)
		{
			OpenExpandDialog(*this, *Sketcher);

			// Resize the window
			SizeCanvasAppropriately();
		}

		void Configure(void)
		{
			OpenSettings(*this, Settings, Sketcher->GetSize());

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
				UpdateBrushSizeIndicator();
				SetBackgroundColor(ToolbarColorIndicator, State.Brush->Black ? Settings.DisplayInk : Settings.DisplayPaper);
			}
		}

	private:
		KeyboardWidget WindowKeys;
		std::map<std::tuple<unsigned int, bool>, std::function<void(void)>> KeyCallbacks;
		SettingsData &Settings;
		String SaveFilename;

		/// Interface
		Layout EverythingBox;

		Layout ToolbarBox;
		Toolbar MainToolbar;
		ToolButton NewButton, OpenButton, SaveButton, ExpandButton, ConfigureButton;
		GtkWidget *ToolbarIndicatorToolbar, *ToolbarColorIndicator;
		Label ToolbarSizeIndicator;

		GtkWidget *Scroller, *Canvas;

		/// State
		Image *Sketcher;

		CursorState State, LastState;

		// Used for keeping the focus off the toolbar
		bool FirstDraw;

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
	InitializeTranslation("inscribist");
	gtk_init(&ArgumentCount, &Arguments);

	/// Load the settings
	SettingsData Settings;

	/// Parse the arguments to override settings and stuff
	String Filename;
	if (ArgumentCount >= 2)
	{
		Filename = Arguments[1];

		if ((Filename == "-v") || (Filename == "--version"))
		{
			std::cout << "Inscribist, revision " << Revision << std::endl;
#ifndef NDEBUG
			std::cout << "Assertions are enabled." << std::endl;
#endif

			return 0;
		}

		StandardStream << "Opening " << Filename << "\n" << OutputStream::Flush();
	}

	if (ArgumentCount > 4)
	{
		MemoryStream(Arguments[2]) >> Settings.ImageSize[0];
		MemoryStream(Arguments[3]) >> Settings.ImageSize[1];
		StandardStream << "If creating a new file, use size " << Settings.ImageSize.AsString() << "\n" << OutputStream::Flush();
	}

	/// Create the window
	::MainWindow MainWindow(Settings, Filename);
	gtk_main();

	return 0;
}
