/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <list>
#include <map>
#include <scroom/presentationinterface.hh>
#include <scroom/resizablepresentationinterface.hh>
#include <set>

class SizeDeterminer
{
  public:
  typedef boost::shared_ptr<SizeDeterminer> Ptr;

  private:
  class PresentationData
  {
    public:
    ResizablePresentationInterface::Ptr const resizablePresentationInterface;
    std::set<ViewInterface::WeakPtr> views;

    public:
    PresentationData();  // Don't use
    explicit PresentationData(
        ResizablePresentationInterface::Ptr const& resizablePresentationInterface);
  };

  private:
  std::list<PresentationInterface::Ptr> presentations;
  std::map<PresentationInterface::Ptr, PresentationData> resizablePresentationData;

  private:
  SizeDeterminer();
  void sendUpdates();

  public:
  static Ptr create();
  void add(PresentationInterface::Ptr const& p);
  Scroom::Utils::Rectangle<double> getRect() const;

  void open(PresentationInterface::Ptr const& p, ViewInterface::WeakPtr const& vi);
  void close(PresentationInterface::Ptr const& p, ViewInterface::WeakPtr const& vi);
};
