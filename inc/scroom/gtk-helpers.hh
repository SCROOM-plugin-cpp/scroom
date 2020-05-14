/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#pragma once

#include <gtk/gtk.h>

#include <boost/function.hpp>
#include <ostream>

namespace Scroom
{
  namespace GtkHelpers
  {
    class Wrapper
    {
      public:
      GtkFunction f;
      gpointer data;

      public:
      Wrapper(const boost::function<bool()>& f);
    };

    Wrapper wrap(boost::function<bool()> f);

    class TakeGdkLock
    {
      public:
      TakeGdkLock();
      ~TakeGdkLock();
    };

    void useRecursiveGdkLock();

    inline GdkRectangle createGdkRectangle(int x, int y, int width, int height)
    {
      GdkRectangle rect;
      rect.x = x;
      rect.y = y;
      rect.width = width;
      rect.height = height;
      return rect;
    }

  }  // namespace GtkHelpers
}  // namespace Scroom

inline bool operator==(GdkRectangle const& left, GdkRectangle const& right)
{
  return left.x == right.x && left.y == right.y && left.width == right.width &&
         left.height == right.height;
}

std::ostream& operator<<(std::ostream& os, GdkRectangle const& r);
