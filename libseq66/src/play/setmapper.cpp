/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          setmapper.cpp
 *
 *  This module declares a class to manage a number of screensets.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-02-12
 * \updates       2019-10-02
 * \license       GNU GPLv2 or above
 *
 *  Implements three classes:  seq, screenset, and setmapper, which replace a
 *  number of data items and functions in the seq66::performer class.
 *
 *  Together, these classes emulate the way Sequencer64 worked, with 32 sets
 *  of 32 patterns, and some extra data buffers to supplement the information
 *  within each sequence object.  The sequence number is still the same, from
 *  0 on up (e.g. to 1023).  The performer object can access sequences by
 *  number, as before, but it no longer has to make calculations for picking
 *  sets and mute-groups, and it can let the setmanager do the looping in a
 *  lot of cases.
 *
 *  Create a new sequence from a file:
 *
 *      -#  midifile::finalize_sequence(p, seqptr, seqno, screenset).
 *          This function calculates the preferred seq number from the number
 *          in the MIDI file and the desired destination screenset.
 *      -#  performer::add_sequence(seqptr, prefseqnum).  We need to keep the
 *          seq and set numbers separate and pass them along.
 *      -#  performer::install_sequence(seqptr, seqno). Ditto.
 *      -#  setmapper::install_sequence(seqptr, seqno, setnum)
 *      -#  screenset::install_sequence(seqptr, seqno).  Let the sequence
 *          the original seqno, and let seq hold the set and modified
 *          seqno.
 *
 *  Create a new sequence from the GUI:
 *
 *      -#  perform::new_sequence(seqno).  Again, the seqno is based on the
 *          old style of numbering, obtained by mouse location and set
 *          selection.
 *
 *  Mute groups:
 *
 *      Mute-group handling in Sequencer64 was perhaps needlessly complex, with
 *      a number of similarly named functions in the performer class.  The
 *      functionality has been offloaded to setmapper and the mutegroups class.
 *      The process should be simple:
 *
 *      -#  Enter learn-mode, then obtain the number of the desired mute-group
 *          from the keyboard or a MIDI control.
 *      -#  Notify all subscribers.
 *      -#  Copy the current armed/muted statuses of the playing screen into the
 *          desired mute-group.
 *      -#  Exit learn-mode and notify all subscribers.
 *      -#  TO BE CONTINUED...
 */

#include <iostream>                     /* std::cout                        */
#include <sstream>                      /* std::stringstream                */

#include "cfg/rcsettings.hpp"           /* c_max_sequence, etc.             */
#include "play/mutegroups.hpp"          /* seq66::mutegroups class          */
#include "play/setmapper.hpp"           /* seq66::setmapper class           */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Creates a manager for all of the sets in a tune, and also the mutegroups.
 *
 *  After creation, screenset 0 is created and set as the play-screen.
 *
 * \param mgs
 *      Provides the existing mutegroup to be managed.
 *
 * \param sets
 *      Provides the maximum number of sets to be supported and managed.
 *
 * \param rows
 *      Provides the number of rows (virtual rows) in each set and mutegroup.
 *      We could use the mutegroups sizes, perhaps.
 *
 * \param columns
 *      Provides the number of columns (virtual columns) in each set and
 *      mutegroup.
 */

setmapper::setmapper
(
    mutegroups & mgs,
    int sets,
    int rows,
    int columns
) :
    m_mute_groups           (mgs),
    m_set_size              (rows * columns),
    m_set_count             (sets),                     /* rows * columns?? */
    m_rows                  (rows),
    m_columns               (columns),
    m_container             (),                         /* screensets map   */
    m_sequence_count        (0),
    m_sequence_max          (m_set_size * m_set_count), /* c_max_sequence   */
    m_sequence_high         (seq::unassigned()),
    m_edit_sequence         (seq::unassigned()),
    m_playscreen            (seq::unassigned()),
    m_playscreen_pointer    (nullptr),
    m_tracks_mute_state     (m_set_size, false)
{
    reset();
}

/**
 *  Resets back to the constructor set.  This means we have one set, the empty
 *  play-screen, plus a "dummy" set.
 */

void
setmapper::reset ()
{
    clear();
    auto setp = add_set(0);
    if (setp != m_container.end())
        (void) set_playscreen(0);

    (void) add_set(screenset::limit());     /* created the dummy set    */
}

