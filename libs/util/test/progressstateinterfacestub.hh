/*
 * Scroom - Generic viewer for 2D data
 * Copyright (C) 2009-2017 Kees-Jan Dijkzeul
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License, version 2, as published by the Free Software Foundation.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef PROGRESSSTATEINTERFACESTUB_HH
#define PROGRESSSTATEINTERFACESTUB_HH

#include <scroom/progressinterfacehelpers.hh>

class ProgressStateInterfaceStub : public Scroom::Utils::ProgressStateInterface
{
public:
  typedef boost::shared_ptr<ProgressStateInterfaceStub> Ptr;

public:
  State state;
  double progress;
  
public:
  static Ptr create();
  virtual ~ProgressStateInterfaceStub() {}

private:
  ProgressStateInterfaceStub();

public:
  // ProgressStateInterface
  virtual void setProgress(State s, double d);
};




#endif
