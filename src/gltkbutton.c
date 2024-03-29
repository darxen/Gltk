/* gltkbutton.c
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

#include "gltkbutton.h"

#include "gltkfonts.h"
#include "gltkscreen.h"

#include <string.h>
#include <math.h>

#include <GL/gl.h>

G_DEFINE_TYPE(GltkButton, gltk_button, GLTK_TYPE_WIDGET)

#define USING_PRIVATE(obj) GltkButtonPrivate* priv = GLTK_BUTTON_GET_PRIVATE(obj)
#define GLTK_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GLTK_TYPE_BUTTON, GltkButtonPrivate))

#define CORNER_SIZE 8

static GLuint texBorder;

enum
{
	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_TEXT,

	N_PROPERTIES
};

typedef struct _GltkButtonPrivate		GltkButtonPrivate;
struct _GltkButtonPrivate
{
	int dummy;
};

//static guint signals[LAST_SIGNAL] = {0,};
static GParamSpec* properties[N_PROPERTIES] = {0,};

static void gltk_button_dispose(GObject* gobject);
static void gltk_button_finalize(GObject* gobject);

static void	gltk_button_set_property	(	GObject* object, guint property_id,
											const GValue* value, GParamSpec* pspec);
static void	gltk_button_get_property	(	GObject* object, guint property_id,
											GValue* value, GParamSpec* pspec);

static void gltk_button_size_request(GltkWidget* widget, GltkSize* size);
static void gltk_button_render(GltkWidget* widget);
static gboolean gltk_button_touch_event(GltkWidget* widget, GltkEventTouch* touch);

static void
gltk_button_class_init(GltkButtonClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
	GltkWidgetClass* gltkwidget_class = GLTK_WIDGET_CLASS(klass);

	g_type_class_add_private(klass, sizeof(GltkButtonPrivate));
	
	gobject_class->dispose = gltk_button_dispose;
	gobject_class->finalize = gltk_button_finalize;
	gobject_class->set_property = gltk_button_set_property;
	gobject_class->get_property = gltk_button_get_property;

	gltkwidget_class->size_request = gltk_button_size_request;
	gltkwidget_class->render = gltk_button_render;
	gltkwidget_class->touch_event = gltk_button_touch_event;

	properties[PROP_TEXT] = 
		g_param_spec_string(	"text", "Text", "Text to display",
								"", G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, properties);
}

static void
gltk_button_init(GltkButton* self)
{
	self->text = NULL;
	self->isDown = FALSE;

	self->renderStyle = GLTK_BUTTON_RENDER_DEFAULT;
}

static void
gltk_button_dispose(GObject* gobject)
{
	G_OBJECT_CLASS(gltk_button_parent_class)->dispose(gobject);
}

static void
gltk_button_finalize(GObject* gobject)
{
	GltkButton* self = GLTK_BUTTON(gobject);

	g_free(self->text);

	G_OBJECT_CLASS(gltk_button_parent_class)->finalize(gobject);
}

GltkWidget*
gltk_button_new(const gchar* text)
{
	GObject *gobject = g_object_new(GLTK_TYPE_BUTTON, "text", text, NULL);

	return (GltkWidget*)gobject;
}

const gchar*
gltk_button_get_text(GltkButton* button)
{
	return button->text;
}

void
gltk_button_set_text(GltkButton* button, const gchar* text)
{
	if (button->text)
		g_free(button->text);

	button->text = g_strdup(text);

	gltk_widget_layout(GLTK_WIDGET(button));
}

GQuark
gltk_button_error_quark()
{
	return g_quark_from_static_string("gltk-button-error-quark");
}

/*********************
 * Private Functions *
 *********************/

