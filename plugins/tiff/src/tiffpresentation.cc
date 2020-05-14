/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#include "tiffpresentation.hh"

#include <string.h>
#include <tiffio.h>

#include <iostream>
#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>
#include <scroom/unused.hh>

TiffPresentation::TiffPresentation() : tif(NULL), height(0), width(0), bps(0), spp(0)
{
  colormapHelper = ColormapHelper::create(256);
}

TiffPresentation::Ptr TiffPresentation::create()
{
  return Ptr(new TiffPresentation());
}

TiffPresentation::~TiffPresentation()
{
  printf("TiffPresentation: Destructing...\n");

  if (tif != NULL)
  {
    TIFFClose(tif);
    tif = NULL;
  }

  tbi.reset();
}

void TiffPresentation::destroy()
{
  tbi.reset();
}

#define TIFFGetFieldChecked(file, field, ...)        \
  if (1 != TIFFGetField(file, field, ##__VA_ARGS__)) \
    throw std::invalid_argument("Field not present in tiff file: " #field);

bool TiffPresentation::load(const std::string& fileName_)
{
  try
  {
    this->fileName = fileName_;
    tif = TIFFOpen(fileName_.c_str(), "r");
    if (!tif)
    {
      // Todo: report error
      printf("PANIC: Failed to open file %s\n", fileName_.c_str());
      return false;
    }

    uint16 spp_ = 0;
    if (1 != TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp_))
      spp_ = 1;  // Default value, according to tiff spec
    if (spp_ != 1 && spp_ != 3)
    {
      printf("PANIC: Samples per pixel is not 1 or 3, but %d. Giving up\n", spp_);
      return false;
    }
    this->spp = spp_;

    TIFFGetFieldChecked(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetFieldChecked(tif, TIFFTAG_IMAGELENGTH, &height);

    uint16 bps_ = 0;
    if (1 != TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps_))
    {
      if (spp == 1)
        bps_ = 1;
      else
        bps_ = 8;
    }
    else
    {
      if (spp == 3)
      {
        if (bps_ != 8)
        {
          printf("PANIC: Bits per sample is not 8, but %d. Giving up\n", bps_);
          return false;
        }
      }
    }
    this->bps = bps_;

    Colormap::Ptr originalColormap;

    uint16 *r, *g, *b;
    int result = TIFFGetField(tif, TIFFTAG_COLORMAP, &r, &g, &b);
    if (result == 1)
    {
      originalColormap = Colormap::create();
      originalColormap->name = "Original";
      size_t count = 1UL << bps_;
      originalColormap->colors.resize(count);

      for (size_t i = 0; i < count; i++)
      {
        originalColormap->colors[i] =
            Color(1.0 * r[i] / 0xFFFF, 1.0 * g[i] / 0xFFFF, 1.0 * b[i] / 0xFFFF);
      }

      colormapHelper = ColormapHelper::create(originalColormap);
    }

    uint16 photometric;
    TIFFGetFieldChecked(tif, TIFFTAG_PHOTOMETRIC, &photometric);
    switch (photometric)
    {
    case PHOTOMETRIC_MINISBLACK:
      if (originalColormap)
        printf("WEIRD: Tiff contains a colormap, but photometric isn't palette\n");

      if (bps == 1 || bps == 8)
        colormapHelper = MonochromeColormapHelper::create(2);
      else
        colormapHelper = MonochromeColormapHelper::create(1 << bps);

      properties[MONOCHROME_COLORMAPPABLE_PROPERTY_NAME] = "";
      break;

    case PHOTOMETRIC_MINISWHITE:
      if (originalColormap)
        printf("WEIRD: Tiff contains a colormap, but photometric isn't palette\n");

      if (bps == 1 || bps == 8)
        colormapHelper = MonochromeColormapHelper::createInverted(2);
      else
        colormapHelper = MonochromeColormapHelper::createInverted(1 << bps);

      properties[MONOCHROME_COLORMAPPABLE_PROPERTY_NAME] = "";
      break;

    case PHOTOMETRIC_PALETTE:
      if (!originalColormap)
      {
        printf("WEIRD: Photometric is palette, but tiff doesn't contain a colormap\n");
        colormapHelper = ColormapHelper::create(1 << bps);
      }
      break;

    case PHOTOMETRIC_RGB:
      if (originalColormap)
        printf("WEIRD: Tiff contains a colormap, but photometric isn't palette\n");
      break;

    default:
      printf("PANIC: Unrecognized value for photometric\n");
      return false;
    }

    float resolutionX;
    float resolutionY;
    uint16 resolutionUnit;

    if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &resolutionX) &&
        TIFFGetField(tif, TIFFTAG_YRESOLUTION, &resolutionY) &&
        TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resolutionUnit) &&
        resolutionUnit == RESUNIT_NONE)
    {
      transformationData = TransformationData::create();
      transformationData->setAspectRatio(1 / resolutionX, 1 / resolutionY);
    }
    else
    {
      resolutionX = 1;
      resolutionY = 1;
    }

    std::cout << "This bitmap has size " << width << "*" << height << ", aspect ratio "
              << 1 / resolutionX << "*" << 1 / resolutionY << std::endl;
    LayerSpec ls;

    if (spp == 3)
    {
      ls.push_back(Operations24bpp::create());
    }
    else if (bps == 2 || bps == 4 || photometric == PHOTOMETRIC_PALETTE)
    {
      ls.push_back(Operations::create(colormapHelper, bps));
      ls.push_back(OperationsColormapped::create(colormapHelper, bps));
      properties[COLORMAPPABLE_PROPERTY_NAME] = "";
    }
    else if (bps == 1)
    {
      ls.push_back(Operations1bpp::create(colormapHelper));
      ls.push_back(Operations8bpp::create(colormapHelper));
    }
    else if (bps == 8)
    {
      ls.push_back(Operations8bpp::create(colormapHelper));
    }
    else
    {
      printf("PANIC: %d bits per pixel not supported\n", bps);
      return false;
    }

    tbi = createTiledBitmap(width, height, ls);
    tbi->setSource(shared_from_this<SourcePresentation>());
    return true;
  }
  catch (const std::exception& ex)
  {
    printf("PANIC: %s\n", ex.what());
    return false;
  }
}

