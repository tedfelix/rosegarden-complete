/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TempoView]"
#define RG_NO_DEBUG_PRINT

#include "TempoView.h"

#include "TempoListItem.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"
#include "commands/segment/RemoveTempoChangeCommand.h"
#include "commands/segment/RemoveTimeSignatureCommand.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "misc/ConfigGroups.h"
#include "gui/dialogs/TimeSignatureDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/general/EditTempoController.h"
#include "misc/PreferenceInt.h"

#include <QAction>
#include <QSettings>
#include <QTreeWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QDialog>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QList>
#include <QDesktopServices>


namespace
{

    Rosegarden::PreferenceInt a_timeMode(
            Rosegarden::TempoViewConfigGroup, "timemode", 0);

}


namespace Rosegarden
{


TempoView::TempoView(
        EditTempoController *editTempoController,
        timeT openTime) :
    EditViewBase(std::vector<Segment *>()),  // ??? default ctor?
    m_editTempoController(editTempoController)
{

    m_ignoreUpdates = true;

    slotUpdateWindowTitle(false);

    setStatusBar(new QStatusBar(this));
    // ??? inline this.
    initStatusBar();

    m_doc->getComposition().addObserver(this);

    // Connect for changes so we can update the list.
    connect(RosegardenDocument::currentDocument,
                &RosegardenDocument::documentModified,
            this, &TempoView::slotDocumentModified);
    // ??? slotDocumentModified() is already connected to this.  Combine.
    connect(m_doc, &RosegardenDocument::documentModified,
            this, &TempoView::slotUpdateWindowTitle);

    setupActions();

    // Create frame and layout.
    m_frame = new QFrame(this);
    m_frame->setMinimumSize(500, 300);
    m_frame->setMaximumSize(2200, 1400);
    // ??? QGridLayout is overkill.  This is a QHBoxLayout.
    m_mainLayout = new QGridLayout(m_frame);
    m_frame->setLayout(m_mainLayout);
    setCentralWidget(m_frame);

    // Filter Group Box
    m_filterGroup = new QGroupBox(tr("Filter"), m_frame);
    m_mainLayout->addWidget(m_filterGroup, 0, 0);
    QVBoxLayout *filterGroupLayout = new QVBoxLayout;
    m_filterGroup->setLayout(filterGroupLayout);

    // Tempo
    m_tempoCheckBox = new QCheckBox(tr("Tempo"), m_filterGroup);
    filterGroupLayout->addWidget(m_tempoCheckBox, 50, Qt::AlignTop);

    // Time Signature
    m_timeSigCheckBox = new QCheckBox(tr("Time Signature"), m_filterGroup);
    filterGroupLayout->addWidget(m_timeSigCheckBox, 50, Qt::AlignTop);

    // hard coded spacers are evil, but I can't find any other way to fix this
    // ??? Make a third row with a spacer and give it a stretch factor.
    //     That's the usual way to take up extra space.
    filterGroupLayout->addSpacing(200);

    readOptions();
    updateFilterCheckBoxes();

    // ??? Use clicked() instead of stateChanged().  Then move this up with
    //     the code setting up the widget.
    connect(m_tempoCheckBox, &QCheckBox::stateChanged,
            this, &TempoView::slotModifyFilter);
    // ??? Use clicked() instead of stateChanged().  Then move this up with
    //     the code setting up the widget.
    connect(m_timeSigCheckBox, &QCheckBox::stateChanged,
            this, &TempoView::slotModifyFilter);

    // Tempo/Time Signature List
    m_list = new QTreeWidget(m_frame);
    m_mainLayout->addWidget(m_list, 0, 1);
    m_list->setAllColumnsShowFocus(true);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QStringList headers;
    headers << tr("Time  ") <<
               tr("Type  ") <<
               tr("Value  ") <<
               tr("Properties  ");
    m_list->setColumnCount(headers.size());
    m_list->setHeaderLabels(headers);
    connect(m_list, &QTreeWidget::itemDoubleClicked,
            this, &TempoView::slotPopupEditor);
    // Update the list.
    applyLayout();
    makeInitialSelection(openTime);

    m_ignoreUpdates = false;
}

TempoView::~TempoView()
{
    QSettings settings;
    settings.beginGroup(TempoViewConfigGroup);
    settings.setValue("filter", m_filter);
    settings.endGroup();

    // We use m_doc instead of RosegardenDocument::currentDocument to
    // make sure that we disconnect from the old document when the
    // documents are switching.
    if (m_doc  &&  !isCompositionDeleted())
        m_doc->getComposition().removeObserver(this);
}

void
TempoView::closeEvent(QCloseEvent *e)
{
    // Let RosegardenMainWindow know we are going down.
    emit closing();

    EditViewBase::closeEvent(e);
}

void
TempoView::tempoChanged(const Composition *comp)
{
    if (m_ignoreUpdates)
        return;

    // Not the current Composition?  Bail.
    // ??? I suspect this can never happen.  Can we safely get rid of this?
    if (comp != &RosegardenDocument::currentDocument->getComposition())
        return;

    applyLayout();
}

void
TempoView::timeSignatureChanged(const Composition *comp)
{
    if (m_ignoreUpdates)
        return;

    // Not the current Composition?  Bail.
    // ??? I suspect this can never happen.  Can we safely get rid of this?
    if (comp != &RosegardenDocument::currentDocument->getComposition())
        return;

    applyLayout();
}

bool
TempoView::applyLayout()
{
    // Recreate list.

    m_list->clear();

    Composition *comp = &RosegardenDocument::currentDocument->getComposition();

    // Time Signatures
    if (m_filter & TimeSignature) {

        for (int timeSignatureIndex = 0;
             timeSignatureIndex < comp->getTimeSignatureCount();
             ++timeSignatureIndex) {

            std::pair<timeT, Rosegarden::TimeSignature> sig =
                    comp->getTimeSignatureChange(timeSignatureIndex);

            QString properties;
            if (sig.second.isHidden()) {
                if (sig.second.isCommon())
                    properties = tr("Common, hidden");
                else
                    properties = tr("Hidden");
            } else {
                if (sig.second.isCommon())
                    properties = tr("Common");
            }

            QString timeString = makeTimeString(sig.first, a_timeMode.get());

            QStringList labels;
            labels << timeString <<
                      tr("Time Signature   ") <<
                      QString("%1/%2   ").
                              arg(sig.second.getNumerator()).
                              arg(sig.second.getDenominator()) <<
                      properties;

            // Add to the list.
            // ??? This doesn't look like an add.  Rearrange the code to
            //     make it more "add" like.  Actually, QTreeWidgetItem takes
            //     the widget as the first parameter.  Maybe move parent to
            //     the first parameter.
            new TempoListItem(
                    comp,  // composition
                    TempoListItem::TimeSignature,  // type
                    sig.first,  // time
                    timeSignatureIndex,  // index
                    m_list,  // parent
                    labels);
        }
    }

    // Tempos
    if (m_filter & Tempo) {

        for (int tempoIndex = 0;
             tempoIndex < comp->getTempoChangeCount();
             ++tempoIndex) {

            const std::pair<timeT, tempoT> tempoPair =
                    comp->getTempoChange(tempoIndex);
            const timeT time = tempoPair.first;
            const tempoT &tempo = tempoPair.second;

            QString desc;

            const float qpm = comp->getTempoQpm(tempo);
            const int qpmUnits = int(qpm + 0.001);
            const int qpmTenths = int((qpm - qpmUnits) * 10 + 0.001);
            const int qpmHundredths =
                    int((qpm - qpmUnits - qpmTenths / 10.0) * 100 + 0.001);

            const Rosegarden::TimeSignature sig =
                    comp->getTimeSignatureAt(time);

            if (sig.getBeatDuration() ==
                    Note(Note::Crotchet).getDuration()) {
                desc = tr("%1.%2%3")
                       .arg(qpmUnits).arg(qpmTenths).arg(qpmHundredths);
            } else {
                const float bpm = (qpm *
                             Note(Note::Crotchet).getDuration()) /
                            sig.getBeatDuration();
                const int bpmUnits = int(bpm + 0.001);
                const int bpmTenths = int((bpm - bpmUnits) * 10 + 0.001);
                const int bpmHundredths = int((bpm - bpmUnits - bpmTenths / 10.0) * 100 + 0.001);

                desc = tr("%1.%2%3 qpm (%4.%5%6 bpm)   ")
                       .arg(qpmUnits).arg(qpmTenths).arg(qpmHundredths)
                       .arg(bpmUnits).arg(bpmTenths).arg(bpmHundredths);
            }

            const QString timeString = makeTimeString(time, a_timeMode.get());

            QStringList labels;
            labels << timeString << tr("Tempo   ") << desc;

            // Add to the list.
            // ??? This doesn't look like an add.  Rearrange the code to
            //     make it more "add" like.  Actually, QTreeWidgetItem takes
            //     the widget as the first parameter.  Maybe move parent to
            //     the first parameter.
            new TempoListItem(
                    comp,
                    TempoListItem::Tempo,
                    time,
                    tempoIndex,
                    m_list,
                    labels);
        }
    }

    if (m_list->topLevelItemCount() == 0) {
        // ??? This is impossible to see due to the size of the first column.
        //     Just leave the list empty.
        //new QTreeWidgetItem(m_list, QStringList() << tr("<nothing at this filter level>"));
        m_list->setSelectionMode(QTreeWidget::NoSelection);
        leaveActionState("have_selection");
    } else {
        m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // If no selection then select the first event
        if (m_listSelection.size() == 0)
            m_listSelection.push_back(0);
        enterActionState("have_selection");
    }

    // Set a selection from a range of indexes
    //
    std::vector<int>::iterator sIt = m_listSelection.begin();

    for (; sIt != m_listSelection.end(); ++sIt) {
        int index = *sIt;

        while (index > 0 && !m_list->topLevelItem(index))
            index--;

//         m_list->setSelected(m_list->topLevelItem(index), true);
        m_list->topLevelItem(index)->setSelected(true);
//         m_list->setCurrentIndex(m_list->topLevelItem(index));
        m_list->setCurrentItem( m_list->topLevelItem(index) );

        // ensure visible
//         m_list->ensureItemVisible(m_list->topLevelItem(index));
        m_list->scrollToItem(m_list->topLevelItem(index));
    }

    m_listSelection.clear();

    return true;
}

void
TempoView::makeInitialSelection(timeT time)
{
    m_listSelection.clear();

    TempoListItem *goodItem = nullptr;
    int goodItemNo = 0;

    for (int i = 0; (m_list->topLevelItem(i) != nullptr); ++i) {

        TempoListItem *item =
                dynamic_cast<TempoListItem *>(m_list->topLevelItem(i));

        // Nothing found, try the next.  This might end the loop.
        // ??? Any way to get the item count?
        if (!item)
            continue;

        item->setSelected(false);

        if (item->getTime() > time)
            break;
        goodItem = item;
        goodItemNo = i;
    }

    if (goodItem) {
        m_listSelection.push_back(goodItemNo);
//         m_list->setSelected(goodItem, true);
        goodItem->setSelected(true);
//         m_list->ensureItemVisible(goodItem);
        m_list->scrollToItem(goodItem);
    }
}

Segment *
TempoView::getCurrentSegment()
{
    // ??? It's always empty.  Just return nullptr.

    if (m_segments.empty())
        return nullptr;
    else
        return *m_segments.begin();
}

QString
TempoView::makeTimeString(timeT time, int timeMode)
{
    // ??? Need an enum for this.
    switch (timeMode) {

    case 0:  // musical time
        {
            int bar, beat, fraction, remainder;
            RosegardenDocument::currentDocument->getComposition().getMusicalTimeForAbsoluteTime
            (time, bar, beat, fraction, remainder);
            ++bar;
            return QString("%1%2%3-%4%5-%6%7-%8%9   ")
                   .arg(bar / 100)
                   .arg((bar % 100) / 10)
                   .arg(bar % 10)
                   .arg(beat / 10)
                   .arg(beat % 10)
                   .arg(fraction / 10)
                   .arg(fraction % 10)
                   .arg(remainder / 10)
                   .arg(remainder % 10);
        }

    case 1:  // real time
        {
            RealTime rt =
                RosegardenDocument::currentDocument->getComposition().getElapsedRealTime(time);
            //    return QString("%1   ").arg(rt.toString().c_str());
            return QString("%1   ").arg(rt.toText().c_str());
        }

    case 2:  // raw time
        return QString("%1   ").arg(time);

    default:
        return "---";
    }
}

void
TempoView::slotEditDelete()
{
    QList<QTreeWidgetItem*> selection = m_list->selectedItems();

    if (selection.count() == 0) return ;

    RG_DEBUG << "TempoView::slotEditDelete - deleting "
    << selection.count() << " items";

    m_ignoreUpdates = true;
    bool haveSomething = false;

    // We want the Remove commands to be in reverse order, because
    // removing one item by index will affect the indices of
    // subsequent items.  So we'll stack them onto here and then pull
    // them off again.
    std::vector<Command *> commands;

    // Create a map of each selected item in index order.
    std::map<int, TempoListItem*> itemMap;
    foreach(auto it, selection) {
        TempoListItem *item = dynamic_cast<TempoListItem*>(it);
        if (!item) continue;
        int index = item->getIndex();
        itemMap[index] = item;
    }

    if (itemMap.empty()) return;

    // For each selected item in index order
    for (auto iter = itemMap.begin(); iter != itemMap.end(); ++iter) {
        int index = (*iter).first;
        RG_DEBUG << "deleting item with index" << index;
        TempoListItem* item = (*iter).second;

        // Add the appropriate command to the "commands" list.

        if (item->getType() == TempoListItem::TimeSignature) {
            commands.push_back(new RemoveTimeSignatureCommand
                               (item->getComposition(),
                                item->getIndex()));
            haveSomething = true;
        } else {
            commands.push_back(new RemoveTempoChangeCommand
                               (item->getComposition(),
                                item->getIndex()));
            haveSomething = true;
        }
    }

    if (haveSomething) {
        MacroCommand *command = new MacroCommand
                                 (tr("Delete Tempo or Time Signature"));
        // For each command in reverse order which also happens to be
        // reverse index order, add the remove command to the macro.
        for (std::vector<Command *>::iterator i = commands.end();
             i != commands.begin();
             /* decrement is inside */) {
            command->addCommand(*--i);
        }
        CommandHistory::getInstance()->addCommand(command);
    }

    applyLayout();
    m_ignoreUpdates = false;
}

void
TempoView::slotAddTempoChange()
{
    timeT insertTime = 0;
    QList<QTreeWidgetItem*> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.first());
        if (item)
            insertTime = item->getTime();
    }

    m_editTempoController->editTempo(this, insertTime, true /* timeEditable */);
}

