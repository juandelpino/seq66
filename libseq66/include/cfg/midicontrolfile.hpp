#if ! defined SEQ66_MIDICONTROLFILE_HPP
#define SEQ66_MIDICONTROLFILE_HPP

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
 * \file          midicontrolfile.hpp
 *
 *  This module declares/defines the base class for managing the MIDI control
 *  configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-13
 * \updates       2019-06-19
 * \license       GNU GPLv2 or above
 *
 */

#include <iostream>                     /* std::cout, ifstream, ofstream    */
#include <map>                          /* std::map<>                       */

#include "cfg/configfile.hpp"           /* seq66::configfile base class     */
#include "ctrl/keycontainer.hpp"        /* seq66::keycontainer class        */
#include "ctrl/midicontainer.hpp"       /* seq66::midicontainer class       */
#include "ctrl/midicontrolout.hpp"      /* seq66::midicontrolout class      */
#include "ctrl/opcontrol.hpp"           /* seq66::optcontrol and automation */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides a file for reading and writing the application's main
 *  configuration file.  The settings that are passed around are provided
 *  or used by the performer class.
 */

class midicontrolfile final : public configfile
{

    friend class rcfile;

    /*
     *  This class provides an alternate key for a midicontrol useful to write
     *  out the controls in the proper order for a configuration file.
     */

    class key
    {
    private:

        automation::category m_category;    /* for grouping into sections   */
        int m_slot_control;                 /* pattern/group number, slot   */

    public:

        key (const midicontrol & mc);
        bool operator < (const key & rhs) const;

        std::string category_name () const
        {
            return opcontrol::category_name(m_category);
        }

        int slot_control () const
        {
            return m_slot_control;
        }

    };          // midicontrolfile::key

    class stanza
    {
    private:

        automation::category m_category;
        std::string m_key_name;
        std::string m_op_name;
        int m_slot_number;
        int m_settings[automation::ACTCOUNT][automation::SUBCOUNT];

    public:

        stanza (const midicontrol & mc);
        bool set (const midicontrol & mc);

        automation::category category_code () const
        {
            return m_category;
        }

        std::string category_name () const
        {
            return opcontrol::category_name(m_category);
        }

        std::string key_name () const
        {
            return m_key_name;
        }

        std::string op_name () const
        {
            return m_op_name;
        }

        int slot_number () const
        {
            return m_slot_number;
        }

        int setting (unsigned action, unsigned part) const
        {
            return action < automation::ACTCOUNT && part < automation::SUBCOUNT ?
                m_settings[action][part] : 0 ;
        }

    };              // midicontrolfile::stanza

public:

    using storage = std::map<key, stanza>;

private:

    /**
     *  Provides a default-filled keycontrol container.
     */

    keycontainer m_temp_key_controls;

    /**
     *  Provides a default-filled midicontrol container.
     */

    midicontainer m_temp_midi_controls;

    /**
     *  Provides the storage for the mute-groups data.
     */

    storage m_stanzas;

    /**
     *  If true, even inactive (all zero/false) stanzas are saved.
     */

    bool m_allow_inactive;

public:

    midicontrolfile () = delete;
    midicontrolfile
    (
        const std::string & filename,
        rcsettings & rcs,
        bool allowinactive = false
    );
    midicontrolfile (const midicontrolfile &) = delete;
    midicontrolfile & operator = (const midicontrolfile &) = delete;
    midicontrolfile (midicontrolfile &&) = default;
    midicontrolfile & operator = (midicontrolfile &&) = default;
    virtual ~midicontrolfile ();

    virtual bool parse () override;
    virtual bool write () override;

    bool parse_stream (std::ifstream & file);
    bool write_stream (std::ofstream & file);

public:

    bool container_to_stanzas (const midicontainer & mc);
    void show_stanzas () const;

protected:

private:

    bool parse_control_stanza (automation::category opcategory);
    bool parse_midi_control_out (std::ifstream & file);
    void show_stanza (const stanza & stan) const;

    /**
     * \getter m_stanzas
     */

    storage & stanzas ()
    {
        return m_stanzas;
    }

    bool write_midi_control (std::ofstream & file);
    bool write_midi_control_out (std::ofstream & file);
    void read_ctrl_event
    (
        std::ifstream & file,
        midicontrolout & mctrl,
        midicontrolout::action a
    );
    void write_ctrl_event
    (
        std::ofstream & file,
        midicontrolout & mctrl,
        midicontrolout::action a
    );
    void read_ctrl_pair
    (
        std::ifstream & file,
        midicontrolout & mctrl,
        midicontrolout::action a1,
        midicontrolout::action a2
    );
    void write_ctrl_pair
    (
        std::ofstream & file,
        const midicontrolout & mctrl,
        midicontrolout::action a1,
        midicontrolout::action a2
    );

};              // class midicontrolfile

}               // namespace seq66

#endif          // SEQ66_MIDICONTROLFILE_HPP

/*
 * midicontrolfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

