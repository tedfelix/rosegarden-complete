/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ShortcutDialog.h"

#include "gui/general/ActionData.h"
#include "misc/ConfigGroups.h"

#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QSettings>
#include <QItemSelection>

namespace Rosegarden
{

ShortcutDialog::ShortcutDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Shortcuts"));

    m_model = new QStandardItemModel(0, 5, this);

    ActionData* adata = ActionData::getInstance();
    adata->fillModel(m_model);
    
    m_proxyModel = new QSortFilterProxyModel;
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);
    
    m_proxyView = new QTreeView;
    m_proxyView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_proxyView->setRootIsDecorated(false);
    m_proxyView->setAlternatingRowColors(true);
    m_proxyView->setModel(m_proxyModel);
    m_proxyView->setSortingEnabled(true);
    //m_proxyView->hideColumn(0);
    
    connect(m_proxyView->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,
                                    const QItemSelection&)),
            this,
            SLOT(selectionChanged(const QItemSelection&,
                                  const QItemSelection&)));
    
    m_filterPatternLineEdit = new QLineEdit;
    m_filterPatternLabel = new QLabel(tr("Filter pattern:"));
    
    connect(m_filterPatternLineEdit, SIGNAL(textChanged(const QString&)),
            this, SLOT(filterChanged()));
    
    QGridLayout *proxyLayout = new QGridLayout;
    proxyLayout->addWidget(m_filterPatternLabel, 0, 0);
    proxyLayout->addWidget(m_filterPatternLineEdit, 0, 1, 1, 3);
    proxyLayout->addWidget(m_proxyView, 1, 0, 1, 4);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    
    setLayout(mainLayout);
    setWindowTitle(tr("Shortcuts"));
    
    m_proxyView->sortByColumn(0, Qt::AscendingOrder);

    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    this->restoreGeometry(settings.value("Shortcut_Dialog").toByteArray());
    QStringList columnWidths =
        settings.value("Shortcut_Table_Widths").toStringList();
    settings.endGroup();

    // set column widths (except for last one)
    for (int i = 0; i < columnWidths.size() - 1; i++) {
        m_proxyView->setColumnWidth(i, columnWidths[i].toInt());
    }

    QHBoxLayout *hlayout = new QHBoxLayout;
    m_clabel = new QLabel;
    m_alabel = new QLabel;
    m_ilabel = new QLabel;
    hlayout->addWidget(m_clabel);
    hlayout->addWidget(m_alabel);
    hlayout->addWidget(m_ilabel);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    
    mainLayout->addLayout(hlayout);
    mainLayout->addWidget(line);
    mainLayout->addLayout(proxyLayout);
}

ShortcutDialog::~ShortcutDialog()
{
    QStringList columnWidths;
    // save column widths (except for last one)
    for (int i = 0; i < m_model->columnCount() - 1; i++) {
        columnWidths << QString::number(m_proxyView->columnWidth(i));
    }
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Shortcut_Dialog", this->saveGeometry());
    settings.setValue("Shortcut_Table_Widths", columnWidths);
    settings.endGroup();
}

void ShortcutDialog::filterChanged()
{
    m_proxyModel->setFilterFixedString(m_filterPatternLineEdit->text());
}

void ShortcutDialog::selectionChanged(const QItemSelection& selected,
                                      const QItemSelection&)
{
    qDebug() << "selection changed" << selected;
    QModelIndexList indexes = selected.indexes();
    if (indexes.empty()) return;
    QModelIndex index = indexes.first();
    int row = index.row();
    int column = index.column();
    qDebug() << "row" << row << column << "selected";
    QModelIndex i0 = m_proxyModel->index(row, 0);
    m_editKey = m_proxyModel->data(i0, Qt::DisplayRole).toString();
    qDebug() << "editing key" << m_editKey;
    QModelIndex i1 = m_proxyModel->index(row, 1);
    QString ctext = m_proxyModel->data(i1, Qt::DisplayRole).toString();
    m_clabel->setText(ctext);
    QModelIndex i2 = m_proxyModel->index(row, 2);
    QString atext = m_proxyModel->data(i2, Qt::DisplayRole).toString();
    m_alabel->setText(atext);
    QModelIndex i3 = m_proxyModel->index(row, 3);
    QVariant imagev = m_proxyModel->data(i3, Qt::DecorationRole);
    QIcon icon = imagev.value<QIcon>();
    if (! icon.isNull()) {
        QPixmap pixmap =
            icon.pixmap(icon.availableSizes().first());
        int w = m_ilabel->width();
        int h = m_ilabel->height();
        m_ilabel->setPixmap(pixmap.scaled(w, h, Qt::KeepAspectRatio));
    }
}

}
