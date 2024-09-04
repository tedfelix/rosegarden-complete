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

#define RG_MODULE_STRING "[BankEditorDialog]"
//#define RG_NO_DEBUG_PRINT

#include "BankEditorDialog.h"

#include "MidiBankTreeWidgetItem.h"
#include "MidiDeviceTreeWidgetItem.h"
#include "MidiKeyMapTreeWidgetItem.h"
#include "MidiKeyMappingEditor.h"
#include "MidiProgramsEditor.h"

#include "misc/Debug.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "commands/studio/ModifyDeviceCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/ExportDeviceDialog.h"
#include "gui/dialogs/ImportDeviceDialog.h"
#include "gui/dialogs/LibrarianDialog.h"
#include "gui/widgets/FileDialog.h"
#include "gui/general/ResourceFinder.h"
#include "gui/dialogs/AboutDialog.h"

#include <QComboBox>
#include <QTreeWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif


namespace Rosegarden
{


BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   RosegardenDocument *doc,
                                   DeviceId defaultDevice) :
    QMainWindow(parent),
    m_doc(doc),
    m_studio(&doc->getStudio())
{
    setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle(tr("Manage MIDI Banks and Programs"));

    // Main Frame
    QWidget *mainFrame = new QWidget(this);
    mainFrame->setContentsMargins(1, 1, 1, 1);
    setCentralWidget(mainFrame);
    // VBox layout holds most of the dialog at the top and the button box at
    // the bottom.
    // ??? Seems like a grid layout would be simpler.  Get rid of the
    //     command buttons then switch to a grid layout.
    QVBoxLayout *mainFrameLayout = new QVBoxLayout(mainFrame);
    mainFrameLayout->setContentsMargins(0, 0, 0, 0);
    mainFrameLayout->setSpacing(2);
    //mainFrame->setLayout(mainFrameLayout);

    // "Splitter" contains the left (tree) and right (editor) sides.
    // ??? Rename: editor?  Or just use a grid layout.
    QWidget *splitter = new QWidget;
    QHBoxLayout *splitterLayout = new QHBoxLayout(splitter);
    splitterLayout->setContentsMargins(0, 0, 0, 0);
    //splitter->setLayout(splitterLayout);

    // Top of the main vbox layout is the editor.
    mainFrameLayout->addWidget(splitter);

    // Editor Left Side.  The Tree and Command Buttons.

    QWidget *leftPart = new QWidget;
    QVBoxLayout *leftPartLayout = new QVBoxLayout;
    leftPartLayout->setContentsMargins(2, 2, 2, 2);
    leftPart->setLayout(leftPartLayout);
    splitterLayout->addWidget(leftPart);

    m_treeWidget = new QTreeWidget;
    leftPartLayout->addWidget(m_treeWidget);
    m_treeWidget->setMinimumWidth(500);
    m_treeWidget->setColumnCount(4);
    QStringList sl;
    sl << tr("Device and Banks")
       << tr("Type")
       << tr("MSB")
       << tr("LSB");
    m_treeWidget->setHeaderLabels(sl);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);    //qt4
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);    //qt4
    m_treeWidget->setSortingEnabled(true);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &BankEditorDialog::slotEdit);
    connect(m_treeWidget, &QTreeWidget::currentItemChanged,
            this, &BankEditorDialog::slotUpdateEditor);
    connect(m_treeWidget, &QTreeWidget::itemChanged,
            this, &BankEditorDialog::slotItemChanged);

    // Buttons

    // ??? Why do we have buttons when we have a menu?  Get all of this
    //     button functionality into the menu.  Get rid of all
    //     these buttons.  Then add a toolbar.

    QFrame *bankBox = new QFrame(leftPart);
    leftPartLayout->addWidget(bankBox);
    bankBox->setContentsMargins(1, 1, 1, 1);
    QGridLayout *gridLayout = new QGridLayout(bankBox);
    gridLayout->setSpacing(4);
    //bankBox->setLayout(gridLayout);

    m_addBank = new QPushButton(tr("Add Bank"), bankBox);
    m_addBank->setToolTip(tr("Add a Bank to the current device"));
    connect(m_addBank, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotAddBank);
    gridLayout->addWidget(m_addBank, 0, 0);

    m_addKeyMapping = new QPushButton(tr("Add Key Mapping"), bankBox);
    m_addKeyMapping->setToolTip(tr("Add a Percussion Key Mapping to the current device"));
    connect(m_addKeyMapping, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotAddKeyMapping);
    gridLayout->addWidget(m_addKeyMapping, 0, 1);

    m_delete = new QPushButton(tr("Delete"), bankBox);
    m_delete->setToolTip(tr("Delete the current Bank or Key Mapping"));
    connect(m_delete, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotDelete);
    gridLayout->addWidget(m_delete, 1, 0);

    m_deleteAll = new QPushButton(tr("Delete All"), bankBox);
    m_deleteAll->setToolTip(tr("Delete all Banks and Key Mappings from the current Device"));
    connect(m_deleteAll, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotDeleteAll);
    gridLayout->addWidget(m_deleteAll, 1, 1);

    m_import = new QPushButton(tr("Import..."), bankBox);
    m_import->setToolTip(tr("Import Bank and Program data from a Rosegarden file to the current Device"));
    connect(m_import, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotImport);
    gridLayout->addWidget(m_import, 2, 0);

    m_export = new QPushButton(tr("Export..."), bankBox);
    m_export->setToolTip(tr("Export all Device and Bank information to a Rosegarden format  interchange file"));
    connect(m_export, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotExport);
    gridLayout->addWidget(m_export, 2, 1);

    m_copy = new QPushButton(tr("Copy"), bankBox);
    m_copy->setToolTip(tr("Copy all Program names from current Bank or Keymap to clipboard"));
    connect(m_copy, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotCopy);
    gridLayout->addWidget(m_copy, 3, 0);

    m_paste = new QPushButton(tr("Paste"), bankBox);
    m_paste->setToolTip(tr("Paste Program names from clipboard to current Bank or Keymap"));
    connect(m_paste, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotPaste);
    gridLayout->addWidget(m_paste, 3, 1);

    // Editor Right Side.  The Bank and Key Map editors.

    m_rightSide = new QFrame;
    m_rightSide->setContentsMargins(8, 8, 8, 8);
    QVBoxLayout *rightSideLayout = new QVBoxLayout(m_rightSide);
    rightSideLayout->setContentsMargins(0, 0, 0, 0);
    rightSideLayout->setSpacing(6);
    //m_rightSide->setLayout(rightSideLayout);

    splitterLayout->addWidget(m_rightSide);

    // MIDI Programs Editor
    m_programEditor = new MidiProgramsEditor(this, m_rightSide);
    m_programEditor->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
    rightSideLayout->addWidget(m_programEditor);

    // MIDI Key Map Editor
    m_keyMappingEditor = new MidiKeyMappingEditor(this, m_rightSide);
    m_keyMappingEditor->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
    m_keyMappingEditor->hide();
    // ??? These aren't on top of each other.  Should we use QStackedLayout?
    //     That would be easier to understand.
    rightSideLayout->addWidget(m_keyMappingEditor);

    // Options
    m_optionBox = new QGroupBox(tr("Options"), m_rightSide);
    rightSideLayout->addWidget(m_optionBox);

    QHBoxLayout *variationBoxLayout = new QHBoxLayout(m_optionBox);
    variationBoxLayout->setContentsMargins(4, 4, 4, 4);

    // Variation Check Box
    m_variationCheckBox = new QCheckBox(tr("Show Variation list based on "), m_optionBox);
    connect(m_variationCheckBox, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotVariationToggled);
    variationBoxLayout->addWidget(m_variationCheckBox);

    // Variation Combo Box
    m_variationCombo = new QComboBox(m_optionBox);
    m_variationCombo->addItem(tr("LSB"));
    m_variationCombo->addItem(tr("MSB"));
    connect(m_variationCombo,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &BankEditorDialog::slotVariationChanged);
    variationBoxLayout->addWidget(m_variationCombo);


    // Button box.  Close button.

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Close);
    m_closeButton = btnBox->button(QDialogButtonBox::Close);
    // Bottom of the main vbox layout is the button box.
    mainFrameLayout->addWidget(btnBox);

    m_studio->addObserver(this);
    m_observingStudio = true;

    initDialog();
    setupActions();