TransformationData::Ptr TiffPresentation::getTransformationData() const
{
  return transformationData;
}

////////////////////////////////////////////////////////////////////////
// PresentationInterface
////////////////////////////////////////////////////////////////////////

Scroom::Utils::Rectangle<double> TiffPresentation::getRect()
{
  GdkRectangle rect;
  rect.x = 0;
  rect.y = 0;
  rect.width = width;
  rect.height = height;

  return rect;
}

void TiffPresentation::viewAdded(ViewInterface::WeakPtr viewInterface)
{
  views.insert(viewInterface);

  if (tbi)
    tbi->open(viewInterface);
  else
  {
    printf("ERROR: TiffPresentation::open(): No TiledBitmapInterface available!\n");
  }
}

void TiffPresentation::viewRemoved(ViewInterface::WeakPtr vi)
{
  views.erase(vi);

  if (tbi)
    tbi->close(vi);
  else
  {
    printf("ERROR: TiffPresentation::close(): No TiledBitmapInterface available!\n");
  }
}

std::set<ViewInterface::WeakPtr> TiffPresentation::getViews()
{
  return views;
}

void TiffPresentation::redraw(ViewInterface::Ptr const& vi, cairo_t* cr,
                              Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  drawOutOfBoundsWithoutBackground(cr, presentationArea, getRect(), pixelSizeFromZoom(zoom));

  if (tbi)
    tbi->redraw(vi, cr, presentationArea, zoom);
}

bool TiffPresentation::getProperty(const std::string& name, std::string& value)
{
  std::map<std::string, std::string>::iterator p = properties.find(name);
  bool found = false;
  if (p == properties.end())
  {
    found = false;
    value = "";
  }
  else
  {
    found = true;
    value = p->second;
  }

  return found;
}

bool TiffPresentation::isPropertyDefined(const std::string& name)
{
  return properties.end() != properties.find(name);
}

std::string TiffPresentation::getTitle()
{
  return fileName;
}

////////////////////////////////////////////////////////////////////////
// SourcePresentation
////////////////////////////////////////////////////////////////////////

void TiffPresentation::fillTiles(int startLine, int lineCount, int tileWidth, int firstTile,
                                 std::vector<Tile::Ptr>& tiles)
{
  // printf("Filling lines %d to %d, tile %d to %d (tileWidth = %d)\n",
  //        startLine, startLine+lineCount,
  //        firstTile, (int)(firstTile+tiles.size()),
  //        tileWidth);

  const uint32 startLine_ = static_cast<uint32>(startLine);
  const size_t firstTile_ = static_cast<size_t>(firstTile);
  const size_t scanLineSize = static_cast<size_t>(TIFFScanlineSize(tif));
  const size_t tileStride = static_cast<size_t>(tileWidth * spp * bps / 8);
  byte* row = new byte[scanLineSize];

  const size_t tileCount = tiles.size();
  byte** dataPtr = new byte*[tileCount];
  for (size_t tile = 0; tile < tileCount; tile++)
  {
    dataPtr[tile] = tiles[tile]->data.get();
  }

  for (size_t i = 0; i < static_cast<size_t>(lineCount); i++)
  {
    TIFFReadScanline(tif, static_cast<tdata_t>(row), static_cast<uint32>(i) + startLine_);

    for (size_t tile = 0; tile < tileCount - 1; tile++)
    {
      memcpy(dataPtr[tile], row + (firstTile_ + tile) * tileStride, tileStride);
      dataPtr[tile] += tileStride;
    }
    memcpy(dataPtr[tileCount - 1], row + (firstTile_ + tileCount - 1) * tileStride,
           scanLineSize - (firstTile_ + tileCount - 1) * tileStride);
    dataPtr[tileCount - 1] += tileStride;
  }

  delete[] row;
  delete[] dataPtr;
}

