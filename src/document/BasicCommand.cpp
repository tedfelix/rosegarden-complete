/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "BasicCommand.h"

#include "base/Segment.h"
#include <qstring.h>

namespace Rosegarden
{
    
BasicCommand::BasicCommand(const QString &name, Segment &segment,
               timeT start, timeT end, bool bruteForceRedo) :
    KNamedCommand(name),
    m_startTime(calculateStartTime(start, segment)),
    m_endTime(calculateEndTime(end, segment)),
    m_segment(segment),
    m_savedEvents(segment.getType(), m_startTime),
    m_doBruteForceRedo(false),
    m_redoEvents(0)
{
    if (m_endTime == m_startTime) ++m_endTime;

    if (bruteForceRedo) {
        m_redoEvents = new Segment(segment.getType(), m_startTime);
    }
}

BasicCommand::~BasicCommand()
{
    m_savedEvents.clear();
    if (m_redoEvents) m_redoEvents->clear();
    delete m_redoEvents;
}

timeT
BasicCommand::calculateStartTime(timeT given, Segment &segment)
{
    timeT actual = given;
    Segment::iterator i = segment.findTime(given);

    while (i != segment.end() && (*i)->getAbsoluteTime() == given) {
    timeT notation = (*i)->getNotationAbsoluteTime();
    if (notation < given) actual = notation;
    ++i;
    }

    return actual;
}

timeT
BasicCommand::calculateEndTime(timeT given, Segment &segment)
{
    timeT actual = given;
    Segment::iterator i = segment.findTime(given);

    while (i != segment.end() && (*i)->getAbsoluteTime() == given) {
    timeT notation = (*i)->getNotationAbsoluteTime();
    if (notation > given) actual = notation;
    ++i;
    }

    return actual;
}

Rosegarden::Segment& BasicCommand::getSegment()
{
    return m_segment;
}

Rosegarden::timeT BasicCommand::getRelayoutEndTime()
{
    return getEndTime();
}

void
BasicCommand::beginExecute()
{
    copyTo(&m_savedEvents);
}

void
BasicCommand::execute()
{
    beginExecute();

    if (!m_doBruteForceRedo) {

    modifySegment();

    } else {
    copyFrom(m_redoEvents);
    }

    m_segment.updateRefreshStatuses(getStartTime(), getRelayoutEndTime());
    RG_DEBUG << "BasicCommand(" << name() << "): updated refresh statuses "
         << getStartTime() << " -> " << getRelayoutEndTime() << endl;
}

void
BasicCommand::unexecute()
{
    if (m_redoEvents) {
    copyTo(m_redoEvents);
    m_doBruteForceRedo = true;
    }

    copyFrom(&m_savedEvents);

    m_segment.updateRefreshStatuses(getStartTime(), getRelayoutEndTime());
}
    
void
BasicCommand::copyTo(Rosegarden::Segment *events)
{
    RG_DEBUG << "BasicCommand(" << name() << ")::copyTo: " << &m_segment << " to "
         << events << ", range (" 
         << m_startTime << "," << m_endTime
         << ")" << endl;

    Segment::iterator from = m_segment.findTime(m_startTime);
    Segment::iterator to   = m_segment.findTime(m_endTime);

    for (Segment::iterator i = from; i != m_segment.end() && i != to; ++i) {
//  RG_DEBUG << "Found event of type " << (*i)->getType() << " and duration " << (*i)->getDuration() << endl;
    events->insert(new Event(**i));
    }
}
   
void
BasicCommand::copyFrom(Rosegarden::Segment *events)
{
    RG_DEBUG << "BasicCommand(" << name() << ")::copyFrom: " << events << " to "
         << &m_segment << ", range (" 
         << m_startTime << "," << m_endTime
         << ")" << endl;

    m_segment.erase(m_segment.findTime(m_startTime),
            m_segment.findTime(m_endTime));

    for (Segment::iterator i = events->begin(); i != events->end(); ++i) {
//  RG_DEBUG << "Found event of type " << (*i)->getType() << " and duration " << (*i)->getDuration() << endl;
    m_segment.insert(new Event(**i));
    }

    events->clear();
}
    
}
