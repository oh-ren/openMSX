// $Id$

#include "V9990GLRasterizer.hh"

using std::string;

namespace openmsx {

V9990GLRasterizer::V9990GLRasterizer(V9990* vdp_)
	: vdp(vdp_)
{
	PRT_DEBUG("V9990GLRasterizer::V9990GLRasterizer()");
	vram = 0;
}

V9990GLRasterizer::~V9990GLRasterizer()
{
	PRT_DEBUG("V9990GLRasterizer::~V9990GLRasterizer()");
}

void V9990GLRasterizer::paint()
{
	PRT_DEBUG("V9990GLRasterizer::paint()");
}

const string& V9990GLRasterizer::getName()
{
	PRT_DEBUG("V9990GLRasterizer::getName()");
	static const string NAME = "V9990GLRasterizer";
	return NAME;
}

void V9990GLRasterizer::reset()
{
	PRT_DEBUG("V9990GLRasterizer::reset()");
}

void V9990GLRasterizer::frameStart(const V9990DisplayPeriod *horTiming,
                                   const V9990DisplayPeriod *verTiming)
{
	PRT_DEBUG("V9990GLRasterizer::frameStart()");
}

void V9990GLRasterizer::frameEnd()
{
	PRT_DEBUG("V9990GLRasterizer::frameEnd()");
}

void V9990GLRasterizer::setDisplayMode(V9990DisplayMode mode)
{
	PRT_DEBUG("V9990GLRasterizer::setDisplayMode(" << std::dec <<
	          (int) mode << ")");
}

void V9990GLRasterizer::setColorMode(V9990ColorMode mode)
{
	PRT_DEBUG("V9990GLRasterizer::setColorMode(" << std::dec <<
	          (int) mode << ")");
}

void V9990GLRasterizer::setPalette(int index, byte r, byte g, byte b)
{
	PRT_DEBUG("V9990GLRasterizer::setPalette(" << std::dec << index
	          << "," << (int) r << "," << (int) g << "," << (int) b << ")");
}

void V9990GLRasterizer::drawBorder(int fromX, int fromY, int toX, int toY)
{
	PRT_DEBUG("V9990GLRasterizer::drawBorder(" << std::dec <<
	          fromX << "," << fromY << "," << toX << "," << toY << ")");
}

void V9990GLRasterizer::drawDisplay(
		int fromX, int fromY,
		int displayX, int displayY,
		int displayWidth, int displayHeight
		)
{
	PRT_DEBUG("V9990GLRasterizer::drawDisplay(" << std::dec <<
	          fromX << "," << fromY << "," <<
	          displayX << "," << displayY << "," <<
	          displayWidth << "," << displayHeight << ")");
}

void V9990GLRasterizer::setImageWidth(int width)
{
	PRT_DEBUG("V9990GLRasterizer::setImageWidth(" << std::dec << width << ")");
}
} // namespace openmsx