#if 0
    // Check for no MIDI devices and disable everything
    // ??? This code never does anything because we do not allow this
    //     window to launch if there are no devices.
    const DeviceList *devices = m_studio->getDevices();
    bool haveMidiPlayDevice = false;
    for (const Device *device : *devices) {
        const MidiDevice *md = dynamic_cast<const MidiDevice *>(device);
        if (md  &&  md->getDirection() == MidiDevice::Play) {
            haveMidiPlayDevice = true;
            break;
        }
    }
    if (!haveMidiPlayDevice) {
        leftPart->setDisabled(true);
        m_programEditor->setDisabled(true);
        m_keyMappingEditor->setDisabled(true);
        m_optionBox->setDisabled(true);
    }
#endif

    if (defaultDevice != Device::NO_DEVICE)
        setCurrentDevice(defaultDevice);
}

BankEditorDialog::~BankEditorDialog()
{
    RG_DEBUG << "dtor";

    // Unsubscribe from Studio
    if (m_observingStudio) {
        m_observingStudio = false;
        m_studio->removeObserver(this);
    }

    // Unsubscribe from Device(s) unsubscribe
    for (Device *device : m_observedDevices) {
        unobserveDevice(device);
    }
}

void
BankEditorDialog::setupActions()
{
    createAction("file_close", SLOT(slotFileClose()));

    connect(m_closeButton, &QAbstractButton::clicked, this, &BankEditorDialog::slotFileClose);

    createAction("edit_copy", SLOT(slotCopy()));
    createAction("edit_paste", SLOT(slotPaste()));
    createAction("bank_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    createMenusAndToolbars("bankeditor.rc");
}

void
BankEditorDialog::initDialog()
{
    m_treeWidget->clear();

    // Fill tree

    DeviceList *devices = m_studio->getDevices();

    // For each Device...
    // iterates over devices and create device-TreeWidgetItems (level: topLevelItem)
    // then calls populateDeviceItem() to create bank-TreeWidgetItems (level: topLevelItem-child)
    for (Device *device : *devices) {

        // Not a MIDI Device?  Try the next.
        if (device->getType() != Device::Midi)
            continue;

        MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(device);
        if (!midiDevice)
            continue;

        // Not a playback Device?  Try the next.
        if (midiDevice->getDirection() != MidiDevice::Play)
            continue;

        observeDevice(midiDevice);

        QString itemName = strtoqstr(midiDevice->getName());

        RG_DEBUG << "BankEditorDialog::initDialog - adding " << itemName;

        QTreeWidgetItem *twItemDevice = new MidiDeviceTreeWidgetItem(
                midiDevice,
                m_treeWidget,
                itemName);

        m_treeWidget->addTopLevelItem(twItemDevice);

        twItemDevice->setExpanded(true);

        // ??? updateDialog() does this as well.  Can we combine into a
        //     single routine that does both or is used by both?
        populateDeviceItem(twItemDevice, midiDevice);
    }

    // Select the first device item.
    m_treeWidget->topLevelItem(0)->setSelected(true);
    // Set up the right side for item 0.
    updateEditor(m_treeWidget->topLevelItem(0));

    // Make sure the first column is big enough for its contents.
    // ??? We could really use this everywhere the columns come up the wrong
    //     size.  Which seems like everywhere.
    m_treeWidget->resizeColumnToContents(0);
}

void
BankEditorDialog::updateDialog()
{
    //RG_DEBUG << "updateDialog()";

    // Update list view

    // Get selected Item.

    enum class SelectedType {NONE, DEVICE, BANK, KEYMAP};
    SelectedType selectedType{SelectedType::NONE};
    QString selectedName;
    Device *parentDevice{nullptr};

    // ??? This is polymorphism.  Move this behavior to
    //     MidiDeviceTreeWidgetItem and override as appropriate
    //     in MidiKeyMapTreeWidgetItem and MidiBankTreeWidgetItem.
    //     Then the code in the #else reduces to:
#if 0
    // ??? Problem is that the inheritance hierarchy is sus.
    //     MidiKeyMapTreeWidgetItem is *not* a kind of
    //     MidiDeviceTreeWidgetItem.  This looks like inheritance for
    //     convenience rather than inheritance to express a model.
    //     This will need to be addressed before any redesign.
    const QTreeWidgetItem *item = m_treeWidget->currentItem();
    const MidiDeviceTreeWidgetItem *deviceItem =
            dynamic_cast<const MidiDeviceTreeWidgetItem *>(item);
    if (deviceItem) {
        selectedType = deviceItem->getType();
        selectedName = deviceItem->getName();
        parentDevice = deviceItem->getDevice();
    }
#else
    QTreeWidgetItem *item = m_treeWidget->currentItem();
    if (item) {
        const MidiDeviceTreeWidgetItem *deviceItem =
                dynamic_cast<const MidiDeviceTreeWidgetItem *>(item);
        if (deviceItem) {
            selectedType = SelectedType::DEVICE;
            MidiDevice *device = deviceItem->getDevice();
            if (device) {
                selectedName = strtoqstr(device->getName());
                parentDevice = device;
            } else {
                selectedType = SelectedType::NONE;
            }
        }
        const MidiKeyMapTreeWidgetItem *keyItem =
                dynamic_cast<const MidiKeyMapTreeWidgetItem *>(item);
        if (keyItem) {
            selectedType = SelectedType::KEYMAP;
            selectedName = keyItem->getName();
            parentDevice = keyItem->getDevice();
        }
        const MidiBankTreeWidgetItem *bankItem =
            dynamic_cast<const MidiBankTreeWidgetItem *>(item);
        if (bankItem) {
            selectedType = SelectedType::BANK;
            selectedName = bankItem->getName();
            parentDevice = bankItem->getDevice();
        }
    }
#endif

    // The current item was renamed.  Make sure we use the new name to find it.
    if (m_selectionName != "") {
        selectedName = m_selectionName;
        m_selectionName = "";
    }

    //RG_DEBUG << "selected item:" << (int)selectedType << selectedName << parentDevice;

    // Have to block signals or else we will get itemChanged() while we are
    // doing work.  QTreeWidget doesn't offer an itemChangedByUser().
    m_treeWidget->blockSignals(true);

    // Start from scratch.
    m_treeWidget->clear();

    DeviceList *devices = m_studio->getDevices();

    // For each Device in the Studio...
    for (Device *device : *devices) {

        // Not a MIDI device?  Try the next.
        if (device->getType() != Device::Midi)
            continue;

        MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(device);
        if (!midiDevice)
            continue;

        // Record device?  Try the next.
        if (midiDevice->getDirection() == MidiDevice::Record)
            continue;

        QString itemName = strtoqstr(midiDevice->getName());

        //RG_DEBUG << "BankEditorDialog::updateDialog - adding " << itemName;

        // Create a new entry on the tree.
        // ??? Reorg parameters so that m_treeWidget is first like
        //     QTreeWidgetItem's ctor.
        QTreeWidgetItem *deviceItem = new MidiDeviceTreeWidgetItem(
                midiDevice, m_treeWidget, itemName);

        deviceItem->setExpanded(true);

        // Add the banks and key maps for this device to the tree.
        // ??? initDialog() does this as well.  Can we combine into a
        //     single routine that does both or is used by both?
        populateDeviceItem(deviceItem, midiDevice);
    }

    m_treeWidget->blockSignals(false);

    // ??? This does not restore scroll position.

    // Restore the item selection.

    // ??? Might want to pull this out into a function so that the returns can
    //     be contained.  This will help when implementing scroll position
    //     restoration.

    // ??? Could we have searched for this in the last loop and saved the item
    //     pointer for a call to setCurrentItem later?  That would mess up
    //     pulling out the above loop to be shared by initDialog(), but it
    //     would avoid a second scan of the tree.

    //RG_DEBUG << "selecting item:" << (int)selectedType << selectedName << parentDevice;

    if (selectedType == SelectedType::NONE)
        return;

    // Find the top level device item.
    MidiDeviceTreeWidgetItem *selectDeviceItem{nullptr};
    // ??? Use topLevelItemCount() and topLevelItem().
    QTreeWidgetItem *root = m_treeWidget->invisibleRootItem();
    // For each top level item...
    for (int i=0; i < root->childCount(); ++i) {
        QTreeWidgetItem *item = root->child(i);
        MidiDeviceTreeWidgetItem *deviceItem =
                dynamic_cast<MidiDeviceTreeWidgetItem *>(item);
        if (!deviceItem)
            continue;
        // Found it?  Remember it.
        if (deviceItem->getDevice() == parentDevice) {
            selectDeviceItem = deviceItem;
            break;
        }
    }

    // Device is gone?  No selection.
    if (!selectDeviceItem)
        return;

    // The device itself is selected?
    if (selectedType == SelectedType::DEVICE) {
        m_treeWidget->setCurrentItem(selectDeviceItem);
        return;
    }

    // Bank or Keymap?
    if (selectedType == SelectedType::BANK  ||
        selectedType == SelectedType::KEYMAP) {
        int childCount = selectDeviceItem->childCount();
        for (int i=0; i < childCount; ++i) {
            QTreeWidgetItem *childItem = selectDeviceItem->child(i);

            // ??? This is polymorphism like above.  Express this in the
            //     class hierarchy so we can reduce this to a single "if":
            //     if (childItem->getName() == selectedName)
            //         m_treeWidget->setCurrentItem(childItem);

            MidiKeyMapTreeWidgetItem *keyItem =
                    dynamic_cast<MidiKeyMapTreeWidgetItem *>(childItem);
            if (keyItem  &&  selectedType == SelectedType::KEYMAP) {
                const QString childName = keyItem->getName();
                // Found it?
                if (childName == selectedName) {
                    RG_DEBUG << "updateDialog() setCurrent keymap" << childName;
                    m_treeWidget->setCurrentItem(childItem);
                    return;
                }
            }
            MidiBankTreeWidgetItem *bankItem =
                    dynamic_cast<MidiBankTreeWidgetItem *>(childItem);
            if (bankItem  &&  selectedType == SelectedType::BANK) {
                const QString childName = bankItem->getName();
                // Found it?
                if (childName == selectedName) {
                    RG_DEBUG << "updateDialog() setCurrent bank" << childName;
                    m_treeWidget->setCurrentItem(childItem);
                    return;
                }
            }
        }

        RG_DEBUG << "updateDialog() punting, going with device" << selectDeviceItem->getName();

        // No suitable child item found - select device.
        m_treeWidget->setCurrentItem(selectDeviceItem);
    }
}

void
BankEditorDialog::setCurrentDevice(DeviceId device)
{
    const unsigned count = m_treeWidget->topLevelItemCount();

    // For each top level (Device) item...
    for (unsigned i = 0; i < count; ++i) {
        QTreeWidgetItem *item = m_treeWidget->topLevelItem(i);
        MidiDeviceTreeWidgetItem *deviceItem =
                dynamic_cast<MidiDeviceTreeWidgetItem *>(item);
        if (deviceItem  &&  deviceItem->getDevice()->getId() == device) {
            m_treeWidget->setCurrentItem(item);
            break;
        }
    }
}

void
BankEditorDialog::populateDeviceItem(
        QTreeWidgetItem *deviceItem, MidiDevice *midiDevice)
{
    // Remove children from deviceItem.
    // While there are items to remove...
    while (deviceItem->childCount() > 0)
        delete deviceItem->child(0);

    // Add Banks

    BankList banks = midiDevice->getBanks();
    // add banks for this device
    for (size_t i = 0; i < banks.size(); ++i) {
        RG_DEBUG << "populateDeviceItem() - adding bank " << strtoqstr(midiDevice->getName()) << " - " << strtoqstr(banks[i].getName());
        new MidiBankTreeWidgetItem(
                midiDevice,
                i,  // bankNb
                deviceItem,  // parent
                strtoqstr(banks[i].getName()),  // name
                banks[i].isPercussion(),
                banks[i].getMSB(),
                banks[i].getLSB());
    }

    // Add Key Maps

    const KeyMappingList &mappings = midiDevice->getKeyMappings();
    for (size_t i = 0; i < mappings.size(); ++i) {
        RG_DEBUG << "populateDeviceItem() - adding key map " << strtoqstr(midiDevice->getName()) << " - " << strtoqstr(mappings[i].getName());
        new MidiKeyMapTreeWidgetItem(
                midiDevice,
                deviceItem,  // parent
                strtoqstr(mappings[i].getName()));  // name
    }
}

void BankEditorDialog::slotUpdateEditor(QTreeWidgetItem *currentItem, QTreeWidgetItem * /*previousItem*/)
{
    RG_DEBUG << "slotUpdateEditor()";

    if (!currentItem)
        return;

    // Show and update the program editor or the key map editor.
    updateEditor(currentItem);
}

void BankEditorDialog::updateEditor(QTreeWidgetItem *item)
{
    if (!item)
        return;

    // Key Map Editor

    const MidiKeyMapTreeWidgetItem *keyItem =
            dynamic_cast<MidiKeyMapTreeWidgetItem *>(item);

    if (keyItem) {

        enterActionState("on_key_item");
        leaveActionState("on_bank_item");

        m_delete->setEnabled(true);
        m_copy->setEnabled(true);
        m_paste->setEnabled(m_clipboard.itemType == ItemType::KEYMAP);

        m_keyMappingEditor->populate(item);

        m_programEditor->hide();
        m_keyMappingEditor->show();

        m_rightSide->setEnabled(true);

        return;
    }

    // Program Editor

    const MidiBankTreeWidgetItem *bankItem =
            dynamic_cast<MidiBankTreeWidgetItem *>(item);

    if (bankItem) {

        enterActionState("on_bank_item");
        leaveActionState("on_key_item");

        m_delete->setEnabled(true);
        m_copy->setEnabled(true);
        m_paste->setEnabled(m_clipboard.itemType == ItemType::BANK);

        MidiDevice *device = bankItem->getDevice();
        if (!device)
            return;

        // ??? Get rid of this.
        m_variationCheckBox->blockSignals(true);

        m_variationCheckBox->setChecked(
                device->getVariationType() != MidiDevice::NoVariations);

        // ??? Get rid of this.
        m_variationCheckBox->blockSignals(false);

        m_variationCombo->setEnabled(m_variationCheckBox->isChecked());

        // ??? Get rid of this.
        m_variationCombo->blockSignals(true);

        m_variationCombo->setCurrentIndex(
                device->getVariationType() == MidiDevice::VariationFromLSB ? 0 : 1);

        // ??? Get rid of this.
        m_variationCombo->blockSignals(false);

        m_programEditor->populate(item);

        m_keyMappingEditor->hide();
        m_programEditor->show();

        m_rightSide->setEnabled(true);

        return;
    }

    // Device, not bank or key mapping.

    RG_DEBUG << "updateEditor() : not a bank item";

    // Disable buttons.
    m_delete->setEnabled(false);
    m_copy->setEnabled(false);
    m_paste->setEnabled(false);
    m_rightSide->setEnabled(false);

    // Leave all action states.
    leaveActionState("on_bank_item");
    leaveActionState("on_key_item");

    // Clear the right side editors.
    m_programEditor->clearAll();
    m_keyMappingEditor->clearAll();

    // Update Variation Widgets

    // ??? This code is duplicated above.  Pull out an updateVariations().

    MidiDeviceTreeWidgetItem *deviceItem = getParentDeviceItem(item);
    if (!deviceItem) {
        RG_DEBUG << "updateEditor() - no MidiDeviceTreeWidgetItem";
        return;
    }

    MidiDevice *device = deviceItem->getDevice();
    if (!device) {
        RG_DEBUG << "updateEditor() - no MidiDevice for this item";
        return;
    }

    m_variationCheckBox->setChecked(
            device->getVariationType() != MidiDevice::NoVariations);
    m_variationCombo->setEnabled(m_variationCheckBox->isChecked());
    m_variationCombo->setCurrentIndex(
            device->getVariationType() == MidiDevice::VariationFromLSB ? 0 : 1);
}

MidiDeviceTreeWidgetItem *
BankEditorDialog::getParentDeviceItem(QTreeWidgetItem *item)
{
    if (!item)
        return nullptr;

    if (dynamic_cast<MidiBankTreeWidgetItem *>(item))
        item = item->parent();
    else if (dynamic_cast<MidiKeyMapTreeWidgetItem *>(item))
        item = item->parent();

    if (!item) {
        RG_WARNING << "getParentDeviceItem(): missing parent device item for bank item";
        return nullptr;
    }

    return dynamic_cast<MidiDeviceTreeWidgetItem *>(item);
}

void
BankEditorDialog::slotAddBank()
{
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem)
        return;

    MidiDeviceTreeWidgetItem *deviceItem = getParentDeviceItem(currentItem);
    if (!deviceItem)
        return;

    MidiDevice *device = deviceItem->getDevice();
    if (!device)
        return;

    // Make a copy of the bank list so we can add the new one.
    BankList banks = device->getBanks();

    // Generate an unused "new bank" name.
    // ??? Seems like this belongs in MidiDevice.
    QString name;
    for (size_t i = 1; i <= banks.size() + 1; ++i) {
        if (i == 1)
            name = tr("<new bank>");
        else
            name = tr("<new bank %1>").arg(i);
        // No such bank?  Then we have our name.
        if (device->getBankByName(qstrtostr(name)) == nullptr)
            break;
    }

    MidiByte msb;
    MidiByte lsb;
    getFirstFreeBank(device, msb, lsb);

    MidiBank newBank(false,  // percussion
                     msb, lsb,
                     qstrtostr(name));

    banks.push_back(newBank);

    RG_DEBUG << "slotAddBank() : deviceItem->getDeviceId() = " << deviceItem->getDevice()->getId();

    ModifyDeviceCommand *command = makeCommand(tr("add MIDI Bank"));
    if (!command)
        return;
    command->setBankList(banks);
    CommandHistory::getInstance()->addCommand(command);

    // ??? We should select the new item.  ModifyDeviceCommand is probably in
    //     a better position to do that.  Or can we use m_selectionName?
}

