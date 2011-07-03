// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#include "gtkwrapper.h"

#include <cmath>
#include <iomanip>

const float ColorMax = 65535;

bool Confirm(GtkWidget *Window, const String &Title, const String &Message)
{
	GtkWidget *Dialog = gtk_message_dialog_new(
		GTK_WINDOW(Window),
		GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
		Title.c_str());
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Dialog), Message.c_str());
	int Result = gtk_dialog_run(GTK_DIALOG(Dialog));
	gtk_widget_destroy(Dialog);

	return Result == GTK_RESPONSE_YES;
}

void SetBackgroundColor(GtkWidget *Widget, const Color &NewColor)
{
	GdkColor SetColor;
	SetColor.red = ColorMax * NewColor.Red;
	SetColor.green = ColorMax * NewColor.Green;
	SetColor.blue = ColorMax * NewColor.Blue;
	gtk_widget_modify_bg(Widget, GTK_STATE_NORMAL, &SetColor);
}

String GetHomeLocation(void)
{
	return g_get_home_dir();
}

void SetAdjustment(GtkAdjustment *Adjustment, float CanvasPercent, float CanvasSize, float Padding, float ScreenSize)
{
	gtk_adjustment_set_value(Adjustment,
		(CanvasPercent * CanvasSize + Padding) / ScreenSize *
			(gtk_adjustment_get_upper(Adjustment) - gtk_adjustment_get_lower(Adjustment))  +
			(gtk_adjustment_get_lower(Adjustment) - gtk_adjustment_get_page_size(Adjustment)) * 0.5f);
}

Layout::Layout(bool Horizontal, int EdgePadding, int ItemPadding) :
	Data(Horizontal ? gtk_hbox_new(false, ItemPadding) : gtk_vbox_new(false, ItemPadding))
	{ gtk_container_set_border_width(GTK_CONTAINER(Data), EdgePadding); }

Layout::operator GtkWidget*(void)
	{ return Data; }

void Layout::Add(GtkWidget *Widget)
{
	gtk_box_pack_start(GTK_BOX(Data), Widget, false, true, 0);
	gtk_widget_show(Widget);
}

void Layout::AddFill(GtkWidget *Widget)
{
	gtk_box_pack_start(GTK_BOX(Data), Widget, true, true, 0);
	gtk_widget_show(Widget);
}

ActionHandler::~ActionHandler(void) {}

Toolbar::Toolbar(void) : Data(gtk_toolbar_new())
{
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(Data), true);
	gtk_toolbar_set_style(GTK_TOOLBAR(Data), GTK_TOOLBAR_BOTH_HORIZ);
}

Toolbar::operator GtkWidget*(void)
	{ return Data; }

void *Toolbar::Add(const String &Label, const char *Icon, ActionHandler *Handler)
{
	GtkToolItem *Out;
	if (Icon != NULL)
	{
		Out = gtk_tool_button_new_from_stock(Icon);
		if (!Label.empty()) gtk_tool_button_set_label(GTK_TOOL_BUTTON(Out), Label.c_str());
	}
	else Out = gtk_tool_button_new(NULL, Label.c_str());

	if (Handler != NULL) g_signal_connect(G_OBJECT(Out), "clicked", G_CALLBACK(HandlePress), Handler);
	gtk_tool_item_set_is_important(Out, true);

	gtk_toolbar_insert(GTK_TOOLBAR(Data), Out, -1);
	gtk_widget_show(GTK_WIDGET(Out));
	return Out;
}

void Toolbar::HandlePress(GtkWidget *Button, ActionHandler *Handler)
	{ Handler->Act(Button); }

Button::Button(const String &Label, const char *Icon, ActionHandler *Handler) :
	Handler(Handler), Data(gtk_button_new_with_label(Label.c_str()))
{
	gtk_button_set_image(GTK_BUTTON(Data), gtk_image_new_from_stock(Icon, GTK_ICON_SIZE_SMALL_TOOLBAR));

	if (Handler != NULL) g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(PressCallback), this);
}

Button::operator GtkWidget*(void)
	{ return Data; }

void Button::PressCallback(GtkWidget *Widget, Button *This)
	{ This->Handler->Act(This); }