/**
 *  Given the raw sequence number, returns the calculated set number and the
 *  offset of the sequence in the set.
 *
 * \param [out] offset
 *      Holds the calculated offset.  It will always be clamped from 0 to
 *      m_set_size.
 *
 * \param seqno
 *      The raw sequence number.  Normally, this value can range from 0 to 1023,
 *      or whatever the maximum is based on set size and number of sets.
 *
 * \return
 *      Returns the calculated set number.  It is clamped to a valid value.
 */

screenset::number
setmapper::seq_set (int & offset, int seqno) const
{
    screenset::number result = clamp(seqno / m_set_size);
    offset = seqno % m_set_size;
    return result;
}

/**
 *  Given the raw sequence number, returns the calculate set number and the
 *  row and column of the sequence in the set.
 *
 * \param [out] row
 *      Holds the calculated row.  Clamped via the mod operator.
 *
 * \param [out] column
 *      Holds the calculated column.  Clamped via the mod operator.
 *
 * \param seqno
 *      The raw sequence number.  Normally, this value can range from 0 to 1023,
 *      or whatever the maximum is based on set size and number of sets.
 *
 * \return
 *      Returns the calculated set number.  It is clamped to a valid value.
 */

screenset::number
setmapper::seq_set (int & row, int & column, int seqno) const
{
    screenset::number result = clamp(seqno / m_set_size);
    int offset = seqno - result * m_set_size;
    row = offset / m_columns;
    column = offset % m_columns;
    return result;
}

/**
 *  Returns the set number for the given row and column.  Remember that the
 *  layout of sets matches that of sequences.  See the top banner of the
 *  setmapper.cpp file.  This function is useful for picking the correct set in
 *  the qsetmaster button array.
 *
 * Set Layout:
 *
 *  Like the sequences in the main (live) window, the set numbers are
 *  transposed, so that the set number increments vertically, not horizontally:
 *
\verbatim
    0   4   8   12  16  20  24  28
    1   5   9   13  17  21  25  29
    2   6   10  14  18  22  26  30
    3   7   11  15  19  23  27  31
\endverbatim
 *
 * \param row
 *      Provides the desired row, clamped to a legal value.
 *
 * \param row
 *      Provides the desired row, clamped to a legal value.
 *
 * \return
 *      Returns the calculated set value, which will range from 0 to
 *      (m_rows * m_columns) - 1 = m_set_count.
 */

screenset::number
setmapper::calculate_set (int row, int column) const
{
    if (row < 0)
        row = 0;
    else if (row >= m_rows)
        row = m_rows - 1;

    if (column < 0)
        column = 0;
    else if (column >= m_columns)
        row = m_columns - 1;

    return row + m_rows * column;
}

/**
 *  Creates and adds a screenset to the container.
 *
 *  This code revealed that, when items (screenset, seq, etc.) are to be
 *  stored in containers, it is best to not have const data member.  These
 *  make the assignment operator or constructor ill-formed, so that the
 *  compiler deletes these functions!  Then, calling std::make_pair() results
 *  in very mysterious error messages.
 *
 *  Auto types:
 *
 *      -   std::pair<screenset::number, screenset>
 *      -   std::pair<container::iterator, bool>
 */

setmapper::container::iterator
setmapper::add_set (screenset::number setno)
{
    screenset newset(setno, m_rows, m_columns);
    auto setpair = std::make_pair(setno, newset);
    auto resultpair = m_container.insert(setpair);
    return resultpair.first;
}

/**
 *  Look up the screen to be used, given the sequence number.  If the screen
 *  doesn't exist, and is a legal screenset number, then create it.
 */

screenset &
setmapper::screen (seq::number seqno)
{
    screenset::number s = seq_set(seqno);
    if (m_container.find(s) != m_container.end())
    {
        return m_container.at(s);
    }
    else if (seqno >= 0 && seqno < m_set_count)
    {
        if (seqno < seq::limit())
        {
            auto newset = add_set(s);
            return newset->second;
        }
        else
            return dummy_screenset();
    }
    else
        return dummy_screenset();
}

/**
 *
 */

int
setmapper::screenset_index (screenset::number setno) const
{
    int result = seq::unassigned();
    int index = 0;
    for (const auto & sset : m_container)
    {
        if (sset.second.set_number() == setno)
        {
            result = index;
            break;
        }
        ++index;
    }
    return result;
}

