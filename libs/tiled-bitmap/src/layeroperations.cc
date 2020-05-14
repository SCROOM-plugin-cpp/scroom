/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <scroom/bitmap-helpers.hh>
#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>
#include <scroom/unused.hh>
#include <sstream>

using Scroom::Utils::Stuff;
using namespace Scroom::Bitmap;

////////////////////////////////////////////////////////////////////////

namespace
{
  boost::shared_ptr<unsigned char> shared_malloc(size_t size)
  {
    return boost::shared_ptr<unsigned char>(static_cast<unsigned char*>(malloc(size)), free);
  }

}  // namespace
////////////////////////////////////////////////////////////////////////
// BitCountLut

// was: BitCountLut::lookup
// counts the number of 1 bits in n
// adapted from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
inline byte bitcount(byte n)
{
  n = static_cast<byte>(((0xaa & n) >> 1) + (0x55 & n));
  n = static_cast<byte>(((0xcc & n) >> 2) + (0x33 & n));
  n = static_cast<byte>(((0xf0 & n) >> 4) + (0x0f & n));
  return n;
}

////////////////////////////////////////////////////////////////////////
// CommonOperations

void CommonOperations::initializeCairo(cairo_t* cr)
{
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
}

void CommonOperations::drawState(cairo_t* cr, TileState s,
                                 Scroom::Utils::Rectangle<double> viewArea)
{
  Color c;

  switch (s)
  {
  case TILE_UNINITIALIZED:
    c = Color(1, 1, 0.5);  // Yellow
    break;
  case TILE_UNLOADED:
    c = Color(0.5, 1, 0.5);  // Green
    break;
  case TILE_LOADED:
    c = Color(1, 0.5, 0.5);  // Red
    break;
  case TILE_OUT_OF_BOUNDS:
  default:
    c = Colors::OUT_OF_BOUNDS;
    break;
  }

  drawRectangle(cr, c, viewArea);
}

void CommonOperations::drawPixelValue(cairo_t* cr, int x, int y, int size, int value)
{
  std::ostringstream s;
  s << value;
  std::string v = s.str();
  const char* cstr = v.c_str();

  cairo_move_to(cr, x, y);
  cairo_line_to(cr, x + size, y);
  cairo_line_to(cr, x + size, y + size);
  cairo_line_to(cr, x, y + size);
  cairo_line_to(cr, x, y);
  cairo_clip(cr);
  // cairo_stroke(cr);

  cairo_text_extents_t extents;
  cairo_text_extents(cr, cstr, &extents);

  double xx = x + size / 2 - (extents.width / 2 + extents.x_bearing);
  double yy = y + size / 2 - (extents.height / 2 + extents.y_bearing);

  cairo_move_to(cr, xx, yy);
  cairo_show_text(cr, cstr);
}

void CommonOperations::drawPixelValue(cairo_t* cr, int x, int y, int size, int value,
                                      Color const& bgColor)
{
  bgColor.getContrastingBlackOrWhite().setColor(cr);
  drawPixelValue(cr, x, y, size, value);
}

Scroom::Utils::Stuff CommonOperations::cacheZoom(const ConstTile::Ptr tile, int zoom,
                                                 Scroom::Utils::Stuff cache)
{
  // In: Cairo surface at zoom level 0
  // Out: Cairo surface at requested zoom level
  Scroom::Utils::Stuff result;
  if (zoom >= 0)
  {
    // Don't zoom in. It is a waste of space
    result = cache;
  }
  else if (!cache)
  {
    printf("PANIC: Base caching failed to return anything\n");
  }
  else
  {
    int divider = 1 << -zoom;
    BitmapSurface::Ptr source = boost::static_pointer_cast<BitmapSurface>(cache);
    BitmapSurface::Ptr target =
        BitmapSurface::create(tile->width / divider, tile->height / divider, CAIRO_FORMAT_ARGB32);
    result = target;

    cairo_surface_t* surface = target->get();
    cairo_t* cr = cairo_create(surface);
    initializeCairo(cr);
    cairo_scale(cr, 1.0 / divider, 1.0 / divider);
    cairo_set_source_surface(cr, source->get(), 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);
  }

  return result;
}

