// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef ROSEGARDENGUI_H
#define ROSEGARDENGUI_H
 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstrlist.h>

// include files for KDE 
#include <kapp.h>
#include <kmainwindow.h>
#include <kaccel.h>

#include "rosegardendcop.h"
#include "rosegardenguiiface.h"
#include "rosegardentransportdialog.h"
#include "segmentcanvas.h"

// the sequencer interface
//
#include <MappedEvent.h>
#include "Sound.h"

class KURL;
class KRecentFilesAction;
class KToggleAction;
class KProcess;

// forward declaration of the RosegardenGUI classes
class RosegardenGUIDoc;
class RosegardenGUIView;

namespace Rosegarden { class SequenceManager; }

/**
  * The base class for RosegardenGUI application windows. It sets up the main
  * window and reads the config file as well as providing a menubar, toolbar
  * and statusbar. An instance of RosegardenGUIView creates your center view, which is connected
  * to the window's Doc object.
  * RosegardenGUIApp reimplements the methods that KTMainWindow provides for main window handling and supports
  * full session management as well as keyboard accelerator configuration by using KAccel.
  * @see KTMainWindow
  * @see KApplication
  * @see KConfig
  * @see KAccel
  *
  * @author Source Framework Automatically Generated by KDevelop, (c) The KDevelop Team.
  * @version KDevelop version 0.4 code generation
  */
class RosegardenGUIApp : public KMainWindow, virtual public RosegardenIface
{
  Q_OBJECT

  friend class RosegardenGUIView;

public:

    /**
     * construtor of RosegardenGUIApp, calls all init functions to
     * create the application.
     * @see initMenuBar initToolBar
     */
    RosegardenGUIApp();

    virtual ~RosegardenGUIApp();

    /**
     * opens a file specified by commandline option
     */
    void openDocumentFile(const char *_cmdl=0);

    /**
     * returns a pointer to the current document connected to the
     * KTMainWindow instance and is used by * the View class to access
     * the document object's methods
     */	
    RosegardenGUIDoc *getDocument() const; 	

    /**
     * open a file
     */
    virtual void openFile(const QString& url);

    /**
     * Works like openFile but is able to open remote files
     */
    void openURL(const KURL& url);

    /**
     * imports a Rosegarden 2.1 file
     */
    virtual void importRG21File(const QString &url);

    /**
     * imports a MIDI file
     */
    virtual void importMIDIFile(const QString &url);

    /**
     * export a MIDI file
     */
    virtual void exportMIDIFile(const QString &url);

    /**
     * The Sequencer calls this method to get a MappedCompositon
     * full of MappedEvents for it to play.
     */
    Rosegarden::MappedComposition
            getSequencerSlice(long sliceStartSec, long sliceStartUSec,
                              long sliceEndSec, long sliceEndUSec);

    /**
     * The Sequencer sends back a MappedComposition full of
     * any MappedEvents that it's recorded.
     *
     */
    void processRecordedMidi(const Rosegarden::MappedComposition &mC);


    /**
     * Process unexpected MIDI events for the benefit of the GUI
     *
     */
    void processAsynchronousMidi(const Rosegarden::MappedComposition &mC);

    /**
     *
     * Query the sequencer to find out if the sound systems initialised
     * correctly
     *
     */
    bool getSoundSystemStatus();

protected:

    /**
     * Overridden virtuals for Qt drag 'n drop (XDND)
     */
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */ 	
    void saveOptions();

    /**
     * read general Options again and initialize all variables like
     * the recent file list
     */
    void readOptions();

    /**
     * create menus and toolbars
     */
    void setupActions();

    /**
     * sets up the statusbar for the main window by initialzing a
     * statuslabel.
     */
    void initStatusBar();

    /**
     * initializes the document object of the main window that is
     * connected to the view in initView().
     * @see initView();
     */
    void initDocument();

    /**
     * creates the centerwidget of the KTMainWindow instance and sets
     * it as the view
     */
    void initView();