/**
 *  Adds the sequence pointer to the matching screenset.  The caller should have
 *  already made its initial setup of the sequence, and then should relinquish
 *  ownership of the pointer.
 *
 *  If the screenset does not already exist, it is created and added to the list
 *  of screensets the setmapper manages.  Then the sequence is added.
 *
 * \param s
 *      Provides the sequencer pointer, which is rejected if null.
 *
 * \param seqno
 *      The desired sequence number, which is a linear value ranging from 0 to
 *      a number determined by the active set size multiplied by the number of
 *      sets.  It is merely a starting point, and will be incremented until an
 *      unused sequence number is found.
 *
 * \return
 *      Returns true if the sequence pointer was able to be added to the
 *      screenset.  Null pointers to sequence are not added.
 *      False can be returned if the sequence number already exists
 *      in the screenset.
 */

bool
setmapper::add_sequence (sequence * s, int seqno)
{
    bool result = false;
    if (not_nullptr(s))
    {
        // screenset::number setno = seq_set(seqno);
        screenset & sset = screen(seqno);                   /* tricky !!!   */
        while (! result)
        {
            result = sset.usable();
            if (result)
            {
                result = sset.add(s, seqno);
            }
            else
            {
                /*
                 * This should never happen here; taken care of in
                 * install_sequence()!
                 *
                 *      auto setp = add_set(setno);
                 *      result = setp != m_container.end();
                 *      if (result)
                 *          result = setp->second.add(s, seqno);
                 */
            }
            if (! result)
            {
                ++seqno;
                if (seqno == m_sequence_max)
                    break;
            }
        }
        if (result)
        {
            ++m_sequence_count;
            if (seqno >= m_sequence_high)
                m_sequence_high = seqno + 1;    /* no way to back out, tho */
        }
    }
    return result;
}

/**
 *
 */

bool
setmapper::sets_function (screenset::sethandler s)
{
    bool result = false;
    screenset::number index = 0;
    for (auto & sset : m_container)                 /* screenset reference  */
    {
        if (sset.second.usable())
        {
            result = sset.second.sets_function(s, index++);
            if (! result)
                break;
        }
    }
    return result;
}

/**
 *
 */

bool
setmapper::sets_function (screenset::sethandler s, screenset::slothandler p)
{
    bool result = false;
    for (auto & sset : m_container)                 /* screenset reference  */
    {
        if (sset.second.usable())
        {
            result = sset.second.sets_function(s, p);
            if (! result)
                break;
        }
    }
    return result;
}

/**
 *
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
setmapper::set_dirty (seq::number seqno)
{
    if (seqno == seq::all())
    {
        for (auto & sset : m_container)         /* screenset reference  */
            sset.second.set_dirty();
    }
    else
    {
        screenset::number setno = seq_set(seqno);
        auto setiterator = m_container.find(setno);
        if (setiterator != m_container.end())
            setiterator->second.set_dirty(seqno);
    }
}

/**
 *
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
setmapper::toggle (seq::number seqno)
{
    if (seqno == seq::all())
    {
        for (auto & sset : m_container)         /* screenset reference  */
            sset.second.toggle();
    }
    else
    {
        screenset::number setno = seq_set(seqno);
        auto setiterator = m_container.find(setno);
        if (setiterator != m_container.end())
            setiterator->second.toggle(seqno);
    }
}

/**
 *  Toggles the mutes status of all playing (currently unmuted) tracks in the
 *  current set of active patterns/sequences on all screen-sets.  Covers
 *  tracks from 0 to m_sequence_max.  The statuses are preserved for
 *  restoration.
 *
 *  If no sequences are armed, then turn them all on, as a convenience to the
 *  user.
 *
 *  Note that this function should operate only in Live mode; it is too confusing to
 *  use in Song mode.
 */

void
setmapper::toggle_playing_tracks ()
{
    if (armed())                          /* any seqs armed?  */
    {
        if (m_armed_saved)
        {
            m_armed_saved = false;
            apply_armed_statuses();
        }
        else
            m_armed_saved = learn_armed_statuses();
    }
    else
        mute();
}

/**
 *
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
setmapper::toggle_song_mute (seq::number seqno)
{
    if (seqno == seq::all())
    {
        for (auto & sset : m_container)         /* screenset reference  */
            sset.second.toggle_song_mute();
    }
    else
    {
        screenset::number setno = seq_set(seqno);
        auto setiterator = m_container.find(setno);
        if (setiterator != m_container.end())
            setiterator->second.toggle_song_mute(seqno);
    }
}

