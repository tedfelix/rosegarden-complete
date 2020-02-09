
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EVENTINSERTIONCOMMAND_H
#define RG_EVENTINSERTIONCOMMAND_H

#include "document/BasicCommand.h"

#include <QCoreApplication>




namespace Rosegarden
{

class Segment;
class Event;


class EventInsertionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::EventInsertionCommand)

public:
    /**
     * NOTE: This class takes ownership of event and will delete
     *       it upon destruction.
     */
    EventInsertionCommand(Segment &segment, Event *event);

    ~EventInsertionCommand() override;

    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }
    
protected:
    void modifySegment() override;

    Event *m_event;
    Event *m_lastInsertedEvent; // an alias for another event
};



}

#endif
