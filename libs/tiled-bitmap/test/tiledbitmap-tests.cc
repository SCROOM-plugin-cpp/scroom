/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#include <boost/test/unit_test.hpp>
#include <boost/weak_ptr.hpp>
#include <scroom/rectangle.hh>
#include <scroom/tiledbitmapinterface.hh>

//////////////////////////////////////////////////////////////

class DummyLayerOperations : public LayerOperations
{
  public:
  static Ptr create()
  {
    return Ptr(new DummyLayerOperations());
  }
  virtual ~DummyLayerOperations() {}

  virtual int getBpp()
  {
    return 8;
  }
  virtual void initializeCairo(cairo_t*) {}
  virtual void draw(cairo_t*, const ConstTile::Ptr, Scroom::Utils::Rectangle<double>,
                    Scroom::Utils::Rectangle<double>, int, Scroom::Utils::Stuff)
  {
  }
  virtual void drawState(cairo_t*, TileState, Scroom::Utils::Rectangle<double>) {}
  virtual void reduce(Tile::Ptr, const ConstTile::Ptr, int, int) {}
};

//////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(TiledBitmap_Tests)

BOOST_AUTO_TEST_CASE(tiledbitmap_can_be_deleted)
{
  LayerSpec ls;
  ls.push_back(DummyLayerOperations::create());
  TiledBitmapInterface::Ptr bitmap = createTiledBitmap(300000, 300000, ls);
  BOOST_CHECK(bitmap);
  boost::weak_ptr<TiledBitmapInterface> weak = bitmap;
  BOOST_CHECK(weak.lock());
  bitmap.reset();
  BOOST_CHECK(!bitmap);
  BOOST_CHECK(!weak.lock());
}

BOOST_AUTO_TEST_SUITE_END()