void
BankEditorDialog::slotAddKeyMapping()
{
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem)
        return;

    MidiDeviceTreeWidgetItem *deviceItem = getParentDeviceItem(currentItem);
    if (!deviceItem)
        return;

    MidiDevice *device = deviceItem->getDevice();
    if (!device)
        return;

    const KeyMappingList &keyMapList = device->getKeyMappings();

    // Generate an unused "new mapping" name.
    // ??? Seems like this belongs in MidiDevice.
    QString name;
    for (size_t i = 1; i <= keyMapList.size() + 1; ++i) {
        if (i == 1)
            name = tr("<new mapping>");
        else
            name = tr("<new mapping %1>").arg(i);
        // No such bank?  Then we have our name.
        if (device->getKeyMappingByName(qstrtostr(name)) == nullptr)
            break;
    }

    KeyMappingList newKeyMapList;

    MidiKeyMapping newKeyMap(qstrtostr(name));
    newKeyMapList.push_back(newKeyMap);

    ModifyDeviceCommand *command = makeCommand(tr("add Key Mapping"));
    if (!command)
        return;
    command->setKeyMappingList(newKeyMapList);
    // Merge
    command->setOverwrite(false);
    command->setRename(false);
    CommandHistory::getInstance()->addCommand(command);

    // ??? We should select the new item.  ModifyDeviceCommand is probably in
    //     a better position to do that.  Or can we use m_selectionName?
}

