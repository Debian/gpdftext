/***************************************************************************
 *            pdf.h
 *
 *  Wed Oct 14 14:50:35 2009
 *  Copyright  2009  Neil Williams
 *  <linux@codehelp.co.uk>
 ****************************************************************************/

/*
 * gpdftext is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * gpdftext is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 

#ifndef _EPDF_H_
#define _EPDF_H_

#include <glib/poppler-document.h>
#include <glib/poppler-page.h>
#include "ebookui.h"

/** ensure the export filename is set before requesting the conversion. */
void buffer_to_pdf (Ebook * ebook);

/** ensure the export filename is set before requesting the conversion. */
void buffer_to_txt (Ebook * ebook);

void open_pdf_cb (GtkWidget *widget, gpointer data);

#endif // _EPDF_H
