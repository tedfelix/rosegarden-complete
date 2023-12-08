/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2023 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#ifndef RG_PLUGINPORTCONNECTION_H
#define RG_PLUGINPORTCONNECTION_H

#include <QString>
#include <list>

namespace Rosegarden
{

class PluginPortConnection
{
 public:

    struct Connection
    {
        bool isOutput;
        bool isAudio;
        QString pluginPort;
        QString portConnection;
    };

    typedef std::list<Connection> ConnectionList;
};

}

#endif // RG_PLUGINPORTCONNECTION_H