void
TempoView::slotAddTimeSignatureChange()
{
    timeT insertTime = 0;
    QList<QTreeWidgetItem*> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.first());
        if (item)
            insertTime = item->getTime();
    }

    Composition &composition(RosegardenDocument::currentDocument->getComposition());
    Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(insertTime);

    TimeSignatureDialog dialog(this, &composition, insertTime, sig, true);

    if (dialog.exec() == QDialog::Accepted) {

        insertTime = dialog.getTime();

        if (dialog.shouldNormalizeRests()) {
            CommandHistory::getInstance()->addCommand(
                    new AddTimeSignatureAndNormalizeCommand(
                            &composition,
                            insertTime,
                            dialog.getTimeSignature()));
        } else {
            CommandHistory::getInstance()->addCommand(
                    new AddTimeSignatureCommand(
                            &composition,
                            insertTime,
                            dialog.getTimeSignature()));
        }
    }
}

void
TempoView::slotEditItem()
{
    RG_DEBUG << "TempoView::slotEdit";

    QList<QTreeWidgetItem*> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.first());
        if (item)
            slotPopupEditor(item);
    }
}

void
TempoView::slotSelectAll()
{
    m_listSelection.clear();
    for (int i = 0; m_list->topLevelItem(i); ++i) {
        m_listSelection.push_back(i);
//         m_list->setSelected(m_list->topLevelItem(i), true);
        m_list->topLevelItem(i)->setSelected(true);
    }
}