ColorButton::ColorButton(const String &Label, const Color &Initial, ActionHandler *Handler) :
	Handler(Handler),
	Box(gtk_hbox_new(false, 0)), Label(!Label.empty() ? gtk_label_new(Label.c_str()) : NULL), Data(gtk_color_button_new())
{
	if (ColorButton::Label != NULL)
	{
		gtk_box_pack_start(GTK_BOX(Box), ColorButton::Label, false, true, 0);
		gtk_widget_show(ColorButton::Label);
	}

	GdkColor GdkInitialColor = {0, Initial.Red * ColorMax, Initial.Green * ColorMax, Initial.Blue * ColorMax};
	gtk_color_button_set_color(GTK_COLOR_BUTTON(Data), &GdkInitialColor);
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(Data), true);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(Data), Initial.Alpha * ColorMax);
	if (Handler != NULL) g_signal_connect(G_OBJECT(Data), "color-set", G_CALLBACK(PressCallback), this);
	gtk_box_pack_start(GTK_BOX(Box), Data, false, true, 0);
	gtk_widget_show(Data);
}

ColorButton::operator GtkWidget*(void)
	{ return Box; }

Color ColorButton::GetColor(void)
{
	GdkColor RGB; gtk_color_button_get_color(GTK_COLOR_BUTTON(Data), &RGB);

	return Color((float)RGB.red / ColorMax, (float)RGB.green / ColorMax, (float)RGB.blue / ColorMax,
		(float)gtk_color_button_get_alpha(GTK_COLOR_BUTTON(Data)) / ColorMax);
}

void ColorButton::PressCallback(GtkWidget *Widget, ColorButton *This)
	{ This->Handler->Act(This); }

const FlatVector ColorToggleButtonSize(54, 48);
ColorToggleButton::ColorToggleButton(bool InitiallyFore, const Color &ForegroundColor, const Color &BackgroundColor) :
	State(InitiallyFore), Foreground(ForegroundColor), Background(BackgroundColor),
	ColorArea(gtk_drawing_area_new()), Data(gtk_button_new())
{
	gtk_widget_set_size_request(ColorArea, ColorToggleButtonSize[0], ColorToggleButtonSize[1]);
	g_signal_connect(G_OBJECT(ColorArea), "expose_event", G_CALLBACK(RefreshCallback), this);

	gtk_button_set_image(GTK_BUTTON(Data), ColorArea);
	g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(ClickCallback), this);
}

ColorToggleButton::~ColorToggleButton(void)
	{}

ColorToggleButton::operator GtkWidget*(void)
	{ return Data; }

bool ColorToggleButton::GetState(void)
	{ return State; }

void ColorToggleButton::SetForegroundColor(const Color &NewForegroundColor)
	{ Foreground = NewForegroundColor; if (State) Refresh(); }

void ColorToggleButton::SetBackgroundColor(const Color &NewBackgroundColor)
	{ Background = NewBackgroundColor; if (!State) Refresh(); }

void ColorToggleButton::Refresh()
{
	if (!GDK_IS_WINDOW(ColorArea->window)) return;
	const GdkRectangle RefreshRectangle = {0, 0, ColorArea->allocation.width, ColorArea->allocation.height};
	gdk_window_invalidate_rect(ColorArea->window, &RefreshRectangle, false);
	gdk_window_process_updates(ColorArea->window, false);
}

gboolean ColorToggleButton::RefreshCallback(GtkWidget *Widget, GdkEventExpose *Event, ColorToggleButton *This)
{
	cairo_t *CairoContext = gdk_cairo_create(Event->window);
	Color &FillColor = This->State ? This->Foreground : This->Background;
	cairo_set_source_rgb(CairoContext, FillColor.Red, FillColor.Green, FillColor.Blue);
	cairo_paint(CairoContext);
	cairo_destroy(CairoContext);

	return true;
}

void ColorToggleButton::ClickCallback(GtkWidget *Widget, ColorToggleButton *This)
	{ This->State = !This->State; }


extern const float SliderFactor = 4, InverseSliderFactor = 1.0f / SliderFactor;
Slider::Slider(const String &Label, const RangeF &ValueRange, float Initial) :
	ValueRange(ValueRange),
	Box(gtk_hbox_new(false, 0)), Label(!Label.empty() ? gtk_label_new(Label.c_str()) : NULL),
	Value(gtk_entry_new()), Data(gtk_hscale_new_with_range(0, 1, 0.01))
{
	if (Slider::Label != NULL)
	{
		gtk_box_pack_start(GTK_BOX(Box), Slider::Label, false, true, 0);
		gtk_widget_show(Slider::Label);
	}

	gtk_entry_set_width_chars(GTK_ENTRY(Value), 6);
	gtk_box_pack_start(GTK_BOX(Box), Value, false, true, 0);
	gtk_widget_show(Value);

	gtk_scale_set_draw_value(GTK_SCALE(Data), false);
	gtk_widget_set_size_request(Data, 200, 0);
	SetValue(ValueRange.Constrain(Initial));
	gtk_box_pack_start(GTK_BOX(Box), Data, true, true, 0);
	gtk_widget_show(Data);

	SetValue(Initial);
	HandleChangeSliderPosition(Data, this);

	// Connect the slide notification last.  We could do it earlier, but then in most cases
	// the text would be updated twice.  Not beautiful!  Not gorgeous!
	SliderHandlerID = g_signal_connect(G_OBJECT(Data), "value-changed", G_CALLBACK(HandleChangeSliderPosition), this);

	// For the backward editing path
	g_signal_connect(G_OBJECT(Value), "changed", G_CALLBACK(HandleChangeEntryText), this);
}

