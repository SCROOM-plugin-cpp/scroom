/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2020 Kees-Jan Dijkzeul
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#pragma once

#include <gmodule.h>

#include <boost/shared_ptr.hpp>

class PluginInformationInterface;

extern "C"
{
  typedef boost::shared_ptr<PluginInformationInterface> (*PluginFunc)();

  G_MODULE_IMPORT boost::shared_ptr<PluginInformationInterface> getPluginInformation();
}