void
TempoView::slotClearSelection()
{
    m_listSelection.clear();
    for (int i = 0; m_list->topLevelItem(i); ++i) {
//         m_list->setSelected(m_list->topLevelItem(i), false);
        m_list->topLevelItem(i)->setSelected(false);
    }
}

void
TempoView::setupActions()
{
    setupBaseActions(false);

    createAction("insert_tempo", SLOT(slotAddTempoChange()));
    createAction("insert_timesig", SLOT(slotAddTimeSignatureChange()));
    createAction("delete", SLOT(slotEditDelete()));
    createAction("edit", SLOT(slotEditItem()));
    createAction("select_all", SLOT(slotSelectAll()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("tempo_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    QAction *a;
    a = createAction("time_musical", SLOT(slotViewMusicalTimes()));
    a->setCheckable(true);
    if (a_timeMode.get() == 0)
        a->setChecked(true);

    a = createAction("time_real", SLOT(slotViewRealTimes()));
    a->setCheckable(true);
    if (a_timeMode.get() == 1)
        a->setChecked(true);

    a = createAction("time_raw", SLOT(slotViewRawTimes()));
    a->setCheckable(true);
    if (a_timeMode.get() == 2)
        a->setChecked(true);

    createMenusAndToolbars("tempoview.rc");
}

void
TempoView::initStatusBar()
{
    QStatusBar* sb = statusBar();
    sb->showMessage(QString());
}

void
TempoView::readOptions()
{
    QSettings settings;
    settings.beginGroup(TempoViewConfigGroup);
    m_filter = settings.value("filter", m_filter).toInt();
    settings.endGroup();
}

void
TempoView::slotModifyFilter(int)
{
    m_filter = 0;

    if (m_tempoCheckBox->isChecked())
        m_filter |= Tempo;

    if (m_timeSigCheckBox->isChecked())
        m_filter |= TimeSignature;

    applyLayout();
}

void
TempoView::updateFilterCheckBoxes()
{
    if (m_filter & Tempo)
        m_tempoCheckBox->setChecked(true);
    else
        m_tempoCheckBox->setChecked(false);

    if (m_filter & TimeSignature)
        m_timeSigCheckBox->setChecked(true);
    else
        m_timeSigCheckBox->setChecked(false);
}

void
TempoView::slotViewMusicalTimes()
{
    findAction("time_musical")->setChecked(true);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(false);

    a_timeMode.set(0);

    applyLayout();
}

void
TempoView::slotViewRealTimes()
{
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(true);
    findAction("time_raw")->setChecked(false);

    a_timeMode.set(1);

    applyLayout();
}

void
TempoView::slotViewRawTimes()
{
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(true);

    a_timeMode.set(2);

    applyLayout();
}

void
TempoView::slotPopupEditor(QTreeWidgetItem *qitem, int)
{
    TempoListItem *item = dynamic_cast<TempoListItem *>(qitem);
    if (!item)
        return ;

    timeT time = item->getTime();

    switch (item->getType()) {

    case TempoListItem::Tempo:
    {
        m_editTempoController->editTempo(this, time, true /* timeEditable */);
        break;
    }

    case TempoListItem::TimeSignature:
    {
        Composition &composition(RosegardenDocument::currentDocument->getComposition());
        Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(time);

        TimeSignatureDialog dialog(this, &composition, time, sig, true);

        if (dialog.exec() == QDialog::Accepted) {

            time = dialog.getTime();

            if (dialog.shouldNormalizeRests()) {
                CommandHistory::getInstance()->addCommand(
                        new AddTimeSignatureAndNormalizeCommand(
                                &composition,
                                time,
                                dialog.getTimeSignature()));
            } else {
                CommandHistory::getInstance()->addCommand(
                        new AddTimeSignatureCommand(
                                &composition,
                                time,
                                dialog.getTimeSignature()));
            }
        }

        break;
    }

    default:
        break;
    }
}

void
TempoView::slotUpdateWindowTitle(bool)
{
    setWindowTitle(tr("%1 - Tempo and Time Signature Editor")
                .arg(RosegardenDocument::currentDocument->getTitle()));
}

void
TempoView::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:tempoView-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:tempoView-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
TempoView::slotHelpAbout()
{
    new AboutDialog(this);
}

void
TempoView::slotDocumentModified(bool /*modified*/)
{
    applyLayout();
}


}