    /**
     * queryClose is called by KTMainWindow on each closeEvent of a
     * window. Against the default implementation (only returns true),
     * this calles saveModified() on the document object to ask if the
     * document shall be saved if Modified; on cancel the closeEvent
     * is rejected.
     *
     * @see KTMainWindow#queryClose
     * @see KTMainWindow#closeEvent
     */
    virtual bool queryClose();

    /**
     * queryExit is called by KTMainWindow when the last window of the
     * application is going to be closed during the closeEvent().
     * Against the default implementation that just returns true, this
     * calls saveOptions() to save the settings of the last window's
     * properties.
     *
     * @see KTMainWindow#queryExit
     * @see KTMainWindow#closeEvent
     */
    virtual bool queryExit();

    /**
     * saves the window properties for each open window during session
     * end to the session config file, including saving the currently
     * opened file by a temporary filename provided by KApplication.
     *
     * @see KTMainWindow#saveProperties
     */
    virtual void saveProperties(KConfig *_cfg);

    /**
     * reads the session config file and restores the application's
     * state including the last opened files and documents by reading
     * the temporary files saved by saveProperties()
     *
     * @see KTMainWindow#readProperties
     */
    virtual void readProperties(KConfig *_cfg);

    /*
     * Send the result of getSequencerSlice (operated by the
     * Sequencer) to the GUI so as to get visual representation
     * of the events/sounds going out
     */
    void showVisuals(const Rosegarden::MappedComposition &mC);

    /*
     * place clicktrack events into the global MappedComposition
     *
     */
    void insertMetronomeClicks(Rosegarden::timeT sliceStart,
			       Rosegarden::timeT sliceEnd);

public slots:
    /**
     * open a new application window by creating a new instance of
     * RosegardenGUIApp
     */
    void fileNewWindow();

    /**
     * clears the document in the actual view to reuse it as the new
     * document
     */
    virtual void fileNew();

    /**
     * open a file and load it into the document
     */
    void fileOpen();

    /**
     * opens a file from the recent files menu
     */
    void fileOpenRecent(const KURL&);

    /**
     * save a document
     */
    virtual void fileSave();

    /**
     * save a document by a new filename
     */
    void fileSaveAs();

    /**
     * asks for saving if the file is modified, then closes the actual
     * file and window
     */
    virtual void fileClose();

    /**
     * print the actual file
     */
    void filePrint();

    /**
     * Let the user select a MIDI file for import
     */
    void importMIDI();

    /**
     * Let the user select a Rosegarden 2.1 file for import 
     */
    void importRG21();


    /**
     * Let the user enter a MIDI file to export to
     */
    void exportMIDI();

    /**
     * closes all open windows by calling close() on each memberList
     * item until the list is empty, then quits the application.  If
     * queryClose() returns false because the user canceled the
     * saveModified() dialog, the closing breaks.
     */
    virtual void quit();
    
    /**
     * put the marked text/object into the clipboard and remove * it
     * from the document
     */
    void editCut();

    /**
     * put the marked text/object into the clipboard
     */
    void editCopy();

    /**
     * paste the clipboard into the document
     */
    void editPaste();

    /**
     * toggles the toolbar
     */
    void toggleToolBar();

    /**
     * toggles the transport window
     */
    void toggleTransport();

    /**
     * toggles the tracks toolbar
     */
    void toggleTracksToolBar();

    /**
     * toggles the statusbar
     */
    void toggleStatusBar();

    /**
     * changes the statusbar contents for the standard label
     * permanently, used to indicate current actions.
     *
     * @param text the text that is displayed in the statusbar
     */
    void statusMsg(const QString &text);

    /**
     * changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menuentries.
     *
     * @param text the text that is displayed in the statusbar
     */
    void statusHelpMsg(const QString &text);

    /**
     * segment select tool
     */
    void pointerSelected();

    /**
     * segment eraser tool is selected
     */
    void eraseSelected();
    
    /**
     * segment draw tool is selected
     */
    void drawSelected();
    
    /**
     * segment move tool is selected
     */
    void moveSelected();

