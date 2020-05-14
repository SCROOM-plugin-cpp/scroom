/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#pragma once

#include <gtk/gtk.h>

#include <boost/function.hpp>
#include <vector>

extern std::vector<boost::function<bool()> > functions;

GtkWidget* create_window();
void init();
void invalidate();