/**
 *  Installs the sequence/track/loop.  The performer class does a lot of the
 *  work first, and also does the modification check, so it is not done here.
 *  However, if the sequence requires a new set to be created, we do it here.
 *
 * \param s
 *      Provides a pointer to the sequence to be installed.
 *
 * \param seqno
 *      Provides the number of the sequence, which also determines what set it
 *      it is in.
 *
 * \return
 *      Returns true if the sequence was added successfully.
 */

bool
setmapper::install_sequence (sequence * s, seq::number seqno)
{
    bool result = true;
    screenset::number setno = seq_set(seqno);
    auto setiterator = m_container.find(setno);
    if (setiterator == m_container.end())
    {
        auto setp = add_set(setno);
        result = setp != m_container.end();
    }
#if 0
    else
    {
        seq::pointer sp = loop(seqno);
        if (sp)
            (void) remove_sequence(seqno);
    }
#endif
    if (result)
        result = add_sequence(s, seqno);

    return result;
}

/**
 *  Removes a sequence from the tune, based on its sequence number.  It will not
 *  be removed if it is in editing.
 *
 * \param seqno
 *      Provides the sequence number, used to look up any existing sequences.
 *      All sequences have a unique number ranging from 0 to 1023.  Also, please
 *      note that the setmapper does lookups, including of screensets, using
 *      this range of numbers, rather than set number.
 *
 * \return
 *      Returns true if the sequence did exist and was removed successfully.
 *      If true, the caller should decrement its sequence count.
 */

bool
setmapper::remove_sequence (int seqno)
{
    screenset & sset = screen(seqno);       /* tricky                       */
    bool result = ! sset.usable();          /* doesn't exist, we're golden  */
    if (! result)
    {
        result = sset.remove(seqno);        /* it exists, remove it!        */
        if (result)
            --m_sequence_count;
    }
    return result;
}

/**
 *  Does a brute-force lookup of the given set number, obtained by
 *  screenset::set_number().
 */

setmapper::container::iterator
setmapper::find_by_value (screenset::number setno)
{
#if defined THIS_CODE_WORKS                     // hint: it does not
    container::iterator result = std::find_if
    (
        m_container.begin(), m_container.end(),
        [setno] (std::pair<screenset::number, screenset> & p) -> bool
        {
            return p.second.set_number() == setno;
        }
    );
#else
    auto result = m_container.end();
    for (auto it = m_container.begin(); it != m_container.end(); ++it)
    {
        if (it->second.set_number() == setno)
        {
            result = it;
            break;
        }
    }
#endif
    return result;
}

/**
 * \tricky
 *      For use in the qsetmaster set-list, we need to look up the iterator to a
 *      set by value, not by key, because after the first swap there is no
 *      longer a correspondence between the key and the actual set-number value.
 */

bool
setmapper::swap_sets (seq::number set0, seq::number set1)
{
    auto item0 = find_by_value(set0);
    auto item1 = find_by_value(set1);
    bool result = item0 != m_container.end() && item1 != m_container.end();
    if (result)
    {
        std::swap(item0->second, item1->second);
        item0->second.change_set_number(set1);      /* also changes seq #s  */
        item1->second.change_set_number(set0);      /* also changes seq #s  */
    }
    return result;
}

/**
 *  Learns the current armed status, and also toggles them.
 *
 * \return
 *      Returns true if an armed status was found.
 */

bool
setmapper::learn_armed_statuses ()
{
    bool result = false;
    for (auto & sset : m_container)                     /* screenset ref    */
    {
        bool ok = sset.second.learn_armed_statuses();
        if (ok)
            result = true;
    }
    return result;
}

/**
 *
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
setmapper::apply_song_transpose (seq::number seqno)
{
    if (seqno == seq::all())
    {
        for (auto & sset : m_container)         /* screenset reference  */
            sset.second.apply_song_transpose();
    }
    else
    {
        screenset::number setno = seq_set(seqno);
        auto setiterator = m_container.find(setno);
        if (setiterator != m_container.end())
            setiterator->second.apply_song_transpose(seqno);
    }
}

/*
 * -------------------------------------------------------------------------
 * Triggers
 * -------------------------------------------------------------------------
 */

/**
 *  Locates the largest trigger value among the active sequences.
 *
 * \return
 *      Returns the highest trigger value, or zero.  It is not clear why
 *      this function doesn't return a "no trigger found" value. Is there
 *      always at least one trigger, at 0?
 */

