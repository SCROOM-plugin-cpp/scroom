/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <list>
#include <map>
#include <scroom/progressinterfacehelpers.hh>
#include <scroom/scroominterface.hh>
#include <scroom/threadpool.hh>
#include <scroom/tiledbitmapinterface.hh>

#include "layercoordinator.hh"
#include "scroom/tiledbitmaplayer.hh"
#include "tiledbitmapviewdata.hh"

class TiledBitmap;

class FileOperation
{
  public:
  typedef boost::shared_ptr<FileOperation> Ptr;

  private:
  ProgressInterface::Ptr progress;
  boost::mutex waitingMutex;
  bool waiting;

  protected:
  FileOperation(ProgressInterface::Ptr progress);

  public:
  virtual ~FileOperation() {}

  virtual void doneWaiting();
  virtual void finished() = 0;
  virtual void operator()() = 0;
  virtual void abort() = 0;
};

class TiledBitmap : public TiledBitmapInterface,
                    public TileInitialisationObserver,
                    public virtual Scroom::Utils::Base
{
  public:
  typedef boost::shared_ptr<TiledBitmap> Ptr;
  typedef boost::weak_ptr<TiledBitmap> WeakPtr;
  typedef std::map<ViewInterface::WeakPtr, TiledBitmapViewData::Ptr> ViewDataMap;

  private:
  int bitmapWidth;
  int bitmapHeight;
  LayerSpec ls;
  std::vector<Layer::Ptr> layers;
  std::list<LayerCoordinator::Ptr> coordinators;
  boost::mutex viewDataMutex;
  ViewDataMap viewData;
  int tileCount;
  boost::mutex tileFinishedMutex;
  int tileFinishedCount;
  FileOperation::Ptr fileOperation;
  Scroom::Utils::ProgressInterfaceBroadcaster::Ptr progressBroadcaster;
  ThreadPool::Queue::Ptr queue;

  public:
  static Ptr create(int bitmapWidth, int bitmapHeight, LayerSpec const& ls);
  virtual ~TiledBitmap();

  private:
  TiledBitmap(int bitmapWidth, int bitmapHeight, LayerSpec const& ls);
  void initialize();

  private:
  void drawTile(cairo_t* cr, const CompressedTile::Ptr tile,
                const Scroom::Utils::Rectangle<double> viewArea);
  void connect(Layer::Ptr const& layer, Layer::Ptr const& prevLayer, LayerOperations::Ptr prevLo);

  public:
  ////////////////////////////////////////////////////////////////////////
  // TiledBitmapInterface

  public:
  virtual void setSource(SourcePresentation::Ptr sp);
  virtual Layer::Ptr getBottomLayer();

  virtual void open(ViewInterface::WeakPtr viewInterface);
  virtual void close(ViewInterface::WeakPtr vi);
  virtual void redraw(ViewInterface::Ptr const& vi, cairo_t* cr,
                      Scroom::Utils::Rectangle<double> const& presentationArea, int zoom);
  virtual void clearCaches(ViewInterface::Ptr vi);

  ////////////////////////////////////////////////////////////////////////
  // TileInitialisationObserver

  virtual void tileCreated(CompressedTile::Ptr tile);
  virtual void tileFinished(CompressedTile::Ptr tile);
};
