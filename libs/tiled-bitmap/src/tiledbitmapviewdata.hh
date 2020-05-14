/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#pragma once

#include <gtk/gtk.h>

#include <boost/shared_ptr.hpp>
#include <scroom/bookkeeping.hh>
#include <scroom/observable.hh>
#include <scroom/viewinterface.hh>

#include "scroom/tiledbitmaplayer.hh"

class TiledBitmapViewData : virtual public Scroom::Utils::Base,
                            public TileLoadingObserver,
                            public ProgressInterface
{
  public:
  typedef boost::shared_ptr<TiledBitmapViewData> Ptr;

  public:
  ViewInterface::WeakPtr viewInterface;
  ProgressInterface::Ptr progressInterface;
  Scroom::Bookkeeping::Token token;

  private:
  Layer::Ptr layer;
  int imin;
  int imax;
  int jmin;
  int jmax;
  int zoom;
  LayerOperations::Ptr layerOperations;

  /**
   * References to things we want to keep around.
   *
   * This includes
   * @li observer registrations
   * @li tiles that have been loaded
   */
  Scroom::Utils::StuffList stuff;

  /**
   * References to things the user should be able to throw away on request
   *
   * This includes
   * @li pre-drawn bitmaps to make redraws go faster
   */
  Scroom::Utils::StuffList volatileStuff;

  bool redrawPending;

  /** Protect @c stuff and @c redrawPending */
  boost::mutex mut;

  private:
  TiledBitmapViewData(ViewInterface::WeakPtr viewInterface);

  public:
  static Ptr create(ViewInterface::WeakPtr viewInterface);

  void setNeededTiles(Layer::Ptr const& l, int imin, int imax, int jmin, int jmax, int zoom,
                      LayerOperations::Ptr layerOperations);
  void resetNeededTiles();
  void storeVolatileStuff(Scroom::Utils::Stuff stuff);
  void clearVolatileStuff();

  // TileLoadingObserver ////////////////////////////////////////////////
  virtual void tileLoaded(ConstTile::Ptr tile);

  // ProgressInterface ///////////////////////////////////////////////////
  virtual void setIdle();
  virtual void setWaiting(double progress = 0.0);
  virtual void setWorking(double progress);
  virtual void setFinished();
};