void
BankEditorDialog::slotDelete()
{
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem)
        return;

    // Bank

    const MidiBankTreeWidgetItem *bankItem =
            dynamic_cast<const MidiBankTreeWidgetItem *>(currentItem);
    if (bankItem) {
        const MidiDevice *device = bankItem->getDevice();
        if (!device)
            return;

        const BankList &banks = device->getBanks();
        const MidiBank &bank = banks[bankItem->getBank()];

        BankList newBanks;
        // Copy all banks except for the one we are deleting to newBanks.
        for (size_t i = 0; i < banks.size(); ++i) {
            MidiBank ibank = banks[i];
            if (!ibank.compareKey(bank))
                newBanks.push_back(ibank);
        }

        // Confirm the bank is not in use.
        // ??? Shouldn't we do this before creating newBanks?
        const bool used = tracksUsingBank(bank, *device);
        if (used)
            return;

        // Are You Sure?
        const int reply = QMessageBox::warning(
                this,  // parent
                tr("Rosegarden"), // title
                tr("Really delete this bank?"),  // text
                QMessageBox::Yes | QMessageBox::No,  // buttons
                QMessageBox::No);  // defaultButton

        if (reply == QMessageBox::No)
            return;

        // Copy all programs that aren't in the doomed bank to
        // newProgramList.
        ProgramList newProgramList;
        const ProgramList &oldProgramList = device->getPrograms();
        for (const MidiProgram &midiProgram : oldProgramList) {
            // If this program isn't in the bank that is being deleted,
            // add it to the new program list.  We use compareKey()
            // because the MidiBank objects in the program list do not
            // have their name fields filled in.
            if (!midiProgram.getBank().compareKey(bank))
                newProgramList.push_back(midiProgram);
        }

        // If the bank that is about to be deleted is in the clipboard...
        if (m_clipboard.itemType == ItemType::BANK  &&
            m_clipboard.deviceId == bankItem->getDevice()->getId()  &&
            m_clipboard.bank == bankItem->getBank()) {

            // Clear the clipboard to avoid pasting a non-existent bank.
            m_paste->setEnabled(false);
            m_clipboard.itemType = ItemType::NONE;
            m_clipboard.deviceId = Device::NO_DEVICE;
            m_clipboard.bank = -1;
            m_clipboard.keymapName = "";
        }

        ModifyDeviceCommand *command = makeCommand(tr("delete MIDI bank"));
        if (!command)
            return;
        command->setBankList(newBanks);
        command->setProgramList(newProgramList);
        CommandHistory::getInstance()->addCommand(command);

        return;
    }

    // Key Map

    const MidiKeyMapTreeWidgetItem *keyItem =
            dynamic_cast<const MidiKeyMapTreeWidgetItem *>(currentItem);
    if (keyItem) {
        const MidiDevice *device = keyItem->getDevice();
        if (!device)
            return;

        const int reply = QMessageBox::warning(
                this,  // parent
                tr("Rosegarden"),  // title
                tr("Really delete this key mapping?"),  // text
                QMessageBox::Yes | QMessageBox::No,  // buttons
                QMessageBox::No);  // defaultButton

        if (reply == QMessageBox::No)
            return;

        const std::string keyMappingName = qstrtostr(keyItem->getName());

        // Make a copy of the key map list so we can remove the deleted one.
        KeyMappingList keyMapList = device->getKeyMappings();

        for (KeyMappingList::iterator i = keyMapList.begin();
             i != keyMapList.end();
             ++i) {
            if (i->getName() == keyMappingName) {
                RG_DEBUG << "slotDelete(): erasing " << keyMappingName;
                keyMapList.erase(i);
                break;
            }
        }

        RG_DEBUG << "slotDelete(): setting" << keyMapList.size() << "key mappings to device";

        ModifyDeviceCommand *command = makeCommand(tr("delete Key Mapping"));
        if (!command)
            return;
        command->setKeyMappingList(keyMapList);
        CommandHistory::getInstance()->addCommand(command);

        RG_DEBUG << "device has" << device->getKeyMappings().size() << "key mappings now";

        return;
    }
}