Slider::operator GtkWidget*(void)
	{ return Box; }

float Slider::GetValue(void)
	{ return ValueRange.AtPercent(powf(gtk_range_get_value(GTK_RANGE(Data)), SliderFactor)); }

void Slider::SetValue(float NewValue)
	{ gtk_range_set_value(GTK_RANGE(Data), powf(RangeF(0, 1).Constrain(ValueRange.Percent(NewValue)), InverseSliderFactor)); }

void Slider::HandleChangeSliderPosition(GtkWidget *Widget, Slider *This)
{
	StringStream LabelRenderedText;
	LabelRenderedText << std::setprecision(4) << This->GetValue();
	gtk_entry_set_text(GTK_ENTRY(This->Value), LabelRenderedText.str().c_str());
}

void Slider::HandleChangeEntryText(GtkWidget *Widget, Slider *This)
{
	g_signal_handler_block(This->Data, This->SliderHandlerID);
	StringStream EntryText(gtk_entry_get_text(GTK_ENTRY(This->Value)));
	float NewValue;
	EntryText >> NewValue;
	This->SetValue(NewValue);
	g_signal_handler_unblock(This->Data, This->SliderHandlerID);
}

Dropdown::Dropdown(const String &Label) :
	Box(gtk_hbox_new(false, 0)), Label(!Label.empty() ? gtk_label_new(Label.c_str()) : NULL),
	Store(gtk_list_store_new(1, G_TYPE_STRING))
{
	if (Dropdown::Label != NULL)
	{
		gtk_box_pack_start(GTK_BOX(Box), Dropdown::Label, false, true, 0);
		gtk_widget_show(Dropdown::Label);
	}

	GtkTreeModel *Model = GTK_TREE_MODEL(Store);
	GtkCellRenderer *ColumnRenderer = gtk_cell_renderer_text_new();
	Data = gtk_combo_box_new_with_model(Model);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(Data), ColumnRenderer, true);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(Data), ColumnRenderer, "text", 0, NULL);

	gtk_box_pack_start(GTK_BOX(Box), Data, true, true, 0);
	gtk_widget_show(Data);
}

Dropdown::~Dropdown(void)
{
	g_object_unref(Store);
}

Dropdown::operator GtkWidget*(void)
	{ return Box; }

void Dropdown::Add(const String &Choice)
{
	GtkTreeIter Iterator;
	gtk_list_store_append(Store, &Iterator);
	gtk_list_store_set(Store, &Iterator, 0, Choice.c_str(), -1);
}

unsigned int Dropdown::GetValue(void)
	{ return gtk_combo_box_get_active(GTK_COMBO_BOX(Data)); }

void Dropdown::SetValue(unsigned int Choice)
	{ gtk_combo_box_set_active(GTK_COMBO_BOX(Data), Choice); }

Spinner::Spinner(const String &Label, const RangeD &ValueRange, int Initial) :
	ValueRange(ValueRange),
	Box(gtk_hbox_new(false, 0)), Label(!Label.empty() ? gtk_label_new(Label.c_str()) : NULL),
	Data(gtk_spin_button_new_with_range(ValueRange.Min, ValueRange.Max, 1))
{
	if (Spinner::Label != NULL)
	{
		gtk_box_pack_start(GTK_BOX(Box), Spinner::Label, false, true, 0);
		gtk_widget_show(Spinner::Label);
	}

	int IncrementValue = ValueRange.Length() < 100 ? 1 :
			ValueRange.Length() < 1000 ? (int)ValueRange.Length() / 100 : (int)ValueRange.Length() / 1000 * 10;
	gtk_spin_button_set_increments(GTK_SPIN_BUTTON(Data), IncrementValue, IncrementValue * 10);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(Data), ValueRange.Constrain(Initial));
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(Data), true);
	gtk_box_pack_start(GTK_BOX(Box), Data, false, true, 0);
	gtk_widget_show(Data);
}

Spinner::operator GtkWidget *(void)
	{ return Box; }

int Spinner::GetValue(void)
	{ return gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(Data)); }

void Spinner::SetValue(int NewValue)
	{ gtk_spin_button_set_value(GTK_SPIN_BUTTON(Data), NewValue); }
