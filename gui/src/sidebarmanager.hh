/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#pragma once

#include <gtk/gtk.h>

#include <map>
#include <string>

class SidebarManager
{
  private:
  GtkWidget* panelWindow;
  GtkBox* panel;

  std::map<GtkWidget*, GtkWidget*> widgets;

  public:
  SidebarManager();
  void setWidgets(GtkWidget* panelWindow, GtkBox* panel);

  void addSideWidget(std::string title, GtkWidget* w);
  void removeSideWidget(GtkWidget* w);
};