void
BankEditorDialog::slotDeleteAll()
{
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem)
        return;

    MidiDeviceTreeWidgetItem *deviceItem = getParentDeviceItem(currentItem);
    if (!deviceItem)
        return;

    MidiDevice *device = deviceItem->getDevice();
    if (!device)
        return;

    const BankList &banks = device->getBanks();

    // Check for banks in use.
    for (const MidiBank &bank : banks) {
        bool used = tracksUsingBank(bank, *device);
        if (used)
            return;
    }

    const QString question = tr("Really delete all banks and keymaps for ") +
                             strtoqstr(device->getName()) + QString(" ?");

    const int reply = QMessageBox::warning(
            this,  // parent
            tr("Rosegarden"),  // title
            question,  // text
            QMessageBox::Yes | QMessageBox::No,  // buttons
            QMessageBox::No);  // defaultButton

    if (reply == QMessageBox::No)
        return;

    // Clear the clipboard if it refers to the device being cleared.
    if (m_clipboard.deviceId == device->getId()) {
        m_paste->setEnabled(false);
        m_clipboard.itemType = ItemType::NONE;
        m_clipboard.deviceId = Device::NO_DEVICE;
        m_clipboard.bank = -1;
        m_clipboard.keymapName = "";
    }


    ModifyDeviceCommand *command = makeCommand(tr("delete all"));
    if (!command)
        return;

    BankList emptyBankList;
    command->setBankList(emptyBankList);
    ProgramList emptyProgramList;
    command->setProgramList(emptyProgramList);
    KeyMappingList emptyKeymapList;
    command->setKeyMappingList(emptyKeymapList);

    CommandHistory::getInstance()->addCommand(command);
}

void
BankEditorDialog::getFirstFreeBank(
        MidiDevice *device, MidiByte &o_msb, MidiByte &o_lsb)
{
    // This ignores percussion true/false.  That's ok because the user can
    // toggle percussion then adjust the msb/lsb to get the one they want.

    o_msb = 0;
    o_lsb = 0;

    BankList banks = device->getBanks();

    // For all msb values...
    for (int msb = MidiMinValue; msb < MidiMaxValue; ++msb) {
        // For all lsb values...
        for (int lsb = MidiMinValue; lsb < MidiMaxValue; ++lsb) {
            BankList::const_iterator i = banks.begin();
            // For all banks on this Device...
            for (; i != banks.end(); ++i) {
                // Conflict?  Try the next msb/lsb pair.
                if (i->getLSB() == lsb  &&  i->getMSB() == msb)
                    break;
            }
            // No conflict?  Go with this.
            if (i == banks.end()) {
                o_msb = msb;
                o_lsb = lsb;
                return;
            }
        }
    }
}

void
BankEditorDialog::slotItemChanged(QTreeWidgetItem *item, int /* column */)
{
    RG_DEBUG << "slotItemChanged()";

    const QString label = item->text(0);
    // do not allow blank names
    if (label == "") {
        updateDialog();
        return;
    }

    // Bank

    const MidiBankTreeWidgetItem *bankItem =
            dynamic_cast<MidiBankTreeWidgetItem *>(item);

    if (bankItem) {

        RG_DEBUG << "  modify bank name to " << label;

        const MidiDeviceTreeWidgetItem *deviceItem = getParentDeviceItem(item);
        if (!deviceItem)
            return;

        const MidiDevice *device = deviceItem->getDevice();
        if (!device)
            return;

        // Make a copy of the bank list so we can change the name.
        BankList banks = device->getBanks();

        // Make sure the new name is unique.
        const QString uniqueName = makeUniqueBankName(label, banks);

        // Let updateDialog() know it should select the item with this new name.
        m_selectionName = uniqueName;

        const int bankIndex = bankItem->getBank();
        banks[bankIndex].setName(qstrtostr(uniqueName));

        RG_DEBUG << "  deviceItem->getDeviceId() = " << deviceItem->getDevice()->getId();

        ModifyDeviceCommand *command = makeCommand(tr("rename MIDI Bank"));
        if (!command)
            return;
        command->setBankList(banks);
        CommandHistory::getInstance()->addCommand(command);

        return;

    }

    // Key Map

    const MidiKeyMapTreeWidgetItem *keyItem =
            dynamic_cast<MidiKeyMapTreeWidgetItem *>(item);

    if (keyItem) {

        RG_DEBUG << "  modify key mapping name to " << label;

        const QString oldName = keyItem->getName();

        const MidiDeviceTreeWidgetItem *deviceItem = getParentDeviceItem(item);
        if (!deviceItem)
            return;

        const MidiDevice *device = deviceItem->getDevice();
        if (!device)
            return;

        // Make a copy of the key map list so we can change the name.
        KeyMappingList keyMapList = device->getKeyMappings();

        // Make sure the new name is unique.
        const QString uniqueName = makeUniqueKeymapName(label, keyMapList);

        // Let updateDialog() know it should select the item with this new name.
        m_selectionName = uniqueName;

        // For each key map...
        for (MidiKeyMapping &keyMap : keyMapList) {
            // Found it?  Change it.
            if (keyMap.getName() == qstrtostr(oldName)) {
                keyMap.setName(qstrtostr(uniqueName));
                break;
            }
        }

        ModifyDeviceCommand *command =
                makeCommand(tr("rename Key Mapping"));
        if (!command)
            return;
        command->setKeyMappingList(keyMapList);
        CommandHistory::getInstance()->addCommand(command);

        return;

    }
}