midipulse
setmapper::max_trigger () const
{
    midipulse result = 0;
    for (const auto & sset : m_container)
    {
        midipulse t = sset.second.max_trigger();
        if (t > result)
            result = t;
    }
    return result;
}

/**
 *
 * \param seqlow
 *      Provides the low track to be selected.
 *
 * \param seqhigh
 *      Provides the high track to be selected.  If not in the same set, nothing
 *      is done.  We need a way to report that.
 *
 * \param tick_start
 *      Provides the low end of the box.
 *
 * \param tick_finish
 *      Provides the high end of the box.
 */

void
setmapper::select_triggers_in_range
(
    seq::number seqlow, seq::number seqhigh,
    midipulse tick_start, midipulse tick_finish
)
{
    screenset::number setlow = seq_set(seqlow);
    screenset::number sethigh = seq_set(seqhigh);
    if (setlow == sethigh)
    {
        auto setiterator = m_container.find(setlow);
        if (setiterator != m_container.end())
        {
            setiterator->second.select_triggers_in_range
            (
                seqlow, seqhigh, tick_start, tick_finish
            );
        }
    }
    else
    {
        // What to do?
    }
}

/**
 *
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
setmapper::unselect_triggers (seq::number seqno)
{
    if (seqno == seq::all())
    {
        for (auto & sset : m_container)         /* screenset reference  */
            sset.second.unselect_triggers();
    }
    else
    {
        screenset::number setno = seq_set(seqno);
        auto setiterator = m_container.find(setno);
        if (setiterator != m_container.end())
            setiterator->second.unselect_triggers(seqno);
    }
}

/**
 *  If the left tick is less than the right tick, then, for each sequence
 *  that is active, its triggers are moved by the difference between the
 *  right and left in the specified direction.
 *
 * \param lefftick
 *      Provides the low of the box.
 *
 * \param righttick
 *      Provides the high end of the box.
 *
 * \param direction
 *      The direction of the trigger move.  False is leftward, and true is
 *      rightward.
 *
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
setmapper::move_triggers
(
    midipulse lefttick, midipulse righttick,
    bool direction, seq::number seqno
)
{
    if (righttick > lefttick)
    {
        midipulse distance = righttick - lefttick;
        if (seqno == seq::all())
        {
            for (auto & sset : m_container)     /* screenset reference  */
                sset.second.move_triggers(lefttick, distance, direction);
        }
        else
        {
            screenset::number setno = seq_set(seqno);
            auto setiterator = m_container.find(setno);
            if (setiterator != m_container.end())
            {
                setiterator->second.move_triggers
                (
                    lefttick, distance, direction, seqno
                );
            }
        }
    }
}

/**
 *  If the left tick is less than the right tick, then, for each sequence that
 *  is active, its triggers are copied, offset by the difference between the
 *  right and left.  This copies the triggers between the L marker and R
 *  marker to the R marker.
 *
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
setmapper::copy_triggers
(
    midipulse lefttick, midipulse righttick, seq::number seqno
)
{
    if (righttick > lefttick)
    {
        midipulse distance = righttick - lefttick;
        if (seqno == seq::all())
        {
            for (auto & sset : m_container)     /* screenset reference  */
                sset.second.copy_triggers(lefttick, distance);
        }
        else
        {
            screenset::number setno = seq_set(seqno);
            auto setiterator = m_container.find(setno);
            if (setiterator != m_container.end())
                setiterator->second.copy_triggers(lefttick, distance, seqno);
        }
    }
}

/*
 * -------------------------------------------------------------------------
 * Play-screen
 * -------------------------------------------------------------------------
 */

/**
 *  If the desired play-screen exists, unmark the current play-screen and mark
 *  the new one.
 *
 *  If there was a existing screen-set \a setno, just mark it (again).  [DO WE
 *  NEED TO CLEAR IT?]  Otherwise, if the set number is valid, then create a new
 *  screenset and set it as the play-screen.
 *
 *  We no longer check for a change in m_playscreen, because doing so leads to a
 *  segfault... bad set?
 *
 * \param setno
 *      Provides the desired set number.  This ranges from 0 to 2047, though
 *      generally the number of sets is 32 or lower.  There is a screenset
 *      #2048 that always exists in order to provide an inactive/dummy
 *      screenset.
 *
 * \return
 *      Returns true if the play-screen was able to be set.
 */