static void
gltk_button_set_property	(	GObject* object, guint property_id,
								const GValue* value, GParamSpec* pspec)
{
	GltkButton* self = GLTK_BUTTON(object);

	switch (property_id)
	{
		case PROP_TEXT:
			if (self->text)
				g_free(self->text);
			self->text = g_value_dup_string(value);
			gltk_widget_invalidate(GLTK_WIDGET(self));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
gltk_button_get_property	(	GObject* object, guint property_id,
								GValue* value, GParamSpec* pspec)
{
	GltkButton* self = GLTK_BUTTON(object);

	switch (property_id)
	{
		case PROP_TEXT:
			g_value_set_string(value, self->text);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
gltk_button_size_request(GltkWidget* widget, GltkSize* size)
{
	GltkGLFont* font = gltk_fonts_cache_get_font(GLTK_FONTS_BASE, 24, FALSE);

	GltkGLFontBounds bounds = gltk_fonts_measure_string(font, GLTK_BUTTON(widget)->text);
	size->height = bounds.height + 30;
	size->width = bounds.width + 40;

	GLTK_WIDGET_CLASS(gltk_button_parent_class)->size_request(widget, size);
}

static float colorBright[] = {0.87f, 0.87f, 0.87f};
static float colorDark[] = {0.78f, 0.78f, 0.78f};

static float colorHighlightBright[] = {1.0f, 0.6f, 0.03f};
static float colorHighlightDark[] = {1.0f, 0.65f, 0.16f};

#define OFFSET 18

static void
gltk_button_render(GltkWidget* widget)
{
	if (!texBorder)
	{
		//glEnable(GL_TEXTURE_2D);
		float* border = g_new0(float, CORNER_SIZE*CORNER_SIZE);
		int x;
		int y;
		for (y = 0; y < CORNER_SIZE; y++)
			for (x = 0; x < CORNER_SIZE; x++)
				border[y*CORNER_SIZE+x] = (float)(sqrt(x*x+y*y) <= CORNER_SIZE);

		glGenTextures(1, &texBorder);
		glBindTexture(GL_TEXTURE_2D, texBorder);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, CORNER_SIZE, CORNER_SIZE, 0, GL_ALPHA, GL_FLOAT, border);
		g_free(border);
	}

	GltkAllocation allocation = gltk_widget_get_allocation(GLTK_WIDGET(widget));

	GltkButton* button = GLTK_BUTTON(widget);

	switch (button->renderStyle)
	{
		case GLTK_BUTTON_RENDER_DEFAULT:
		{
			float* bright = colorBright;
			float* dark = colorDark;

			if (GLTK_BUTTON(widget)->isDown)
			{
				bright = colorHighlightBright;
				dark = colorHighlightDark;
			}

			glBegin(GL_QUADS);
			{
				glColor3fv(bright);
				glVertex2i(allocation.width - CORNER_SIZE, 0);
				glVertex2i(CORNER_SIZE, 0);
				glVertex2i(CORNER_SIZE, CORNER_SIZE);
				glVertex2i(allocation.width - CORNER_SIZE, CORNER_SIZE);
				
				glColor3fv(dark);
				glVertex2i(allocation.width - CORNER_SIZE, allocation.height);
				glVertex2i(CORNER_SIZE, allocation.height);
				glVertex2i(CORNER_SIZE, allocation.height - CORNER_SIZE);
				glVertex2i(allocation.width - CORNER_SIZE, allocation.height - CORNER_SIZE);

				glColor3fv(bright);
				glVertex2i(allocation.width, CORNER_SIZE);
				glVertex2i(0, CORNER_SIZE);
				glColor3fv(dark);
				glVertex2i(0, allocation.height - CORNER_SIZE);
				glVertex2i(allocation.width, allocation.height - CORNER_SIZE);
			}
			glEnd();

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texBorder);
			glBegin(GL_QUADS);
			{
				glColor3fv(bright);
				glTexCoord2i(0, 0); glVertex2i(CORNER_SIZE, CORNER_SIZE);
				glTexCoord2i(0, 1); glVertex2i(CORNER_SIZE, 0);
				glTexCoord2i(1, 1); glVertex2i(0, 0);
				glTexCoord2i(1, 0); glVertex2i(0, CORNER_SIZE);

				glTexCoord2i(0, 0); glVertex2i(allocation.width - CORNER_SIZE, CORNER_SIZE);
				glTexCoord2i(0, 1); glVertex2i(allocation.width - CORNER_SIZE, 0);
				glTexCoord2i(1, 1); glVertex2i(allocation.width, 0);
				glTexCoord2i(1, 0); glVertex2i(allocation.width, CORNER_SIZE);
				
				glColor3fv(dark);
				glTexCoord2i(0, 0); glVertex2i(allocation.width - CORNER_SIZE, allocation.height - CORNER_SIZE);
				glTexCoord2i(0, 1); glVertex2i(allocation.width - CORNER_SIZE, allocation.height);
				glTexCoord2i(1, 1); glVertex2i(allocation.width, allocation.height);
				glTexCoord2i(1, 0); glVertex2i(allocation.width, allocation.height - CORNER_SIZE);
				
				glTexCoord2i(0, 0); glVertex2i(CORNER_SIZE, allocation.height - CORNER_SIZE);
				glTexCoord2i(0, 1); glVertex2i(CORNER_SIZE, allocation.height);
				glTexCoord2i(1, 1); glVertex2i(0, allocation.height);
				glTexCoord2i(1, 0); glVertex2i(0, allocation.height - CORNER_SIZE);
			}
			glEnd();
			glDisable(GL_TEXTURE_2D);
		} break;
		case GLTK_BUTTON_RENDER_OUTLINE:
		{
			glBegin(GL_LINE_LOOP);
			{
				float* bright = colorBright;
				float* dark = colorDark;

				if (GLTK_BUTTON(widget)->isDown)
				{
					bright = colorHighlightBright;
					dark = colorHighlightDark;
				}
				glColor3fv(bright);
				glVertex2i(allocation.width, 0);
				glVertex2i(OFFSET, 0);
				glVertex2i(0, OFFSET);
				glColor3fv(dark);
				glVertex2i(0, allocation.height);
				glVertex2i(allocation.width - OFFSET, allocation.height);
				glVertex2i(allocation.width, allocation.height - OFFSET);
			}
			glEnd();
		} break;
		default:
			g_critical("Invalid render style: %d", button->renderStyle);
			break;
	}


	if (GLTK_BUTTON(widget)->text)
	{
		glPushMatrix();
		{
			GltkGLFont* font = gltk_fonts_cache_get_font(GLTK_FONTS_BASE, 24, TRUE);
			if (button->renderStyle == GLTK_BUTTON_RENDER_DEFAULT)
				glColor3f(0.0f, 0.0f, 0.0f);
			else
				glColor3f(1.0f, 1.0f, 1.0f);

			GltkGLFontBounds bounds = gltk_fonts_measure_string(font, GLTK_BUTTON(widget)->text);
			float height = bounds.height;
			float width = bounds.width;

			float x;
			float y;
			x = (allocation.width - width) / 2.0;
			y = (allocation.height - height) / 2.0 + font->ascender + font->descender;

			glTranslatef(x, y, 0.1f);
			glScalef(1.0f, -1.0f, 1.0f);

			ftglRenderFont(font->font, GLTK_BUTTON(widget)->text, FTGL_RENDER_ALL);
		}
		glPopMatrix();
	}
}

static gboolean
gltk_button_touch_event(GltkWidget* widget, GltkEventTouch* touch)
{
	if (touch->touchType == TOUCH_BEGIN)
	{
		GLTK_BUTTON(widget)->isDown = TRUE;
		gltk_screen_set_widget_pressed(widget->screen, widget);
	}
	else if (touch->touchType == TOUCH_END)
	{
		GLTK_BUTTON(widget)->isDown = FALSE;
		gltk_screen_set_widget_unpressed(widget->screen, widget);
	}
	else
	{
		return FALSE;
	}

	gltk_widget_invalidate(widget);

	return TRUE;
}

