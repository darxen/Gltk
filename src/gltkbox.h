/* gltkbox.h
 *
 * Copyright (C) 2011 - Kevin Wells <kevin@darxen.org>
 *
 * This file is part of darxen
 *
 * darxen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * darxen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with darxen.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GLTKBOX_H_UVGUAGZ7
#define GLTKBOX_H_UVGUAGZ7

#include <glib-object.h>
#include "gltkwidget.h"

G_BEGIN_DECLS

#define GLTK_BOX_ERROR gltk_box_error_quark()

#define GLTK_TYPE_BOX				(gltk_box_get_type())
#define GLTK_BOX(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), GLTK_TYPE_BOX, GltkBox))
#define GLTK_BOX_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), GLTK_TYPE_BOX, GltkBoxClass))
#define GLTK_IS_BOX(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), GLTK_TYPE_BOX))
#define GLTK_IS_BOX_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), GLTK_TYPE_BOX))
#define GLTK_BOX_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), GLTK_TYPE_BOX, GltkBoxClass))

typedef struct _GltkBox				GltkBox;
typedef struct _GltkBoxClass		GltkBoxClass;
typedef struct _GltkBoxChild		GltkBoxChild;

struct _GltkBox
{
	GltkWidget parent;
	
	GList* children; //GltkBoxChild

	int expandCount;
	int childrenCount;

	int spacing;
};

struct _GltkBoxClass
{
	GltkWidgetClass parent_class;
	
	/* signals */
	/* virtual funcs */
};

struct _GltkBoxChild
{
	GltkWidget* widget;
	gboolean expand;
	gboolean fill;
};

typedef enum
{
	GLTK_BOX_ERROR_FAILED
} GltkBoxError;

GType			gltk_box_get_type	() G_GNUC_CONST;
GltkWidget*		gltk_box_new		();

void			gltk_box_insert_widget	(GltkBox* box, GltkWidget* widget, int index, gboolean expand, gboolean fill);
void			gltk_box_append_widget	(GltkBox* box, GltkWidget* widget, gboolean expand, gboolean fill);
void			gltk_box_remove_widget	(GltkBox* box, GltkWidget* widget);

GQuark			gltk_box_error_quark	();

G_END_DECLS

#endif