void
BankEditorDialog::selectDeviceItem(MidiDevice *device)
{
    // For each top-level item in the tree...
    for (int itemIndex = 0;
         itemIndex < m_treeWidget->topLevelItemCount();
         ++itemIndex) {

        QTreeWidgetItem *child = m_treeWidget->topLevelItem(itemIndex);
        const MidiDeviceTreeWidgetItem *midiDeviceItem =
                dynamic_cast<const MidiDeviceTreeWidgetItem *>(child);

        if (midiDeviceItem) {
            MidiDevice *midiDevice = midiDeviceItem->getDevice();

            // Found the device?  Make it the current (selected) item and bail.
            if (midiDevice == device) {
                m_treeWidget->setCurrentItem(child);
                break;
            }
        }

    }
}

QString BankEditorDialog::makeUniqueBankName(const QString &name,
                                             const BankList &banks)
{
    QString uniqueName = name;

    int suffix = 1;

    while (true) {

        bool foundName = false;

        // For each bank in banks...
        for (const MidiBank &midiBank : banks) {
            QString bankName = strtoqstr(midiBank.getName());
            // If found, we need to add a suffix.
            if (uniqueName == bankName) {
                foundName = true;
                uniqueName = QString("%1_%2").arg(name).arg(suffix);
                ++suffix;
                break;
            }
        }

        // Not found, so this one is unique.
        if (!foundName)
            break;

    }

    return uniqueName;
}

QString BankEditorDialog::makeUniqueKeymapName(const QString &name,
                                               const KeyMappingList &keyMaps)
{
    QString uniqueName = name;

    int suffix = 1;

    while (true) {

        bool foundName = false;

        // For each key map in keyMaps...
        for (const MidiKeyMapping &keyMap : keyMaps) {
            const QString keyMapName = strtoqstr(keyMap.getName());
            // If found, we need to add a suffix.
            if (uniqueName == keyMapName) {
                foundName = true;
                uniqueName = QString("%1_%2").arg(name).arg(suffix);
                ++suffix;
                break;
            }
        }

        // Not found, so this one is unique.
        if (!foundName)
            break;

    }

    return uniqueName;
}


void
BankEditorDialog::slotVariationToggled()
{
    MidiDevice::VariationType variation = MidiDevice::NoVariations;
    if (m_variationCheckBox->isChecked()) {
        if (m_variationCombo->currentIndex() == 0)
            variation = MidiDevice::VariationFromLSB;
        else
            variation = MidiDevice::VariationFromMSB;
    }

    ModifyDeviceCommand *command = makeCommand(tr("variation toggled"));
    if (!command)
        return;
    command->setVariation(variation);
    CommandHistory::getInstance()->addCommand(command);

    m_variationCombo->setEnabled(m_variationCheckBox->isChecked());
}

void
BankEditorDialog::slotVariationChanged(int)
{
    MidiDevice::VariationType variation = MidiDevice::NoVariations;
    if (m_variationCheckBox->isChecked()) {
        if (m_variationCombo->currentIndex() == 0)
            variation = MidiDevice::VariationFromLSB;
        else
            variation = MidiDevice::VariationFromMSB;
    }

    ModifyDeviceCommand *command = makeCommand(tr("variation changed"));
    if (!command)
        return;
    command->setVariation(variation);
    CommandHistory::getInstance()->addCommand(command);
}

ModifyDeviceCommand *
BankEditorDialog::makeCommand(const QString &commandName)
{
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem)
        return nullptr;

    const MidiDeviceTreeWidgetItem *deviceItem = getParentDeviceItem(currentItem);
    if (!deviceItem)
        return nullptr;

    const MidiDevice *device = deviceItem->getDevice();
    if (!device)
        return nullptr;

    ModifyDeviceCommand *command = new ModifyDeviceCommand(
            m_studio,  // studio
            device->getId(),  // device
            device->getName(),  // name
            device->getLibrarianName(),  // librarianName
            device->getLibrarianEmail(),  // librarianEmail
            commandName);

    return command;
}

void
BankEditorDialog::slotImport()
{
#if QT_VERSION >= 0x050000
    const QString home = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).path();
#else
    const QString home = QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation)).path();
#endif

    const QString deviceDir = home + "/.local/share/rosegarden/library";

    QString urlString = FileDialog::getOpenFileName(
            this,  // parent
            tr("Import Banks from Device in File"),  // caption
            deviceDir,  // dir
            tr("Rosegarden Device files") + " (*.rgd *.RGD)" + ";;" +
                tr("Rosegarden files") + " (*.rg *.RG)" + ";;" +
                tr("Sound fonts") + " (*.sf2 *.SF2)" + ";;" +
                tr("LinuxSampler configuration files") + " (*.lscp *.LSCP)" + ";;" +
                tr("All files") + " (*)",  // filter
            nullptr);  // selectedFilter

    QUrl url(urlString);
    if (url.isEmpty())
        return;

    std::unique_ptr<ImportDeviceDialog> dialog{new ImportDeviceDialog(this, url)};
    if (!dialog)
        return;

    // Set the dialog up for import.
    if (!dialog->doImport())
        return;

    if (dialog->exec() == QDialog::Accepted) {

        if (!dialog->haveDevice()) {
            QMessageBox::critical(
                    this,  // parent
                    tr("Rosegarden"),  // title
                    tr("Some internal error: no device selected"));  // text

            return;
        }

        MidiDeviceTreeWidgetItem *deviceItem =
                dynamic_cast<MidiDeviceTreeWidgetItem *>(
                        m_treeWidget->currentItem());

        if (!deviceItem) {
            QMessageBox::critical(
                    this,  // parent
                    tr("Rosegarden"),  // title
                    tr("Some internal error: cannot locate selected device"));  // text

            return;
        }

        MidiDevice *device = deviceItem->getDevice();
        if (!device)
            return;

        std::string librarianName(dialog->getLibrarianName());
        std::string librarianEmail(dialog->getLibrarianEmail());

        // don't record the librarian when
        // merging banks -- it's misleading.
        if (!dialog->shouldOverwriteBanks()) {
            librarianName = "";
            librarianEmail = "";
        }

        ModifyDeviceCommand *command = new ModifyDeviceCommand(
                m_studio,  // studio
                device->getId(),  // device
                dialog->getDeviceName(),  // name
                librarianName,
                librarianEmail,
                tr("import device"));  // commandName

        if (dialog->shouldOverwriteBanks())
            command->setVariation(dialog->getVariationType());
        if (dialog->shouldImportBanks()) {
            command->setBankList(dialog->getBanks());
            command->setProgramList(dialog->getPrograms());
        }
        if (dialog->shouldImportControllers())
            command->setControlList(dialog->getControllers());
        if (dialog->shouldImportKeyMappings())
            command->setKeyMappingList(dialog->getKeyMappings());

        command->setOverwrite(dialog->shouldOverwriteBanks());
        command->setRename(dialog->shouldRename());

        CommandHistory::getInstance()->addCommand(command);

        selectDeviceItem(device);
    }

    updateDialog();
}

void
BankEditorDialog::slotEdit(QTreeWidgetItem *item, int /* column */)
{
    RG_DEBUG << "slotEdit()";

    if (item->flags() & Qt::ItemIsEditable)
        m_treeWidget->editItem(item);
}

