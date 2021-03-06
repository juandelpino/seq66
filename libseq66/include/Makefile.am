#******************************************************************************
# Makefile.am (libseq66)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq66 library
# \author     	Chris Ahlstrom
# \date       	2018-11-11
# \update      2019-06-17
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the seq66 C/C++
# 		library.
#
#------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2 subdir-objects
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)
CLEANFILES = *.gc*
MOSTLYCLEANFILES = *~

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = *.h *.hpp

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@
GIT_VERSION = @GIT_VERSION@

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
includedir = @seq66includedir@
libdir = @seq66libdir@
datadir = @datadir@
datarootdir = @datarootdir@
seq66includedir = @seq66includedir@
seq66libdir = @seq66libdir@

#******************************************************************************
# Compiler and linker flags
#------------------------------------------------------------------------------
#
#  $(GTKMM_CFLAGS)
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 $(JACK_CFLAGS) \
 -DSEQ66_GIT_VERSION=\"$(git_info)\" \
 -Wall $(MM_WFLAGS)

#******************************************************************************
# Source files
#----------------------------------------------------------------------------

pkginclude_HEADERS = \
 app_limits.h \
 main_impl.hpp \
 seq66_features.h \
 seq66_features.hpp \
 seq66_platform_macros.h \
 cfg/basesettings.hpp \
 cfg/cmdlineopts.hpp \
 cfg/comments.hpp \
 cfg/configfile.hpp \
 cfg/midicontrolfile.hpp \
 cfg/mutegroupsfile.hpp \
 cfg/rcfile.hpp \
 cfg/rcsettings.hpp \
 cfg/recent.hpp \
 cfg/scales.hpp \
 cfg/settings.hpp \
 cfg/userinstrument.hpp \
 cfg/usermidibus.hpp \
 cfg/usrfile.hpp \
 cfg/usrsettings.hpp \
 ctrl/automation.hpp \
 ctrl/keycontrol.hpp \
 ctrl/keycontainer.hpp \
 ctrl/keymap.hpp \
 ctrl/keystroke.hpp \
 ctrl/midicontainer.hpp \
 ctrl/midicontrol.hpp \
 ctrl/midicontrolout.hpp \
 ctrl/midioperation.hpp \
 ctrl/opcontainer.hpp \
 ctrl/opcontrol.hpp \
 midi/businfo.hpp \
 midi/controllers.hpp \
 midi/editable_event.hpp \
 midi/editable_events.hpp \
 midi/event.hpp \
 midi/event_list.hpp \
 midi/jack_assistant.hpp \
 midi/mastermidibase.hpp \
 midi/mastermidibus.hpp \
 midi/midibase.hpp \
 midi/midibus_common.hpp \
 midi/midibus.hpp \
 midi/midibytes.hpp \
 midi/midifile.hpp \
 midi/midi_splitter.hpp \
 midi/midi_vector_base.hpp \
 midi/midi_vector.hpp \
 midi/wrkfile.hpp \
 play/clockslist.hpp \
 play/inputslist.hpp \
 play/mutegroups.hpp \
 play/mutegroup.hpp \
 play/performer.hpp \
 play/playlist.hpp \
 play/screenset.hpp \
 play/seq.hpp \
 play/sequence.hpp \
 play/setmapper.hpp \
 play/triggers.hpp \
 qt/qsmacros.hpp \
 unix/daemonize.hpp \
 util/automutex.hpp \
 util/basic_macros.h \
 util/basic_macros.hpp \
 util/calculations.hpp \
 util/condition.hpp \
 util/filefunctions.hpp \
 util/palette.hpp \
 util/recmutex.hpp \
 util/rect.hpp \
 util/strfunctions.hpp \
 util/victor.hpp

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (libseq66)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------

