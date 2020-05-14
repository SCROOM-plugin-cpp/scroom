/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#pragma once

#include <cairo.h>
#include <gtk/gtk.h>

#include <boost/shared_ptr.hpp>
#include <list>
#include <map>
#include <scroom/presentationinterface.hh>
#include <scroom/progressinterfacehelpers.hh>
#include <scroom/utilities.hh>
#include <scroom/viewinterface.hh>
#include <vector>

#include "sizedeterminer.hh"

class TransparentOverlayViewInfo;

class ChildView : public ViewInterface, virtual public Scroom::Utils::Base
{
  public:
  typedef boost::shared_ptr<ChildView> Ptr;

  private:
  boost::shared_ptr<TransparentOverlayViewInfo> parent;
  ProgressInterface::Ptr progressInterface;

  private:
  ChildView(boost::shared_ptr<TransparentOverlayViewInfo> const& parent);

  public:
  static Ptr create(boost::shared_ptr<TransparentOverlayViewInfo> const& parent);

  // ViewInterface ///////////////////////////////////////////////////////
  virtual void invalidate();
  virtual ProgressInterface::Ptr getProgressInterface();
  virtual void addSideWidget(std::string title, GtkWidget* w);
  virtual void removeSideWidget(GtkWidget* w);
  virtual void addToToolbar(GtkToolItem* ti);
  virtual void removeFromToolbar(GtkToolItem* ti);
};

class TransparentOverlayViewInfo : virtual public Scroom::Utils::Base
{
  public:
  typedef boost::shared_ptr<TransparentOverlayViewInfo> Ptr;

  private:
  typedef std::map<PresentationInterface::Ptr, ChildView::Ptr> ChildMap;

  private:
  ViewInterface::Ptr parentView;
  ChildMap childViews;
  Scroom::Utils::ProgressInterfaceMultiplexer::Ptr progressInterfaceMultiplexer;
  std::vector<GtkWidget*> buttons;
  std::vector<PresentationInterface::Ptr> children;
  SizeDeterminer::Ptr sizeDeterminer;

  private:
  TransparentOverlayViewInfo(const ViewInterface::WeakPtr& vi,
                             SizeDeterminer::Ptr const& sizeDeterminer);
  void createToggleToolButton(PresentationInterface::Ptr const& p);

  public:
  static Ptr create(const ViewInterface::WeakPtr& vi, SizeDeterminer::Ptr const& sizeDeterminer);
  void addChildren(const std::list<PresentationInterface::Ptr>& children);
  void addChild(const PresentationInterface::Ptr& child);

  void close();

  void redraw(cairo_t* cr, Scroom::Utils::Rectangle<double> const& presentationArea, int zoom);

  void invalidate()
  {
    parentView->invalidate();
  }
  ProgressInterface::Ptr getProgressInterface()
  {
    return progressInterfaceMultiplexer->createProgressInterface();
  }

  // Helpers
  void toggled(GtkToggleButton* button);
};