bool
setmapper::set_playscreen (screenset::number setno)
{
    bool result = setno >= 0 && setno < screenset::limit();
    if (result)
    {
        auto sset = m_container.find(setno);
        result = false;
        if (sset != m_container.end())
        {
            auto oldset = m_container.find(m_playscreen);
            if (oldset != m_container.end())
                oldset->second.is_playscreen(false);

            m_playscreen = setno;
            sset->second.is_playscreen(true);
            result = true;
        }
        else
        {
            auto setp = add_set(setno);
            if (setp != m_container.end())
            {
                set_playscreen(setno);
                setp->second.is_playscreen(true);
                result = true;
            }
        }
        if (! result)
            m_playscreen = 0;           /* use the always-present set 0 */

        m_playscreen_pointer = &m_container.at(m_playscreen);
    }
    return result;
}

/**
 *  Sets the screen-set that is active, based on the value of m_screenset.
 *  This function is called when one of the snapshot keys is pressed.
 *
 *  For each value up to m_seqs_in_set (32), the index of the current sequence
 *  in the current screen-set is obtained.  If the sequence
 *  is active and the sequence actually exists, it is processed; null
 *  sequences are no longer flagged as an error, they are just ignored.
 *
 *  Modifies the playscreen, stores the current playing-status of each sequence,
 *  and calls turns on unmuted tracks in the current screen-set.
 *  Basically, this function retrieves and saves the playing status of the
 *  sequences in the current play-screen, sets the play-screen to the current
 *  screen-set, and then mutes the previous play-screen.  It is called via the
 *  c_midi_control_play_ss value or via the set-playing-screen-set keystroke.
 */

