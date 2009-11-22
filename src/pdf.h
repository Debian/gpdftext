/***************************************************************************
 *            pdf.h
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
 

#ifndef _EPDF_H_
#define _EPDF_H_

#include <glib/poppler-document.h>
#include <glib/poppler-page.h>
#include "ebookui.h"

/** ensure the export filename is set before requesting the conversion. */
void buffer_to_ps (Ebook * ebook);

void open_pdf_cb (GtkWidget *widget, gpointer data);

#endif // _EPDF_H