void
BankEditorDialog::slotCopy()
{
    // Bank

    MidiBankTreeWidgetItem *bankItem =
            dynamic_cast<MidiBankTreeWidgetItem *>(m_treeWidget->currentItem());

    if (bankItem) {
        m_clipboard.itemType = ItemType::BANK;
        m_clipboard.deviceId = bankItem->getDevice()->getId();
        m_clipboard.bank = bankItem->getBank();
        m_clipboard.keymapName = "";
        m_paste->setEnabled(true);
        return;
    }

    // Key Map

    MidiKeyMapTreeWidgetItem *keyItem =
            dynamic_cast<MidiKeyMapTreeWidgetItem*>(m_treeWidget->currentItem());

    if (keyItem) {
        m_clipboard.itemType = ItemType::KEYMAP;
        m_clipboard.deviceId = keyItem->getDevice()->getId();
        m_clipboard.bank = -1;
        m_clipboard.keymapName = keyItem->getName();
        m_paste->setEnabled(true);
        return;
    }
}

void
BankEditorDialog::slotPaste()
{
    // Bank

    // ??? This is odd.  It requires that you paste a bank over another
    //     existing bank.  And then it gets the msb/lsb of that destination
    //     bank.  That doesn't seem very useful.
    //     You can't paste a bank into a device
    //     that has no banks.  Very weird.  I don't think that is ideal.
    //
    //     We should think through some use cases and figure out what
    //     is the best approach.  I don't like the fact that it clobbers
    //     the selected bank.  I think it should always add the bank unless
    //     there is an msb/lsb/percussion conflict.  If there is a conflict,
    //     pop up a window letting the user resolve it.
    //
    //     Given a simple "paste into" rather than "paste over" behavior,
    //     we can do what this does by first deleting the bank we want to paste
    //     over, then paste the bank from the clipboard, and adjust its msb/lsb
    //     to match the one we deleted.

    const MidiBankTreeWidgetItem *bankItem =
            dynamic_cast<const MidiBankTreeWidgetItem *>(m_treeWidget->currentItem());

    if (bankItem) {

        // Bank must be pasted over top of an existing bank.
        if (m_clipboard.itemType != ItemType::BANK)
            return;

        // Remove the bank we are pasting over top of.

        const MidiDevice *device = bankItem->getDevice();
        const BankList bankList = device->getBanks();

        const MidiBank currentBank = bankList[bankItem->getBank()];

        // Get the full program and bank list for the destination device.
        const ProgramList &oldPrograms = device->getPrograms();

        ProgramList newPrograms;

        RG_DEBUG << "slotEditPaste() copying programs we will keep";

        // Copy the programs we will be keeping from oldPrograms to
        // newPrograms.
        for (ProgramList::const_iterator it = oldPrograms.begin();
             it != oldPrograms.end();
             ++it) {

            RG_DEBUG << "slotEditPaste() check remove program" << (*it).getName();

            // If this isn't one we need to remove, copy it to newPrograms.
            if (!(it->getBank().compareKey(currentBank))) {

                RG_DEBUG << "slotEditPaste() add program" << (*it).getName();

                newPrograms.push_back(*it);
            }
        }

        // Add the programs from the clipboard to newPrograms.

        const MidiBank &sourceBank = bankList[m_clipboard.bank];

        const Device *sourceDevice = m_studio->getDevice(m_clipboard.deviceId);
        if (!sourceDevice)
            return;

        const MidiDevice *sourceMidiDevice = dynamic_cast<const MidiDevice *>(sourceDevice);
        if (!sourceMidiDevice)
            return;

        const ProgramList &sourcePrograms = sourceMidiDevice->getPrograms();

        RG_DEBUG << "slotEditPaste copy programs";

        // For each program from the clipboard...
        for (ProgramList::const_iterator it = sourcePrograms.begin();
             it != sourcePrograms.end();
             ++it) {

            RG_DEBUG << "slotEditPaste check copy program" << (*it).getName();

            // If this is a bank from the clipboard...
            if (it->getBank().compareKey(sourceBank)) {

                RG_DEBUG << "slotEditPaste copy program" << (*it).getName();

                // Assemble program for the destination (current) bank.
                const MidiProgram copyProgram(currentBank,
                                              it->getProgram(),
                                              it->getName());

                newPrograms.push_back(copyProgram);
            }
        }

        // Modify the Device.

        ModifyDeviceCommand *command = makeCommand(tr("paste bank"));
        if (!command)
            return;
        command->setProgramList(newPrograms);
        CommandHistory::getInstance()->addCommand(command);

        return;
    }

    // Key Map

    // ??? This is like pasting a bank.  It clobbers the selected key map in
    //     the destination and takes its name.  Given a non-destructive paste,
    //     this can be achieved by deleting the destination key map, pasting,
    //     then renaming the key map to match the one that was deleted.

    const MidiKeyMapTreeWidgetItem *keyItem =
            dynamic_cast<MidiKeyMapTreeWidgetItem*>(m_treeWidget->currentItem());

    if (keyItem) {

        // Key map must be pasted over top of an existing key map.
        if (m_clipboard.itemType != ItemType::KEYMAP)
            return;

        RG_DEBUG << "slotEditPaste() paste keymap";

        // Find the source key map.

        const MidiDevice *device = keyItem->getDevice();
        const KeyMappingList &keyMapList = device->getKeyMappings();

        const Device *sourceDevice = m_studio->getDevice(m_clipboard.deviceId);
        if (!sourceDevice)
            return;

        const MidiDevice *sourceMidiDevice = dynamic_cast<const MidiDevice *>(sourceDevice);
        if (!sourceMidiDevice)
            return;

        const KeyMappingList &sourceKeyMapList = sourceMidiDevice->getKeyMappings();

        // Find the source key map by name.
        int sourceIndex = -1;
        for (size_t i = 0; i < sourceKeyMapList.size(); ++i) {
            if (sourceKeyMapList[i].getName() ==
                        qstrtostr(m_clipboard.keymapName)) {
                sourceIndex = i;
                break;
            }
        }

        RG_DEBUG << "slotEditPaste() sourceIndex" << sourceIndex;

        // Not found?  Bail.
        if (sourceIndex == -1)
            return;

        // Combine the key maps from the destination with the key map
        // from the clipboard.

        // Make a copy so we can modify it.
        MidiKeyMapping sourceMap = sourceKeyMapList[sourceIndex];

        // Name of the key map in the destination that we are going to clobber.
        const std::string selectedKeyItemName = qstrtostr(keyItem->getName());

        // keep the old name
        sourceMap.setName(selectedKeyItemName);

        KeyMappingList newKeymapList;

        for (size_t i = 0; i < keyMapList.size(); ++i) {
            // If this is the one we are pasting over top of, add the
            // key map from the clipboard.
            if (keyMapList[i].getName() == selectedKeyItemName) {
                RG_DEBUG << "slotEditPaste() add new keymap" << i;
                newKeymapList.push_back(sourceMap);
            } else {  // Copy any key maps we are keeping from the destination.
                RG_DEBUG << "slotEditPaste() add old keymap" << i;
                newKeymapList.push_back(keyMapList[i]);
            }
        }

        // Modify the Device.

        ModifyDeviceCommand *command = makeCommand(tr("paste keymap"));
        if (!command)
            return;
        command->setKeyMappingList(newKeymapList);
        CommandHistory::getInstance()->addCommand(command);

        return;
    }
}

