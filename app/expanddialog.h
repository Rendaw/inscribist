// Copyright 2011 Rendaw, under the FreeBSD license (See included LICENSE.txt)

#ifndef expanddialog_h
#define expanddialog_h

#include "ren-gtk/gtkwrapper.h"

#include "image.h"

class ExpandDialog : public Dialog
{
	public:
		ExpandDialog(GtkWidget *Window, Image &Sketcher);

	private:
		Image &Sketcher;

		Wheel Scale;
		Wheel AddLeft, AddRight, AddTop, AddBottom;
		Label PostWidth, PostHeight;

		Button Okay;
		Button Cancel;
};

void OpenExpandDialog(GtkWidget *Window, Image &Sketcher);

#endif