void TiffPresentation::done()
{
  TIFFClose(tif);
  tif = NULL;
}

////////////////////////////////////////////////////////////////////////
// Colormappable
////////////////////////////////////////////////////////////////////////

void TiffPresentation::setColormap(Colormap::Ptr colormap)
{
  colormapHelper->setColormap(colormap);
  clearCaches();
}

Colormap::Ptr TiffPresentation::getOriginalColormap()
{
  return colormapHelper->getOriginalColormap();
}

int TiffPresentation::getNumberOfColors()
{
  return colormapHelper->getNumberOfColors();
}

Color TiffPresentation::getMonochromeColor()
{
  return colormapHelper->getMonochromeColor();
}

void TiffPresentation::setMonochromeColor(const Color& c)
{
  colormapHelper->setMonochromeColor(c);
  clearCaches();
}

void TiffPresentation::setTransparentBackground()
{
  colormapHelper->setTransparentBackground();
  clearCaches();
}

void TiffPresentation::disableTransparentBackground()
{
  colormapHelper->disableTransparentBackground();
  clearCaches();
}

bool TiffPresentation::getTransparentBackground()
{
  return colormapHelper->getTransparentBackground();
}

void TiffPresentation::clearCaches()
{
  for (const Views::value_type& p : views)
  {
    ViewInterface::Ptr v = p.lock();
    if (v)
    {
      if (tbi)
      {
        tbi->clearCaches(v);
      }
      v->invalidate();
    }
  }
}

////////////////////////////////////////////////////////////////////////
// TiffPresentationWrapper
////////////////////////////////////////////////////////////////////////

TiffPresentationWrapper::TiffPresentationWrapper() : presentation(TiffPresentation::create()) {}

TiffPresentationWrapper::Ptr TiffPresentationWrapper::create()
{
  return Ptr(new TiffPresentationWrapper());
}

TiffPresentationWrapper::~TiffPresentationWrapper()
{
  presentation->destroy();
}

bool TiffPresentationWrapper::load(const std::string& fileName)
{
  return presentation->load(fileName);
}

TransformationData::Ptr TiffPresentationWrapper::getTransformationData() const
{
  return presentation->getTransformationData();
}

Scroom::Utils::Rectangle<double> TiffPresentationWrapper::getRect()
{
  return presentation->getRect();
}

void TiffPresentationWrapper::viewAdded(ViewInterface::WeakPtr viewInterface)
{
  presentation->viewAdded(viewInterface);
}

void TiffPresentationWrapper::redraw(ViewInterface::Ptr const& vi, cairo_t* cr,
                                     Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  presentation->redraw(vi, cr, presentationArea, zoom);
}

void TiffPresentationWrapper::viewRemoved(ViewInterface::WeakPtr vi)
{
  presentation->viewRemoved(vi);
}

std::set<ViewInterface::WeakPtr> TiffPresentationWrapper::getViews()
{
  return presentation->getViews();
}

bool TiffPresentationWrapper::getProperty(const std::string& name, std::string& value)
{
  return presentation->getProperty(name, value);
}

bool TiffPresentationWrapper::isPropertyDefined(const std::string& name)
{
  return presentation->isPropertyDefined(name);
}

std::string TiffPresentationWrapper::getTitle()
{
  return presentation->getTitle();
}

void TiffPresentationWrapper::setColormap(Colormap::Ptr colormap)
{
  presentation->setColormap(colormap);
}

Colormap::Ptr TiffPresentationWrapper::getOriginalColormap()
{
  return presentation->getOriginalColormap();
}

int TiffPresentationWrapper::getNumberOfColors()
{
  return presentation->getNumberOfColors();
}

Color TiffPresentationWrapper::getMonochromeColor()
{
  return presentation->getMonochromeColor();
}

void TiffPresentationWrapper::setMonochromeColor(const Color& c)
{
  presentation->setMonochromeColor(c);
}

void TiffPresentationWrapper::setTransparentBackground()
{
  presentation->setTransparentBackground();
}

void TiffPresentationWrapper::disableTransparentBackground()
{
  presentation->disableTransparentBackground();
}

bool TiffPresentationWrapper::getTransparentBackground()
{
  return presentation->getTransparentBackground();
}