void BankEditorDialog::slotEditLibrarian()
{
    RG_DEBUG << "slotEditLibrarian";

    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem)
        return;

    const MidiDeviceTreeWidgetItem *deviceItem = getParentDeviceItem(currentItem);
    if (!deviceItem)
        return;

    const MidiDevice *device = deviceItem->getDevice();
    if (!device)
        return;

    const QString name = strtoqstr(device->getLibrarianName());
    const QString mail = strtoqstr(device->getLibrarianEmail());

    LibrarianDialog dlg(this, name, mail);

    if (dlg.exec() != QDialog::Accepted)
        return;

    RG_DEBUG << "accepted";

    QString newName;
    QString newMail;
    dlg.getLibrarian(newName, newMail);
    if (newName == "") newName = "<none>";
    if (newMail == "") newMail = "<none>";

    RG_DEBUG << "librarian" << name << mail  << "->" << newName << newMail;

    // No change?  Bail.
    if (name == newName  &&  mail == newMail) {
        RG_DEBUG << "librarian unchanged";
        return;
    }

    ModifyDeviceCommand *command =
            new ModifyDeviceCommand(m_studio,
                                    device->getId(),
                                    device->getName(),
                                    qstrtostr(newName),
                                    qstrtostr(newMail),
                                    tr("change librarian"));
    CommandHistory::getInstance()->addCommand(command);
}

void
BankEditorDialog::slotExport()
{
    const QString extension = "rgd";

    const QString dir = ResourceFinder().getResourceSaveDir("library");

    QString name = FileDialog::getSaveFileName(
            this,  // parent
            tr("Export Device as..."),  // caption
            dir,
            "*." + extension);  // defaultName
    if (name.isEmpty())
        return;

    // Append extension if needed.
    if (!name.endsWith("." + extension))
        name += "." + extension;

    const QFileInfo info(name);

    // ??? Is this even possible?  I thought file save dialogs avoided this?
    if (info.isDir()) {
        QMessageBox::warning(
                this,  // parent
                tr("Rosegarden"),  // title
                tr("You have specified a directory"));  // text
        return;
    }

    if (info.exists()) {
        const int overwrite = QMessageBox::question(
                this,  // parent
                tr("Rosegarden"),  // title
                tr("The specified file exists.  Overwrite?"),  // text
                QMessageBox::Yes | QMessageBox::No,  // buttons
                QMessageBox::No);  // defaultButton

        if (overwrite != QMessageBox::Yes)
            return;
    }

    // Note that this might actually be a bank or key map item.
    // That's ok since getDevice() will get the containing Device.
    const MidiDeviceTreeWidgetItem *deviceItem =
            dynamic_cast<const MidiDeviceTreeWidgetItem *>(
                    m_treeWidget->currentItem());

    std::vector<DeviceId> devices;

    // Get the selected Device or the Device that contains the selected
    // bank or key map.
    const MidiDevice *midiDevice = deviceItem->getDevice();

    if (midiDevice) {
        ExportDeviceDialog *exportDeviceDialog = new ExportDeviceDialog(
                this, strtoqstr(midiDevice->getName()));
        if (exportDeviceDialog->exec() != QDialog::Accepted)
            return;

        // Let exportStudio() know which device to export.  Otherwise it
        // will export all devices.
        if (exportDeviceDialog->getExportType() == ExportDeviceDialog::ExportOne)
            devices.push_back(midiDevice->getId());
    }

    // Export the Device file.

    QString errMsg;
    if (!m_doc->exportStudio(name, errMsg, devices)) {
        if (errMsg != "") {
            QMessageBox::critical(
                    this,
                    tr("Rosegarden"),
                    tr("Could not export studio to file at %1\n(%2)").
                            arg(name).arg(errMsg));
        } else {
            QMessageBox::critical(
                    this,
                    tr("Rosegarden"),
                    tr("Could not export studio to file at %1").arg(name));
        }
    }
}

void
BankEditorDialog::slotFileClose()
{
    RG_DEBUG << "BankEditorDialog::slotFileClose()\n";

    if (m_observingStudio) {
        m_observingStudio = false;
        m_studio->removeObserver(this);
    }
    for(Device* device : m_observedDevices) {
        unobserveDevice(device);
    }

    // We need to do this because we might be here due to a
    // documentAboutToChange signal, in which case the document won't
    // be valid by the time we reach the dtor, since it will be
    // triggered when the closeEvent is actually processed.
    //
//     CommandHistory::getInstance()->detachView(actionCollection());    //&&&
    m_doc = nullptr;
    close();
}

void
BankEditorDialog::closeEvent(QCloseEvent *e)
{
    emit closing();
    QMainWindow::closeEvent(e);
}


void
BankEditorDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:bankEditorDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:bankEditorDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
BankEditorDialog::slotHelpAbout()
{
    new AboutDialog(this);
}

bool BankEditorDialog::tracksUsingBank(const MidiBank& bank,
                                       const MidiDevice& device)
{
    QString bankName = strtoqstr(bank.getName());
    RG_DEBUG << "tracksUsingBank" << bankName << device.getId();
    std::vector<int> trackPositions;

    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();
    const Composition::trackcontainer &tracks = composition.getTracks();

    // For each Track in the Composition...
    for (const Composition::trackcontainer::value_type &pair : tracks) {
        const Track *track = pair.second;
        if (!track)
            continue;

        const InstrumentId instrumentID = track->getInstrument();
        const Instrument *instrument = m_studio->getInstrumentById(instrumentID);
        if (!instrument)
            continue;
        if (instrument->getType() != Instrument::Midi)
            continue;

        Device *idevice = instrument->getDevice();
        // if the bank is on a different device ignore it
        if (idevice->getId() != device.getId()) continue;

        const MidiProgram& program = instrument->getProgram();
        const MidiBank& ibank = program.getBank();
        if (bank.compareKey(ibank)) {
            // Found a Track using this bank
            trackPositions.push_back(track->getPosition());
        }
    }

    // If there are Tracks using this Bank, issue a message and return true
    if (!trackPositions.empty()) {
        QString msg =
            QString(tr("The following tracks are using bank %1:")).
            arg(bankName);
        msg += '\n';
        for (const int &trackPos : trackPositions) {
            msg += QString::number(trackPos + 1) + " ";
        }
        msg += '\n';
        msg += tr("The bank cannot be deleted.");
        QMessageBox::warning(
                this,
                tr("Rosegarden"),
                msg);
        return true;
    }
    return false;
}

void BankEditorDialog::deviceAdded(Device* device)
{
    RG_DEBUG << "deviceAdded" << device;
    observeDevice(device);
    updateDialog();
}

void BankEditorDialog::deviceRemoved(Device* device)
{
    RG_DEBUG << "deviceRemoved" << device;
    unobserveDevice(device);
    updateDialog();
}

void BankEditorDialog::deviceModified(Device* device)
{
    RG_DEBUG << "deviceModified" << device;
    updateDialog();
}

void BankEditorDialog::observeDevice(Device* device)
{
    RG_DEBUG << "observeDevice" << device;
    if (m_observedDevices.find(device) != m_observedDevices.end()) return;
    m_observedDevices.insert(device);
    device->addObserver(this);
}

void BankEditorDialog::unobserveDevice(Device* device)
{
    RG_DEBUG << "unobserveDevice" << device;
    if (m_observedDevices.find(device) == m_observedDevices.end()) return;
    m_observedDevices.erase(device);
    device->removeObserver(this);
}

}
