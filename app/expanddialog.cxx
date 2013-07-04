// Copyright 2013 Rendaw, under the FreeBSD license (See included license.txt)

#include "expanddialog.h"

#include "ren-translation/translation.h"

ExpandDialog::ExpandDialog(GtkWidget *Window, Image &Sketcher) : Dialog(Window, Local("Expand Image")),
	Sketcher(Sketcher),
	Scale(Local("Factor"), RangeF(1, 10), 1),
	AddLeft(Local("Left"), RangeF(0, 1000000), 0), 
	AddRight(Local("Right"), RangeF(0, 1000000), 0), 
	AddTop(Local("Up"), RangeF(0, 1000000), 0), 
	AddBottom(Local("Down"), RangeF(0, 1000000), 0), 
	PostWidth(""), PostHeight(""),
	Okay(Local("Okay"), diSave),
	Cancel(Local("Cancel"), diClose)
{
	auto UpdateResultLabels = [&]()
	{
		PostWidth.SetText(Local("Width: ") + AsString(
			(int)Sketcher.GetSize()[0] * Scale.GetValue() + 
			AddLeft.GetValue() + AddRight.GetValue()));
		PostHeight.SetText(Local("Height: ") + AsString(
			(int)Sketcher.GetSize()[1] * Scale.GetValue() + 
			AddTop.GetValue() + AddBottom.GetValue()));
	};
	UpdateResultLabels();

	Layout BeforeLayout(false);
	BeforeLayout.Add(Label(Local("Width: ") + AsString((int)Sketcher.GetSize()[0])));
	BeforeLayout.Add(Label(Local("Height: ") + AsString((int)Sketcher.GetSize()[1])));
	LayoutBorder BeforeBorder(Local("Original Size"));
	BeforeBorder.Set(BeforeLayout);
	Add(BeforeBorder);

	LayoutBorder ScaleBorder(Local("Scale"));
	Scale.SetInputHandler([UpdateResultLabels]() { UpdateResultLabels(); });
	ScaleBorder.Set(Scale);
	Add(ScaleBorder);

	Layout AddLayout(false);
	AddLeft.SetInputHandler([UpdateResultLabels]() { UpdateResultLabels(); });
	AddLayout.Add(AddLeft);
	AddRight.SetInputHandler([UpdateResultLabels]() { UpdateResultLabels(); });
	AddLayout.Add(AddRight);
	AddTop.SetInputHandler([UpdateResultLabels]() { UpdateResultLabels(); });
	AddLayout.Add(AddTop);
	AddBottom.SetInputHandler([UpdateResultLabels]() { UpdateResultLabels(); });
	AddLayout.Add(AddBottom);
	LayoutBorder AddBorder(Local("Add"));
	AddBorder.Set(AddLayout);
	Add(AddBorder);
	
	Layout AfterLayout(false);
	AfterLayout.Add(PostWidth);
	AfterLayout.Add(PostHeight);
	LayoutBorder AfterBorder(Local("New Size"));
	AfterBorder.Set(AfterLayout);
	Add(AfterBorder);

	Okay.SetAction([&]()
	{
		Sketcher.Scale(Scale.GetValue());
		Sketcher.Add(AddLeft.GetValue(), AddRight.GetValue(), AddTop.GetValue(), AddBottom.GetValue());
		Close();
	});
	AddAction(Okay);

	Cancel.SetAction([&]() { Close(); });
	AddAction(Cancel);
}

void OpenExpandDialog(GtkWidget *Window, Image &Sketcher)
{
	ExpandDialog Dialog(Window, Sketcher);
	Dialog.Run();
}

