
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_PROPERTYCONTROLRULER_H_
#define _RG_PROPERTYCONTROLRULER_H_

//#include <Q3Canvas>
//#include <Q3CanvasLine>
#include "base/PropertyName.h"
#include "ControlRuler.h"
#include <QString>
#include "base/Event.h"
#include "base/Segment.h"


class QWidget;
class QMouseEvent;
class QContextMenuEvent;
//class Q3CanvasLine;
//class Q3Canvas;


namespace Rosegarden
{

class ViewElement;
//class MatrixScene;
class ViewSegment;
class Segment;
class RulerScale;
//class EditViewBase;


/**
 * PropertyControlRuler : edit a property on events on a staff (only
 * events with a ViewElement attached, mostly notes)
 */
class PropertyControlRuler :  public ControlRuler, public ViewSegmentObserver
{
public:
    PropertyControlRuler(PropertyName propertyName,
                        ViewSegment*,
                        RulerScale*,
                        QWidget* parent=0, const char* name=0);

    virtual ~PropertyControlRuler();

    virtual void update();
    
    virtual void paintEvent(QPaintEvent *);

    virtual QString getName();

    const PropertyName& getPropertyName()     { return m_propertyName; }

    // Allow something external to reset the selection of Events
    // that this ruler is displaying
    //
    virtual void setViewSegment(ViewSegment *);

    // ViewSegmentObserver interface
    virtual void elementAdded(const ViewSegment *, ViewElement*);
//    virtual void eventAdded(const Segment *, Event *);
    virtual void elementRemoved(const ViewSegment *, ViewElement*);
//    virtual void eventRemoved(const Segment *, Event *);
    virtual void viewSegmentDeleted(const ViewSegment *);
//    virtual void segmentDeleted(const Segment *);

//    virtual void startPropertyLine();
    virtual void selectAllProperties();

    /// SegmentObserver interface
    virtual void endMarkerTimeChanged(const Segment *, bool shorten);

    void updateSelection(std::vector<ViewElement*>*);
    void updateSelectedItems();

public slots:
    void slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);
    virtual void slotSetTool(const QString &);

protected:
//    void addControlItem(Event *);
    void addControlItem(ViewElement *);

    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void contextMenuEvent(QContextMenuEvent*);

    //void drawPropertyLine(timeT startTime,
                          //timeT endTime,
                          //int startValue,
                          //int endValue);

    virtual void init();
//    virtual void drawBackground();
//    virtual void computeSegmentOffset();

    //--------------- Data members ---------------------------------

    PropertyName m_propertyName;
//    Segment *m_segment;

//    Q3CanvasLine *m_propertyLine;

//    bool m_propertyLineShowing;
//    int m_propertyLineX;
//    int m_propertyLineY;
};



}

#endif
