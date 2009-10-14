/***************************************************************************
 *            pdf.c
 *
 *  Wed Oct 14 14:50:35 2009
 *  Copyright  2009  Neil Williams
 *  <linux@codehelp.co.uk>
 ****************************************************************************/

/*
 * gpdftext is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gpdftext is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
/** general PDF functions. */
/**

Idea: file_selector SaveAs to get a filename, then create a
 new PDF - with preview support - and choice of font, using
 these routines:

PopplerDocument *
poppler_document_new_from_data (char        *data,
                                int          length,
                                const char  *password,
                                GError     **error)

(never set *password)

gboolean
poppler_document_save (PopplerDocument  *document,
               const char       *uri,
               GError          **error)

Need to add a new window to the GTKBuilder XML using glade-3
 that just shows a GdkPixbuf, also with controls for next page
 first page etc. or just use libevview?

void                   poppler_page_render_to_pixbuf     (PopplerPage        *page,
                              int                 src_x,
                              int                 src_y,
                              int                 src_width,
                              int                 src_height,
                              double              scale,
                              int                 rotation,
                              GdkPixbuf          *pixbuf);

The problem (and the goal) is to harness poppler to change the
paper size from A4 to something more like a book.
Something like A5 portrait or B5 portrait.
http://www.cl.cam.ac.uk/~mgk25/iso-paper.html
http://www.hintsandthings.co.uk/office/paper.htm

   width   height
A5 148mm × 210mm   5.8 x  8.3 inches
B5 176mm × 250mm   6.9 x  9.8 inches

Ratio of 0.7:1
Hence the unusual shape of the main gpdftext window.
(440x630, same proportions as A5.)

Realistically, the only way to do this is to produce a PostScript
 file and then Recommend: ghostscript - check if /usr/bin/ps2pdf
 exists and call it. Creating a PDF of a non-standard size is
 (or at least appears to be) more trouble than it is worth. (The
 functions to do this are private within poppler and are not
 exported at all in poppler-glib.)

 * @width: the paper width in 1/72 inch
 * @height: the paper height in 1/72 inch

void
poppler_ps_file_set_paper_size (PopplerPSFile *ps_file,
                                double width, double height)

So we'd need: 
    width  height
A5  417.6   597.6
B5  496.8   705.6

to revert to A4, just leave these as defaults.

then use:

void
poppler_page_render_to_ps (PopplerPage   *page,
               PopplerPSFile *ps_file)

(one page at a time - so use the progressbar!)

Needs support in the UI using gtk_stock_convert 

Needs support from GConf so that the user can choose A4, A5 or B5
and stick with that choice. That, in turn, needs a Preferences
window to be added to the glade file. GConf also holds details of
the default font.

Spell check support also needs GConf and should be added once
GConf is enabled.

The main window size should also be configurable via GConf, with the
default values retained.

Other GConf values are which regular expressions should be used.

This will also probably need some kind of context object
that can be passed around between functions to provide the
pointer to the builder: GPContext * context; The context could
then have other bits that may become necessary, like
the preferences values.

*/

#include <glib.h>

/** width for an A5 PS file. */
gdouble a5_width = 417.6;
/** height for an A5 PS file. */
gdouble a5_height = 597.6;
/** width for a B5 PS file. */
gdouble b5_width = 496.8;
/** height for a B5 PS file. */
gdouble b5_height = 705.6;


