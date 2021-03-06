#if ! defined SEQ66_QSEQKEYS_HPP
#define SEQ66_QSEQKEYS_HPP

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
 * \file          qseqkeys.hpp
 *
 *  This module declares/defines the base class for the left-side piano of
 *  the pattern/sequence panel.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2019-07-27
 * \license       GNU GPLv2 or above
 *
 *      We've added the feature of a right-click toggling between showing the
 *      main octave values (e.g. "C1" or "C#1") versus the numerical MIDI
 *      values of the keys.
 */

#include <QWidget>

#include "play/seq.hpp"                 /* seq66::seq::pointer              */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

    class performer;

/**
 *  Draws the piano keys in the sequence editor.
 */

class qseqkeys final : public QWidget
{
    Q_OBJECT

    friend class qseqroll;

public:

    qseqkeys
    (
        performer & perf,
        seq::pointer seqp,
        QWidget * parent,
        int keyHeight       = 12,
        int keyAreaHeight   = 12 * c_num_keys + 1
    );

    virtual ~qseqkeys ()
    {
        // no code needed
    }

protected:

    virtual void paintEvent (QPaintEvent *) override;
    virtual void resizeEvent (QResizeEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual QSize sizeHint() const override;

signals:

public slots:

    // void conditional_update ();

private:

    void convert_y (int y, int & note);

    /**
     *  Detects a black key.
     *
     * \param key
     *      The key to analyze.
     *
     * \return
     *      Returns true if the key is black (value 1, 3, 6, 8, or 10).
     */

    bool is_black_key (int key) const
    {
        return key == 1 || key == 3 || key == 6 || key == 8 || key == 10;
    }

    seq::pointer seq_pointer ()
    {
        return m_seq;
    }

private:

    performer & m_performer;
    seq::pointer m_seq;
    QFont m_font;

    /**
     *  The default value is to show the octave letters on the vertical
     *  virtual keyboard.  If false, then the MIDI key numbers are shown
     *  instead.  This is a new feature of Seq66.
     */

    bool m_show_octave_letters;

    bool m_is_previewing;
    int m_key;
    int m_key_y;
    int m_key_area_y;
    int m_preview_key;

};          // class qseqkeys

}           // namespace seq66

#endif      // SEQ66_QSEQKEYS_HPP

/*
 * qseqkeys.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