    /**
     * segment resize tool is selected
     */
    void resizeSelected();

    /*
     * Segment join tool
     *
     */
    void joinSelected();

    /*
     * Segment split tool
     *
     */
    void splitSelected();

    /**
     * change the resolution of the segment display
     */
    void changeTimeResolution();

    /**
     * edit all tracks at once
     */
    void editAllTracks();

    /**
     * Set the song position pointer - we use longs so that
     * this method is directly accesible from the sequencer
     * (longs are required over DCOP)
     */
    void setPointerPosition(Rosegarden::RealTime time);
    void setPointerPosition(const long &posSec, const long &posUSec);

    /**
     * Set the pointer position and start playing (from LoopRuler)
     */
    void setPlayPosition(Rosegarden::timeT position);

    /**
     * Set a loop
     */
    void setLoop(Rosegarden::timeT lhs, Rosegarden::timeT rhs);

    /**
     * timeT version of the same
     */
    void setPointerPosition(Rosegarden::timeT t);


    /**
     * Update the transport with the bar, beat and unit times for
     * a given timeT
     */
    void displayBarTime(Rosegarden::timeT t);


    /**
     * Transport controls
     */
    void play();
    void stop();
    void rewind();
    void fastforward();
    void record();
    void rewindToBeginning();
    void fastForwardToEnd();
    void refreshTimeDisplay();

    /**
     * Set the sequencer status - pass through DCOP as an int
     */
    void notifySequencerStatus(const int &status);


    /**
     * Convenience function for sending positional updates to the
     * sequencer if we're ffwding, rwding or just jumping about on the
     * Composition.
     */
    void sendSequencerJump(const Rosegarden::RealTime &position);

    /**
     * Called when the sequencer auxiliary process exits
     */
    void sequencerExited(KProcess*);

    /**
     * Start the sequencer auxiliary process
     * (built in the 'sequencer' directory)
     *
     * @see sequencerExited()
     */
    bool launchSequencer();

    // When the transport closes 
    //
    void closeTransport();

    // Put the GUI into a given ToolType edit mode
    //
    void activateTool(SegmentCanvas::ToolType tt);

    /**
     * Toggles either the play or record metronome according
     * to Transport status
     */
    void toggleMetronome();

private:

    //--------------- Data members ---------------------------------

    /**
     * the configuration object of the application
     */
    KConfig* m_config;

    KRecentFilesAction* m_fileRecent;

    /**
     * view is the main widget which represents your working area. The
     * View class should handle all events of the view widget.  It is
     * kept empty so you can create your view according to your
     * application's needs by changing the view class.
     */
    RosegardenGUIView* m_view;

    /**
     * doc represents your actual document and is created only
     * once. It keeps information such as filename and does the
     * serialization of your files.
     */
    RosegardenGUIDoc* m_doc;

    /**
     * KAction pointers to enable/disable actions
     */
//     KAction* m_fileNewWindow;
    KAction* m_fileNew;
    KAction* m_fileOpen;
    KRecentFilesAction* m_fileOpenRecent;
    KAction* m_fileSave;
    KAction* m_fileSaveAs;
    KAction* m_fileClose;
    KAction* m_filePrint;
    KAction* m_fileQuit;
    KAction* m_editCut;
    KAction* m_editCopy;
    KAction* m_editPaste;
    KToggleAction* m_viewToolBar;
    KToggleAction* m_viewTracksToolBar;
    KToggleAction* m_viewStatusBar;
    KToggleAction* m_viewTransport;
    KAction *m_playTransport;
    KAction *m_stopTransport;
    KAction *m_rewindTransport;
    KAction *m_ffwdTransport; 
    KAction *m_recordTransport;
    KAction *m_rewindEndTransport;
    KAction *m_ffwdEndTransport;

    KProcess* m_sequencerProcess;

    // SequenceManager
    //
    Rosegarden::SequenceManager *m_seqManager;

    // Transport dialog pointer
    //
    Rosegarden::RosegardenTransportDialog *m_transport;
};
 
#endif // ROSEGARDENGUI_H