void CommonOperations::draw(cairo_t* cr, const ConstTile::Ptr tile,
                            Scroom::Utils::Rectangle<double> tileArea,
                            Scroom::Utils::Rectangle<double> viewArea, int zoom,
                            Scroom::Utils::Stuff cache)
{
  // In: Cairo surface at requested zoom level
  // Out: given surface rendered to the canvas
  UNUSED(tile);

  setClip(cr, viewArea);

  if (!cache)
  {
    drawState(cr, TILE_UNLOADED, viewArea);
  }
  else
  {
    BitmapSurface::Ptr source = boost::static_pointer_cast<BitmapSurface>(cache);

    if (zoom > 0)
    {
      // Ask Cairo to zoom in for us
      int multiplier = 1 << zoom;
      auto origin = viewArea.getTopLeft() / multiplier - tileArea.getTopLeft();

      cairo_scale(cr, multiplier, multiplier);
      cairo_set_source_surface(cr, source->get(), origin.x, origin.y);
      cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
      cairo_paint(cr);
    }
    else
    {
      // Cached bitmap is to scale
      int divider = 1 << -zoom;

      auto origin = viewArea.getTopLeft() - tileArea.getTopLeft() / divider;
      cairo_set_source_surface(cr, source->get(), origin.x, origin.y);
      cairo_paint(cr);
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Operations1bpp

LayerOperations::Ptr Operations1bpp::create(ColormapProvider::Ptr colormapProvider)
{
  return Ptr(new Operations1bpp(colormapProvider));
}

Operations1bpp::Operations1bpp(ColormapProvider::Ptr colormapProvider_)
    : colormapProvider(colormapProvider_)
{
}

int Operations1bpp::getBpp()
{
  return 1;
}

Scroom::Utils::Stuff Operations1bpp::cache(const ConstTile::Ptr tile)
{
  const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, tile->width);
  boost::shared_ptr<unsigned char> data = shared_malloc(static_cast<size_t>(stride * tile->height));
  Colormap::Ptr colormap = colormapProvider->getColormap();

  unsigned char* row = data.get();
  for (int j = 0; j < tile->height; j++, row += stride)
  {
    PixelIterator<const byte> bit(tile->data.get() + j * tile->width / 8, 0);
    uint32_t* pixel = reinterpret_cast<uint32_t*>(row);
    for (int i = 0; i < tile->width; i++)
    {
      *pixel = colormap->colors[*bit].getARGB32();
      pixel++;
      ++bit;
    }
  }

  return BitmapSurface::create(tile->width, tile->height, CAIRO_FORMAT_ARGB32, stride, data);
}

void Operations1bpp::reduce(Tile::Ptr target, const ConstTile::Ptr source, int x, int y)
{
  // Reducing by a factor 8. Source tile is 1bpp. Target tile is 8bpp
  int sourceStride = source->width / 8;
  const byte* sourceBase = source->data.get();

  int targetStride = target->width;
  byte* targetBase =
      target->data.get() + target->height * y * targetStride / 8 + target->width * x / 8;

  for (int j = 0; j < source->height / 8;
       j++, targetBase += targetStride, sourceBase += sourceStride * 8)
  {
    // Iterate vertically over target
    const byte* sourcePtr = sourceBase;
    byte* targetPtr = targetBase;

    for (int i = 0; i < source->width / 8; i++, sourcePtr++, targetPtr++)
    {
      // Iterate horizontally over target

      // Goal is to compute a 8-bit grey value from a 8*8 black/white
      // image. To do so, we take each of the 8 bytes, count the
      // number of 1's in each, and add them. Finally, we divide that
      // by 64 (the maximum number of ones in that area

      const byte* current = sourcePtr;
      int sum = 0;
      for (int k = 0; k < 8; k++, current += sourceStride) sum += bitcount(*current);

      *targetPtr = static_cast<byte>(sum * 255 / 64);
    }
  }
}

void Operations1bpp::draw(cairo_t* cr, const ConstTile::Ptr tile,
                          Scroom::Utils::Rectangle<double> tileArea,
                          Scroom::Utils::Rectangle<double> viewArea, int zoom,
                          Scroom::Utils::Stuff cache)
{
  cairo_save(cr);
  CommonOperations::draw(cr, tile, tileArea, viewArea, zoom, cache);
  cairo_restore(cr);

  // Draw pixelvalues at 32:1 zoom
  if (zoom == 5)
  {
    int multiplier = 1 << zoom;
    int stride = tile->width / 8;
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);

    Colormap::Ptr colormap = colormapProvider->getColormap();

    auto tileAreaInt = roundOutward(tileArea);
    auto offset = tileAreaInt.getTopLeft() - tileArea.getTopLeft();
    viewArea += offset * multiplier;

    for (int y = 0; y < tileAreaInt.getHeight(); y++)
    {
      const byte* const data = tile->data.get();
      PixelIterator<const byte> current(data + (tileAreaInt.getTop() + y) * stride,
                                        tileAreaInt.getLeft(), 1);

      for (int x = 0; x < tileAreaInt.getWidth(); x++, ++current)
      {
        const int value = *current;

        cairo_save(cr);
        drawPixelValue(cr, static_cast<int>(viewArea.x() + multiplier * x),
                       static_cast<int>(viewArea.y() + multiplier * y), multiplier, value,
                       colormap->colors[static_cast<size_t>(value)]);
        cairo_restore(cr);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Operations8bpp

LayerOperations::Ptr Operations8bpp::create(ColormapProvider::Ptr colormapProvider)
{
  return Ptr(new Operations8bpp(colormapProvider));
}

Operations8bpp::Operations8bpp(ColormapProvider::Ptr colormapProvider_)
    : colormapProvider(colormapProvider_)
{
}

int Operations8bpp::getBpp()
{
  return 8;
}

Scroom::Utils::Stuff Operations8bpp::cache(const ConstTile::Ptr tile)
{
  const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, tile->width);
  boost::shared_ptr<unsigned char> data = shared_malloc(static_cast<size_t>(stride * tile->height));
  Colormap::Ptr colormap = colormapProvider->getColormap();
  const Color& c1 = colormap->colors[0];
  const Color& c2 = colormap->colors[1];

  unsigned char* row = data.get();
  for (int j = 0; j < tile->height; j++, row += stride)
  {
    const byte* cur = tile->data.get() + j * tile->width;

    uint32_t* pixel = reinterpret_cast<uint32_t*>(row);
    for (int i = 0; i < tile->width; i++)
    {
      *pixel = mix(c2, c1, 1.0 * *cur / 255).getARGB32();

      pixel++;
      ++cur;
    }
  }

  return BitmapSurface::create(tile->width, tile->height, CAIRO_FORMAT_ARGB32, stride, data);
}

void Operations8bpp::reduce(Tile::Ptr target, const ConstTile::Ptr source, int x, int y)
{
  // Reducing by a factor 8. Source tile is 8bpp. Target tile is 8bpp
  int sourceStride = source->width;
  const byte* sourceBase = source->data.get();

  int targetStride = target->width;
  byte* targetBase =
      target->data.get() + target->height * y * targetStride / 8 + target->width * x / 8;

  for (int j = 0; j < source->height / 8;
       j++, targetBase += targetStride, sourceBase += sourceStride * 8)
  {
    // Iterate vertically over target
    const byte* sourcePtr = sourceBase;
    byte* targetPtr = targetBase;

    for (int i = 0; i < source->width / 8; i++, sourcePtr += 8, targetPtr++)
    {
      // Iterate horizontally over target

      // Goal is to compute a 8-bit grey value from a 8*8 grey image.
      const byte* base = sourcePtr;
      int sum = 0;
      for (int k = 0; k < 8; k++, base += sourceStride)
      {
        const byte* current = base;
        for (int l = 0; l < 8; l++, current++) sum += *current;
      }

      *targetPtr = static_cast<byte>(sum / 64);
    }
  }
}

void Operations8bpp::draw(cairo_t* cr, const ConstTile::Ptr tile,
                          Scroom::Utils::Rectangle<double> tileArea,
                          Scroom::Utils::Rectangle<double> viewArea, int zoom,
                          Scroom::Utils::Stuff cache)
{
  cairo_save(cr);
  CommonOperations::draw(cr, tile, tileArea, viewArea, zoom, cache);
  cairo_restore(cr);

  // Draw pixelvalues at 32:1 zoom
  if (zoom == 5)
  {
    int multiplier = 1 << zoom;
    int stride = tile->width;
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);

    Colormap::Ptr colormap = colormapProvider->getColormap();
    const Color& c1 = colormap->colors[0];
    const Color& c2 = colormap->colors[1];
    const byte* const data = tile->data.get();

    auto tileAreaInt = roundOutward(tileArea);
    auto offset = tileAreaInt.getTopLeft() - tileArea.getTopLeft();
    viewArea += offset * multiplier;

    for (int x = 0; x < tileAreaInt.width(); x++)
    {
      for (int y = 0; y < tileAreaInt.height(); y++)
      {
        const int value = data[(tileAreaInt.y() + y) * stride + tileAreaInt.x() + x];
        Color c = mix(c2, c1, 1.0 * value / 255);

        cairo_save(cr);
        drawPixelValue(cr, static_cast<int>(viewArea.x() + multiplier * x),
                       static_cast<int>(viewArea.y() + multiplier * y), multiplier, value, c);
        cairo_restore(cr);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Operations24bpp

LayerOperations::Ptr Operations24bpp::create()
{
  return Ptr(new Operations24bpp());
}

Operations24bpp::Operations24bpp() {}

int Operations24bpp::getBpp()
{
  return 24;
}

Scroom::Utils::Stuff Operations24bpp::cache(const ConstTile::Ptr tile)
{
  const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, tile->width);
  boost::shared_ptr<unsigned char> data = shared_malloc(static_cast<size_t>(stride * tile->height));
  unsigned char* row = data.get();
  for (int j = 0; j < tile->height; j++, row += stride)
  {
    const byte* cur = tile->data.get() + 3 * j * tile->width;

    uint32_t* pixel = reinterpret_cast<uint32_t*>(row);
    for (int i = 0; i < tile->width; i++)
    {
      //         A          R                                     G B
      *pixel = 0xFF000000 | static_cast<uint32_t>(cur[0] << 16) |
               static_cast<uint32_t>(cur[1] << 8) | cur[2];

      pixel++;
      cur += 3;
    }
  }

  return BitmapSurface::create(tile->width, tile->height, CAIRO_FORMAT_ARGB32, stride, data);
}

void Operations24bpp::reduce(Tile::Ptr target, const ConstTile::Ptr source, int x, int y)
{
  // Reducing by a factor 8. Source tile is 24bpp. Target tile is 24bpp
  int sourceStride = 3 * source->width;  // stride in bytes
  const byte* sourceBase = source->data.get();

  int targetStride = 3 * target->width;  // stride in bytes
  byte* targetBase =
      target->data.get() + target->height * y * targetStride / 8 + targetStride * x / 8;

  for (int j = 0; j < source->height / 8;
       j++, targetBase += targetStride, sourceBase += sourceStride * 8)
  {
    // Iterate vertically over target
    const byte* sourcePtr = sourceBase;
    byte* targetPtr = targetBase;

    for (int i = 0; i < source->width / 8; i++, sourcePtr += 8 * 3, targetPtr += 3)
    {
      // Iterate horizontally over target

      // Goal is to compute a average RGB value from a 8*8 RGB image.
      const byte* base = sourcePtr;
      int sum_r = 0;
      int sum_g = 0;
      int sum_b = 0;
      for (int k = 0; k < 8; k++, base += sourceStride)
      {
        const byte* current = base;
        for (int l = 0; l < 8; l++, current += 3)
        {
          sum_r += current[0];
          sum_g += current[1];
          sum_b += current[2];
        }
      }
      targetPtr[0] = static_cast<byte>(sum_r / 64);
      targetPtr[1] = static_cast<byte>(sum_g / 64);
      targetPtr[2] = static_cast<byte>(sum_b / 64);
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Operations

LayerOperations::Ptr Operations::create(ColormapProvider::Ptr colormapProvider, int bpp)
{
  return Ptr(new Operations(colormapProvider, bpp));
}

Operations::Operations(ColormapProvider::Ptr colormapProvider_, int bpp_)
    : colormapProvider(colormapProvider_),
      bpp(static_cast<unsigned int>(bpp_)),
      pixelsPerByte(8 / static_cast<unsigned int>(bpp_)),
      pixelOffset(static_cast<unsigned int>(bpp_)),
      pixelMask(static_cast<unsigned int>((1 << bpp_) - 1))
{
}

int Operations::getBpp()
{
  return static_cast<int>(bpp);
}

Scroom::Utils::Stuff Operations::cache(const ConstTile::Ptr tile)
{
  const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, tile->width);
  boost::shared_ptr<unsigned char> data = shared_malloc(static_cast<size_t>(stride * tile->height));
  Colormap::Ptr colormap = colormapProvider->getColormap();

  unsigned char* row = data.get();
  for (int j = 0; j < tile->height; j++, row += stride)
  {
    PixelIterator<const byte> pixelIn(
        tile->data.get() + static_cast<unsigned int>(j * tile->width) / pixelsPerByte, 0,
        static_cast<int>(bpp));

    uint32_t* pixelOut = reinterpret_cast<uint32_t*>(row);
    for (int i = 0; i < tile->width; i++)
    {
      *pixelOut = colormap->colors[*pixelIn].getARGB32();
      pixelOut++;
      ++pixelIn;
    }
  }

  return BitmapSurface::create(tile->width, tile->height, CAIRO_FORMAT_ARGB32, stride, data);
}

void Operations::reduce(Tile::Ptr target, const ConstTile::Ptr source, int x, int y)
{
  // Reducing by a factor 8. Target is 2*bpp and expects two indices into the colormap
  int sourceStride = source->width / static_cast<int>(pixelsPerByte);
  const byte* sourceBase = source->data.get();

  const int targetMultiplier = 2;  // target is 2*bpp
  int targetStride = targetMultiplier * target->width / static_cast<int>(pixelsPerByte);
  byte* targetBase = target->data.get() + target->height * targetStride * y / 8 +
                     targetMultiplier * target->width * x / 8 / static_cast<int>(pixelsPerByte);

  size_t lookup_size = pixelMask + 1;
  byte* lookup = new byte[lookup_size];

  for (int j = 0; j < source->height / 8;
       j++, targetBase += targetStride, sourceBase += sourceStride * 8)
  {
    // Iterate vertically over target
    const byte* sourcePtr = sourceBase;
    PixelIterator<uint16_t> targetPtr(reinterpret_cast<uint16_t*>(targetBase), 0,
                                      targetMultiplier * static_cast<int>(bpp));

    for (int i = 0; i < source->width / 8; i++, sourcePtr += 8 / pixelsPerByte, ++targetPtr)
    {
      // Iterate horizontally over target

      // Goal is to determine which values occurs most often in a 8*8
      // rectangle, and pick the top two.
      const byte* base = sourcePtr;
      memset(lookup, 0, lookup_size);

      for (int k = 0; k < 8; k++, base += sourceStride)
      {
        PixelIterator<const byte> current(base, 0, static_cast<int>(bpp));
        for (int l = 0; l < 8; l++, ++current) ++(lookup[*current]);
      }
      unsigned first = 0;
      unsigned second = 1;
      if (lookup[1] > lookup[0])
      {
        first = 1;
        second = 0;
      }
      for (unsigned c = 2; c < pixelMask + 1; c++)
      {
        if (lookup[c] > lookup[first])
        {
          second = first;
          first = c;
        }
        else if (lookup[c] > lookup[second])
          second = c;
      }
      if (lookup[second] == 0)
        second = first;

      targetPtr.set(static_cast<uint16_t>(first << pixelOffset | second));
    }
  }

  delete[] lookup;
}

void Operations::draw(cairo_t* cr, const ConstTile::Ptr tile,
                      Scroom::Utils::Rectangle<double> tileArea,
                      Scroom::Utils::Rectangle<double> viewArea, int zoom,
                      Scroom::Utils::Stuff cache)
{
  cairo_save(cr);
  CommonOperations::draw(cr, tile, tileArea, viewArea, zoom, cache);
  cairo_restore(cr);

  // Draw pixelvalues at 32:1 zoom
  if (zoom == 5)
  {
    int multiplier = 1 << zoom;
    int stride = tile->width / static_cast<int>(pixelsPerByte);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);

    Colormap::Ptr colormap = colormapProvider->getColormap();

    auto tileAreaInt = roundOutward(tileArea);
    auto offset = tileAreaInt.getTopLeft() - tileArea.getTopLeft();
    viewArea += offset * multiplier;

    for (int y = 0; y < tileAreaInt.height(); y++)
    {
      const byte* const data = tile->data.get();
      PixelIterator<const byte> current(data + (tileAreaInt.y() + y) * stride, tileAreaInt.x(),
                                        static_cast<int>(bpp));

      for (int x = 0; x < tileAreaInt.width(); x++, ++current)
      {
        const int value = *current;

        cairo_save(cr);
        drawPixelValue(cr, static_cast<int>(viewArea.x() + multiplier * x),
                       static_cast<int>(viewArea.y() + multiplier * y), multiplier, value,
                       colormap->colors[static_cast<size_t>(value)]);
        cairo_restore(cr);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// OperationsColormapped

LayerOperations::Ptr OperationsColormapped::create(ColormapProvider::Ptr colormapProvider, int bpp)
{
  return Ptr(new OperationsColormapped(colormapProvider, bpp));
}

OperationsColormapped::OperationsColormapped(ColormapProvider::Ptr colormapProvider_, int bpp_)
    : Operations(colormapProvider_, bpp_)
{
}

int OperationsColormapped::getBpp()
{
  return 2 * static_cast<int>(bpp);
}

Scroom::Utils::Stuff OperationsColormapped::cache(const ConstTile::Ptr tile)
{
  const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, tile->width);
  boost::shared_ptr<unsigned char> data = shared_malloc(static_cast<size_t>(stride * tile->height));
  Colormap::Ptr colormap = colormapProvider->getColormap();
  const int multiplier = 2;  // data is 2*bpp, containing 2 colors

  unsigned char* row = data.get();
  for (int j = 0; j < tile->height; j++, row += stride)
  {
    PixelIterator<const uint16_t> pixelIn(
        reinterpret_cast<uint16_t const*>(tile->data.get() +
                                          static_cast<unsigned int>(j * multiplier * tile->width) /
                                              pixelsPerByte),
        0, static_cast<int>(multiplier * bpp));
    uint32_t* pixelOut = reinterpret_cast<uint32_t*>(row);
    for (int i = 0; i < tile->width; i++)
    {
      *pixelOut = mix(colormap->colors[*pixelIn & pixelMask],
                      colormap->colors[static_cast<size_t>(*pixelIn >> pixelOffset)], 0.5)
                      .getARGB32();

      pixelOut++;
      ++pixelIn;
    }
  }

  return BitmapSurface::create(tile->width, tile->height, CAIRO_FORMAT_ARGB32, stride, data);
}

void OperationsColormapped::reduce(Tile::Ptr target, const ConstTile::Ptr source, int x, int y)
{
  // Reducing by a factor 8. Source and target both 2*bpp, containing 2 colors
  const int multiplier = 2;  // data is 2*bpp, containing 2 colors
  int sourceStride = multiplier * source->width / static_cast<int>(pixelsPerByte);
  const byte* sourceBase = source->data.get();

  int targetStride = multiplier * target->width / static_cast<int>(pixelsPerByte);
  byte* targetBase = target->data.get() + target->height * y * targetStride / 8 +
                     multiplier * target->width * x / 8 / static_cast<int>(pixelsPerByte);

  size_t lookup_size = pixelMask + 1;
  byte* lookup = new byte[lookup_size];

  for (int j = 0; j < source->height / 8;
       j++, targetBase += targetStride, sourceBase += sourceStride * 8)
  {
    // Iterate vertically over target
    const byte* sourcePtr = sourceBase;
    PixelIterator<uint16_t> targetPtr(reinterpret_cast<uint16_t*>(targetBase), 0,
                                      static_cast<int>(multiplier * bpp));

    for (int i = 0; i < source->width / 8;
         i++, sourcePtr += 8 * multiplier / pixelsPerByte, ++targetPtr)
    {
      // Iterate horizontally over target

      // Goal is to determine which value occurs most often in a 8*8
      // rectangle, and pick that value.
      const byte* base = sourcePtr;
      memset(lookup, 0, lookup_size);

      for (int k = 0; k < 8; k++, base += sourceStride)
      {
        PixelIterator<const uint16_t> current(reinterpret_cast<uint16_t const*>(base), 0,
                                              static_cast<int>(multiplier * bpp));
        for (int l = 0; l < 8; l++, ++current)
        {
          ++lookup[*current & pixelMask];
          ++lookup[*current >> pixelOffset];
        }
      }
      unsigned first = 0;
      unsigned second = 1;
      if (lookup[1] > lookup[0])
      {
        first = 1;
        second = 0;
      }
      for (unsigned c = 2; c < pixelMask + 1; c++)
      {
        if (lookup[c] > lookup[first])
        {
          second = first;
          first = c;
        }
        else if (lookup[c] > lookup[second])
          second = c;
      }
      if (lookup[second] == 0)
        second = first;

      targetPtr.set(static_cast<uint16_t>(first << pixelOffset | second));
    }
  }

  delete[] lookup;
}

////////////////////////////////////////////////////////////////////////
// Operations1bppClipped

LayerOperations::Ptr Operations1bppClipped::create(ColormapProvider::Ptr colormapProvider)
{
  return Ptr(new Operations1bppClipped(colormapProvider));
}

Operations1bppClipped::Operations1bppClipped(ColormapProvider::Ptr colormapProvider_)
    : colormapProvider(colormapProvider_)
{
}

int Operations1bppClipped::getBpp()
{
  return 1;
}

Scroom::Utils::Stuff Operations1bppClipped::cacheZoom(const ConstTile::Ptr tile, int zoom,
                                                      Scroom::Utils::Stuff cache)
{
  UNUSED(cache);

  if (zoom >= 0)
    zoom = 0;

  const int pixelSize = 1 << (-zoom);
  const int outputWidth = tile->width / pixelSize;
  const int outputHeight = tile->height / pixelSize;

  const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, outputWidth);
  boost::shared_ptr<unsigned char> data = shared_malloc(static_cast<size_t>(stride * outputHeight));
  Colormap::Ptr colormap = colormapProvider->getColormap();

  unsigned char* row = data.get();
  for (int j = 0; j < outputHeight; j++, row += stride)
  {
    uint32_t* pixel = reinterpret_cast<uint32_t*>(row);
    for (int i = 0; i < outputWidth; i++)
    {
      int sum = 0;

      for (int y = 0; y < pixelSize; y++)
      {
        const byte* inputByte =
            tile->data.get() + (j * pixelSize + y) * tile->width / 8 + pixelSize * i / 8;
        byte inputBit = static_cast<byte>(pixelSize * i % 8);

        PixelIterator<const byte> bit(inputByte, inputBit);

        for (int x = 0; x < pixelSize; x++, ++bit)
        {
          if (*bit)
            sum++;
        }
      }

      if (sum > 0)
        sum = 1;
      *pixel = colormap->colors[static_cast<size_t>(sum)].getARGB32();
      pixel++;
    }
  }

  return BitmapSurface::create(outputWidth, outputHeight, CAIRO_FORMAT_ARGB32, stride, data);
}

void Operations1bppClipped::reduce(Tile::Ptr target, const ConstTile::Ptr source, int x, int y)
{
  // Reducing by a factor 8. Source tile is 1bpp. Target tile is 1bpp
  int sourceStride = source->width / 8;
  const byte* sourceBase = source->data.get();

  int targetStride = target->width / 8;
  byte* targetBase =
      target->data.get() + target->height * y * targetStride / 8 + target->width * x / 8 / 8;

  for (int j = 0; j < source->height / 8;
       j++, targetBase += targetStride, sourceBase += sourceStride * 8)
  {
    // Iterate vertically over target
    const byte* sourcePtr = sourceBase;
    PixelIterator<byte> targetPtr(targetBase, 0);

    for (int i = 0; i < source->width / 8; i++, sourcePtr++, targetPtr++)
    {
      // Iterate horizontally over target

      // Goal is to compute a 8-bit grey value from a 8*8 black/white
      // image. To do so, we take each of the 8 bytes, count the
      // number of 1's in each, and add them. Finally, we divide that
      // by 64 (the maximum number of ones in that area

      const byte* current = sourcePtr;
      int sum = 0;
      for (int k = 0; k < 8; k++, current += sourceStride) sum += bitcount(*current);

      targetPtr.set((sum > 0) ? 1 : 0);
    }
  }
}
