#include "simplesketcher.h"
#include <cmath>

#include <general/rotation.h>

SimpleSketcher::SimpleSketcher(const FlatVector &ImageSize, int Downscale) :
	ImageSpace(FlatVector(), ImageSize),
	ZoomOut(Downscale), ZoomScale(1.0f / (float)ZoomOut),
	DisplaySpace(FlatVector(), ImageSpace.Size * ZoomScale),
	Data(cairo_image_surface_create(CAIRO_FORMAT_A8, ImageSpace.Size[0], ImageSpace.Size[1])),
	Context(cairo_create(Data))
{
	cairo_set_operator(Context, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(Context, 1, 1, 1, 0);
	cairo_paint(Context);
}

SimpleSketcher::SimpleSketcher(const String &Filename, const FlatVector &ImageSize, int Downscale) :
	ImageSpace(FlatVector(), ImageSize),
	ZoomOut(Downscale), ZoomScale(1.0f / (float)ZoomOut),
	DisplaySpace(FlatVector(), ImageSpace.Size * ZoomScale),
	Data(cairo_image_surface_create_from_png(Filename.c_str())),
	Context(NULL)
{
	if (cairo_surface_status(Data) != CAIRO_STATUS_SUCCESS)
	{
		// Load failed, make a new image
		cairo_surface_destroy(Data);
		Data = cairo_image_surface_create(CAIRO_FORMAT_A8, ImageSize[0], ImageSize[1]);
	}
	else
	{
		// Load succeeded, calculate correct size values
		ImageSpace.Size[0] = cairo_image_surface_get_width(Data);
		ImageSpace.Size[1] = cairo_image_surface_get_height(Data);
		DisplaySpace.Size = ImageSpace.Size * ZoomScale;
	}
	Context = cairo_create(Data);
}

SimpleSketcher::~SimpleSketcher(void)
{
	cairo_destroy(Context);
	cairo_surface_destroy(Data);
}

bool SimpleSketcher::Save(const String &Filename)
{
	cairo_status_t Result = cairo_surface_write_to_png(Data, Filename.c_str());
	return Result == CAIRO_STATUS_SUCCESS;
}

bool SimpleSketcher::Export(const String &Filename, const Color &Foreground, const Color &Background, int Downscale)
{
	// Create an export surface
	cairo_surface_t *ExportSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		ImageSpace.Size[0] / Downscale, ImageSpace.Size[1] / Downscale);
	cairo_t *ExportContext = cairo_create(ExportSurface);

	// Scale and draw to the export surface
	float Scale = 1.0f / (float)Downscale;
	RenderInternal(Region(FlatVector(), ImageSpace.Size * Scale), ExportContext, Scale, Foreground, Background);

	// Save it and clean up
	cairo_status_t Result = cairo_surface_write_to_png(ExportSurface, (Filename + ".png").c_str());
	if (Result != CAIRO_STATUS_SUCCESS)
		return false;

	cairo_destroy(ExportContext);
	cairo_surface_destroy(ExportSurface);

	return true;
}

Region SimpleSketcher::Mark(const CursorState &Start, const CursorState &End, const bool &Black)
{
	const FlatVector From(DisplaySpace.Transform(Start.Position, ImageSpace)),
		To(DisplaySpace.Transform(End.Position, ImageSpace));
	const FlatVector Difference = To - From;
	const float ConnectionAngle = Angle(Difference);

	/// Calculate the area we're modifying
	const Region MarkedRegion(
		FlatVector(
			std::min(From[0] - Start.Radius, To[0] - End.Radius),
			std::min(From[1] - Start.Radius, To[1] - End.Radius)),
		FlatVector(
			fabs(Difference[0]) + Start.Radius + End.Radius,
			fabs(Difference[1]) + Start.Radius + End.Radius));

	/// Clip to that region and draw to the image
	cairo_rectangle(Context,
		(int)MarkedRegion.Start[0], (int)MarkedRegion.Start[1], (int)MarkedRegion.Size[0] + 1, (int)MarkedRegion.Size[1] + 1);
	cairo_clip(Context);
	cairo_set_source_rgba(Context, 1, 1, 1, Black ? 1 : 0);
	cairo_arc(Context, From[0], From[1], Start.Radius, (ConnectionAngle + 90) * ToRadians, (ConnectionAngle + 270) * ToRadians);
	cairo_arc(Context, To[0], To[1], End.Radius, (ConnectionAngle - 90) * ToRadians, (ConnectionAngle + 90) * ToRadians);
	cairo_fill(Context);
	cairo_reset_clip(Context);

	/// Return the modified region in display space for the display to use to refresh
	return ImageSpace.Transform(MarkedRegion, DisplaySpace);
}

bool SimpleSketcher::Render(const Region &Invalid, cairo_t *Destination, const Color &Foreground, const Color &Background)
{
	return RenderInternal(Invalid, Destination, ZoomScale, Foreground, Background);
}

int SimpleSketcher::Zoom(int Amount)
{
	ZoomOut = std::max(1, (ZoomOut + Amount));
	ZoomScale = 1.0f / (float)ZoomOut;
	DisplaySpace.Size = ImageSpace.Size * ZoomScale;
	return ZoomOut;
}

FlatVector &SimpleSketcher::GetDisplaySize(void)
	{ return DisplaySpace.Size; }

bool SimpleSketcher::HasChanges(void) { return false; }
void SimpleSketcher::Undo(void) {}
void SimpleSketcher::Redo(void) {}

bool SimpleSketcher::RenderInternal(const Region &Invalid, cairo_t *Destination, float Scale,
	const Color &Foreground, const Color &Background)
{
	cairo_scale(Destination, Scale, Scale);

	/*const float BackgroundScale = 0.85f;
	cairo_set_source_rgba(Destination,
		Background.Red * BackgroundScale, Background.Green * BackgroundScale,
		Background.Blue * BackgroundScale, Background.Alpha * BackgroundScale);
	cairo_paint(Destination);*/

	cairo_set_source_rgba(Destination, Background.Red, Background.Green, Background.Blue, Background.Alpha);
	cairo_rectangle(Destination, (int)ImageSpace.Start[0], (int)ImageSpace.Start[1], (int)ImageSpace.Size[0], (int)ImageSpace.Size[1]);
	cairo_fill(Destination);

	cairo_set_source_rgba(Destination, Foreground.Red, Foreground.Green, Foreground.Blue, Foreground.Alpha);
	cairo_mask_surface(Destination, Data, 0, 0);
	cairo_fill(Destination);

	// Since there's only one sketcher, and it does more or less all the rendering, don't worry about
	// cleaning up the cairo context.
	return true;
}
