% This LilyPond file was generated by Rosegarden 23.06
\include "nederlands.ly"
\version "2.12.0"
\header {
    arranger = "Arranger"
    composer = "Composer"
    copyright = "© Copyright"
    dedication = "Dedication"
    instrument = "Instrument"
    meter = "Meter"
    opus = "Opus"
    piece = "Piece"
    poet = "Poet"
    subsubtitle = "Subsubtitle"
    subtitle = "Subtitle"
    tagline = "Tagline"
    title = "Title"
}
#(set-global-staff-size 18)
#(set-default-paper-size "a4")
global = { 
    \time 2/4
    \skip 2*4 
}
globalTempo = {
    \override Score.MetronomeMark #'transparent = ##t
    \tempo 4 = 120  
}
\score {
    << % common
        % Force offset of colliding notes in chords:
        \override Score.NoteColumn #'force-hshift = #1.0
        % Allow fingerings inside the staff (configured from export options):
        \override Score.Fingering #'staff-padding = #'()

        \context Staff = "track 1" << 
            \set Staff.midiInstrument = "Acoustic Grand Piano"
            \set Score.skipBars = ##t
            \set Staff.printKeyCancellation = ##f
            \new Voice \global
            \new Voice \globalTempo
            \set Staff.autoBeaming = ##f % turns off all autobeaming

            \context Voice = "voice 0.0" {
                % Segment: Acoustic Grand Piano
                \override Voice.TextScript #'padding = #2.0
                \override MultiMeasureRest #'expand-limit = 1
                \once \override Staff.TimeSignature #'style = #'numbered 
                \time 2/4
                
                \clef "treble"
                c' 4 d'  |
                e' 4 d'  |
                c' 2 _~  |
                c' 2  |
                \bar "|."
            } % Voice

            % End of segment Acoustic Grand Piano

            % End voice 0

            % cycle 1   verse line 1
            \new Lyrics
            \with {alignBelowContext="track 1"}
            \lyricsto "voice 0.0" {
                \lyricmode {
                    \override LyricText #'self-alignment-X = #LEFT
                    \set ignoreMelismata = ##t

                    % Segment "Acoustic Grand Piano": verse 1

                    %{   1 %}    This is
                    %{   2 %}    litt -- le
                    %{   3 %}    song.
                    %{   4 %}    _

                    \unset ignoreMelismata
                }
            } % Lyrics 

            % cycle 2   verse line 1
            \new Lyrics
            \lyricsto "voice 0.0" {
                \lyricmode {
                    \override LyricText #'self-alignment-X = #LEFT
                    \set ignoreMelismata = ##t

                    % Segment "Acoustic Grand Piano": verse 2

                    %{   1 %}    Peut -- être
                    %{   2 %}    est -- ce
                    %{   3 %}    "错误 !"
                    %{   4 %}    _

                    \unset ignoreMelismata
                }
            } % Lyrics 
        >> % Staff ends

    >> % notes

    \layout {
        \context { \Staff \RemoveEmptyStaves }
        \context { \GrandStaff \accepts "Lyrics" }
    }
%     uncomment to enable generating midi file from the lilypond source
%         \midi {
%         } 
} % score
