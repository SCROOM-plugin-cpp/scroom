/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2018 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#include <scroom/colormappable.hh>
#include <scroom/unused.hh>

////////////////////////////////////////////////////////////////////////
// ColormapHelperBase

ColormapHelperBase::ColormapHelperBase(Colormap::Ptr const& colormap)
  : colormap(colormap), originalColormap(colormap)
{
}

void ColormapHelperBase::setColormap(Colormap::Ptr colormap)
{
  this->colormap = colormap;
}

void ColormapHelperBase::setOriginalColormap(Colormap::Ptr colormap)
{
  this->originalColormap = colormap;
}

Colormap::Ptr ColormapHelperBase::getOriginalColormap()
{
  return originalColormap;
}

int ColormapHelperBase::getNumberOfColors()
{
  return originalColormap->colors.size();
}

Color ColormapHelperBase::getMonochromeColor()
{
  OperationNotSupported();
}

void ColormapHelperBase::setMonochromeColor(const Color& c)
{
  UNUSED(c);
  OperationNotSupported();
}

void ColormapHelperBase::setTransparentBackground()
{
  OperationNotSupported();
}

void ColormapHelperBase::disableTransparentBackground()
{
  OperationNotSupported();
}

bool ColormapHelperBase::getTransparentBackground()
{
  OperationNotSupported();
}

Colormap::Ptr ColormapHelperBase::getColormap()
{
  return colormap;
}

void ColormapHelperBase::OperationNotSupported()
{
  throw std::runtime_error("Operation not supported");
}

////////////////////////////////////////////////////////////////////////
// ColormapHelper

ColormapHelper::Ptr ColormapHelper::create(Colormap::Ptr const& colormap)
{
  return Ptr(new ColormapHelper(colormap));
}

ColormapHelper::Ptr ColormapHelper::create(int numberOfColors)
{
  return create(Colormap::createDefault(numberOfColors));
}

ColormapHelper::Ptr ColormapHelper::createInverted(int numberOfColors)
{
  return create(Colormap::createDefaultInverted(numberOfColors));
}

ColormapHelper::ColormapHelper(Colormap::Ptr const& colormap)
  : ColormapHelperBase(colormap)
{
}

////////////////////////////////////////////////////////////////////////
// MonochromeColormapHelper

MonochromeColormapHelper::Ptr MonochromeColormapHelper::create(int numberOfColors)
{
  return Ptr(new MonochromeColormapHelper(numberOfColors, false));
}

MonochromeColormapHelper::Ptr MonochromeColormapHelper::createInverted(int numberOfColors)
{
  return Ptr(new MonochromeColormapHelper(numberOfColors, true));
}

MonochromeColormapHelper::MonochromeColormapHelper(int numberOfColors, bool inverted)
  : ColormapHelperBase(generateInitialColormap(numberOfColors, inverted)),
    numberOfColors(numberOfColors), inverted(inverted), blackish(Color(inverted?1:0)), whitish(Color(inverted?0:1)),
    transparentBackground(false)
{
}

void MonochromeColormapHelper::setMonochromeColor(const Color& c)
{
  if(inverted)
    whitish = c;
  else
    blackish = c;

  regenerateColormap();
}

Color MonochromeColormapHelper::getMonochromeColor()
{
  return inverted?whitish:blackish;
}

void MonochromeColormapHelper::regenerateColormap()
{
  Color w = whitish;
  Color b = blackish;
  if(transparentBackground)
  {
    if(inverted)
      b.setAlpha(0);
    else
      w.setAlpha(0);

  }

  for(int i=0; i<numberOfColors; i++)
  {
    colormap->colors[i] = mix(w, b, 1.0*i/(numberOfColors-1));
  }
}

Colormap::Ptr MonochromeColormapHelper::generateInitialColormap(int numberOfColors, bool inverted)
{
  return inverted?Colormap::createDefaultInverted(numberOfColors):Colormap::createDefault(numberOfColors);
}

void MonochromeColormapHelper::setTransparentBackground()
{
  transparentBackground = true;
  regenerateColormap();
}

void MonochromeColormapHelper::disableTransparentBackground()
{
  transparentBackground = false;
  regenerateColormap();
}

bool MonochromeColormapHelper::getTransparentBackground()
{
  return transparentBackground;
}