bool
setmapper::set_playing_screenset (screenset::number setno)
{
    bool result = set_playscreen(setno);
    if (result)
    {
        result = play_screen()->learn_bits(m_tracks_mute_state);
        if (result)
            mute_group_tracks();
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 * Mutes
 * -------------------------------------------------------------------------
 */

/**
 *  Applies a mute group to the current play-screen.
 */

bool
setmapper::apply_mutes (mutegroup::number group)
{
    auto mgiterator = mutes().list().find(clamp_group(group));
    bool result = mgiterator != mutes().list().end();
    if (result)
        result = play_screen()->apply_bits(mgiterator->second.get());

    return result;
}

/**
 *  Sets the statuses of a mute group to the sequence statuses of the
 *  current play-screen.
 *
 *  If in group-learn mode, this function gets the playing statuses
 *  of all of the sequences in the current play-screen, and copies them into
 *  the desired mute-group.  Then, no matter what, it makes the desired
 *  mute-group the selected mute-group.
 */

bool
setmapper::learn_mutes (bool learnmode, mutegroup::number group)
{
    bool result = learnmode;
    if (result)
    {
        auto mgiterator = mutes().list().find(clamp_group(group));
        bool result = mgiterator != mutes().list().end();
        if (result)
        {
            midibooleans bits;
            result = play_screen()->learn_bits(bits);
            if (result)
            {
                mgiterator->second.set(bits);
                mutes().group_selected(group);
                mutes().group_learn(true);
                m_tracks_mute_state = bits;     /* save playscreen vector   */
            }
        }
    }
    return result;
}

/**
 *  Turn the playing of a sequence on or off.  Used for the implementation of
 *  sequence_playing_on() and sequence_playing_off().
 *
 *  Kepler34's version seems slightly different, may need more study.
 *
 * \param seqno
 *      The sequence number to be affected.
 *
 * \param on
 *      True if the sequence is to be turned on, false if it is to be turned
 *      off.
 *
 * \param qinprogress
 *      Indicates if queuing is in progress.  This status is obtained from the
 *      performer.
 */

void
setmapper::sequence_playing_change
(
    seq::number seqno,
    bool on,
    bool qinprogress
)
{
    int offset;
    screenset::number setno = seq_set(offset, seqno);
    auto setiterator = m_container.find(setno);
    if (setiterator != m_container.end())
    {
        setiterator->second.sequence_playing_change(seqno, on, qinprogress);
        if (setiterator->second.is_playscreen())
            m_tracks_mute_state[offset] = on;
    }
}

/**
 *  A play-screen-specific version of sequence_playing_change().
 */

void
setmapper::sequence_playscreen_change
(
    seq::number seqno,
    bool on,
    bool qinprogress
)
{
    int offset;
    screenset::number setno = seq_set(offset, seqno);
    if (setno == play_screen()->set_number())
    {
        play_screen()->sequence_playing_change(seqno, on, qinprogress);
        m_tracks_mute_state[offset] = on;
    }
}

/**
 *  Also note that screenset::is_seq_in_edit() can be used!!!
 */

bool
setmapper::is_seq_in_edit (seq::number seqno) const
{
    seq::pointer sp = loop(seqno);
    bool result = bool(sp);
    if (result)
        result = sp->get_editing();

    return result;
}

/**
 *  If group_mode() is true, then this function operates.  It loops through
 *  every screen-set.  In each screen-set, it acts on each active sequence.
 *  If the active sequence is in the current "in-view" screen-set (m_screenset
 *  as opposed to m_playscreen, and its m_track_mute_state[] is true,
 *  then the sequence is turned on, otherwise it is turned off.  The result is
 *  that the in-view screen-set is activated as per the mute states, while all
 *  other screen-sets are muted.
 *
 * \change tdeagan 2015-12-22 via git pull.
 *      Replaced m_playscreen with m_screenset.
 *
 *  We are disabling Tim's fix for a bit in order to make sure we're doing
 *  what Seq24 does with the playing set key. (Home).
 *
 *  It seems to us that the for (g) clause should have g range from 0 to
 *  m_max_sets, not m_seqs_in_set.  Done.
 */

void
setmapper::mute_group_tracks ()
{
    if (group_mode())
    {
        for (auto & sset : m_container)
        {
            bool pscreen = sset.second.is_playscreen();
            int seqoffset = sset.second.offset();
            for (int s = 0; s < m_set_size; ++s)
            {
                int seqno = seqoffset + s;
                if (is_seq_active(seqno))           /* semi-redundant check */
                {
                    bool on = pscreen && m_tracks_mute_state[s];
                    sequence_playing_change(seqno, on);
                }
            }
        }
    }
}

/**
 *  Select a mute group and then mutes the track in the group.  Called in
 *  perform and in mainwnd.
 *
 *  When in group-learn mode, for active sequences, the mute-group settings
 *  are set based on the playing status of each sequence.  Then the
 *  mute-group is stored in m_tracks_mute_state[], which holds states for
 *  only the number of sequences in a set.
 *
 *  Compare to select_group_mute(); its main difference is that it will at
 *  least copy the states even if not in group-learn mode.  And, if in
 *  group-learn mode, it will grab the playing states of the sequences
 *  before copying them.
 *
 *  This function is used only once, in select_and_mute_group().  It used to
 *  be called just select_mute_group(), but that's too easy to confuse with
 *  select_group_mute().
 *
 * \change tdeagan 2015-12-22 via git pull:
 *      git pull https://github.com/TDeagan/sequencer64.git mute_groups
 *      m_screenset replaces m_playscreen_offset.
 *
 * \param mutegroup
 *      Provides the mute-group to select.
 *
 * \param group
 *      Provides the group number for the group to be muted.
 */

void
setmapper::select_and_mute_group (mutegroup::number group)
{
    learn_mutes(mutes().is_group_learn(), group);
    mute_group_tracks();
}

/**
 *  Clears all the group-mute items, whether they came from the "rc" file
 *  or from the most recently-loaded Sequencer64 MIDI file.
 *
 * \sideeffect
 *      If true is returned, the modify flag is set, so that the user has the
 *      option to save a MIDI file that contained mute-groups that are no
 *      longer wanted.
 *
 * \return
 *      Returns true if any of the statuses changed from true to false.
 */

bool
setmapper::clear_mutes ()
{
    bool result = false;
    if (m_mute_groups.clear())
    {
        result = true;          // performer SHOULD CALL modify();
    }
    return result;
}

/**
 *
 */

std::string
setmapper::sets_to_string (bool showseqs) const
{
    std::ostringstream result;
    result << "Sets" << (showseqs ? " and Sequences:" : ":") << std::endl;
    for (auto & s : m_container)
    {
        int keyvalue = int(s.first);
        if (keyvalue < screenset::limit())
        {
            result << "  Key " << int(s.first) << ": ";
            if (s.second.usable())
                result << s.second.to_string(showseqs);
            else
                result << std::endl;
        }
    }
    return result.str();
}

/**
 *
 */

void
setmapper::show (bool showseqs) const
{
    std::cout << sets_to_string(showseqs);
}

}               // namespace seq66

/*
 * setmapper.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

