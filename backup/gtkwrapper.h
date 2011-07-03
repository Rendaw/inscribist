#ifndef gtkwrapper_h
#define gtkwrapper_h

#include <gtk/gtk.h>

#include <general/color.h>
#include <general/string.h>
#include <general/range.h>

bool Confirm(GtkWidget *Window, const String &Title, const String &Message);

void SetBackgroundColor(GtkWidget *Widget, const Color &NewColor);

class Layout
{
	public:
		Layout(bool Horizontal, int EdgePadding, int ItemPadding);
		operator GtkWidget*(void);

		void Add(GtkWidget *Widget);
		void AddFill(GtkWidget *Widget);
	private:
		GtkWidget *Data;
};

class ActionHandler
{
	public:
		virtual ~ActionHandler(void);

		virtual void Act(void *Source) = 0;
};

class Toolbar
{
	public:
		Toolbar(void);
		operator GtkWidget*(void);

		void *Add(const String &Label, const char *Icon, ActionHandler *Handler);
	private:
		static void HandlePress(GtkWidget *Button, ActionHandler *Handler);

		GtkWidget *Data;
};

class Button
{
	public:
		Button(const String &Label, const char *Icon, ActionHandler *Handler);
		operator GtkWidget*(void);
	private:
		static void PressCallback(GtkWidget *Widget, Button *This);
		ActionHandler *Handler;
		GtkWidget *Data;
};

class ColorButton
{
	public:
		ColorButton(const String &Label, const Color &Initial, ActionHandler *Handler);
		operator GtkWidget*(void);

		Color GetColor(void);
	private:
		static void PressCallback(GtkWidget *Widget, ColorButton *This);
		ActionHandler *Handler;
		GtkWidget *Box, *Label, *Value, *Data;
};

class ColorToggleButton
{
	public:
		ColorToggleButton(bool InitiallyFore, const Color &ForegroundColor, const Color &BackgroundColor);
		~ColorToggleButton(void);
		operator GtkWidget*(void);

		void SetState(bool Fore);
		bool GetState(void);

		void SetForegroundColor(const Color &NewForegroundColor);
		void SetBackgroundColor(const Color &NewBackgroundColor);
	private:
		void Refresh(void);
		static gboolean RefreshCallback(GtkWidget *Widget, GdkEventExpose *Event, ColorToggleButton *This);
		//GdkPixbuf *SetColor(GdkPixbuf *Destination, const Color &NewColor);
		static void ClickCallback(GtkWidget *Widget, ColorToggleButton *This);

		bool State;
		Color Foreground, Background;
		GtkWidget *ColorArea, *Data;
};

class Slider
{
	public:
		Slider(const String &Label, const RangeF &ValueRange, float Initial);
		operator GtkWidget*(void);

		float GetValue(void);
		void SetValue(float NewValue);
	private:
		static void HandleChangeSliderPosition(GtkWidget *Widget, Slider *This);

		const RangeF ValueRange;
		GtkWidget *Box, *Label, *Value, *Data;
};

class Spinner
{
	public:
		Spinner(const String &Label, const RangeD &ValueRange, int Initial);
		operator GtkWidget *(void);

		int GetValue(void);
		void SetValue(int NewValue);
	private:
		const RangeD ValueRange;
		GtkWidget *Box, *Label, *Data;
};

#endif
