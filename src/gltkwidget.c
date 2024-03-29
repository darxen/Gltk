/* gltkwidget.c
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

#include "gltkwidget.h"
#include "gltkmarshal.h"

#include "gltkscreen.h"


G_DEFINE_TYPE(GltkWidget, gltk_widget, G_TYPE_INITIALLY_UNOWNED)

#define USING_PRIVATE(obj) GltkWidgetPrivate* priv = GLTK_WIDGET_GET_PRIVATE(obj)
#define GLTK_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GLTK_TYPE_WIDGET, GltkWidgetPrivate))

enum
{
	SIZE_REQUEST,
	SIZE_ALLOCATE,
	EVENT,
	LONG_TOUCH_EVENT,
	TOUCH_EVENT,
	DRAG_EVENT,
	MULTI_DRAG_EVENT,
	PINCH_EVENT,
	ROTATE_EVENT,
	CLICK_EVENT,
	RENDER,
	FIND_DROP_TARGET,
	DROP_ITEM,

   	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_TARGET_TYPE,

	N_PROPERTIES
};

typedef struct _GltkWidgetPrivate		GltkWidgetPrivate;
struct _GltkWidgetPrivate
{
	GltkAllocation allocation;

	gchar* targetType;
	gboolean isVisible;
};

static guint signals[LAST_SIGNAL] = {0,};
static GParamSpec* properties[N_PROPERTIES] = {0,};

static void gltk_widget_dispose(GObject* gobject);
static void gltk_widget_finalize(GObject* gobject);

static void	gltk_widget_set_property	(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec);
static void	gltk_widget_get_property	(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);

static void	gltk_widget_real_size_request(GltkWidget* widget, GltkSize* size);
static void	gltk_widget_real_size_allocate(GltkWidget* widget, GltkAllocation* allocation);
static gboolean	gltk_widget_real_event(GltkWidget* widget, GltkEvent* event);
static GltkWidget* gltk_widget_real_find_drop_target(GltkWidget* widget, const gchar* type, const GltkRectangle* bounds);

static void	gltk_widget_set_screen_default(GltkWidget* widget, GltkScreen* screen);
static void gltk_widget_render_default(GltkWidget* widget);

static void
gltk_widget_class_init(GltkWidgetClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(GltkWidgetPrivate));
	
	signals[SIZE_REQUEST] = 
		g_signal_new(	"size-request",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET(GltkWidgetClass, size_request),
						NULL, NULL,
						g_cclosure_marshal_VOID__POINTER,
						G_TYPE_NONE, 1,
						G_TYPE_POINTER);

	signals[SIZE_ALLOCATE] = 
		g_signal_new(	"size-allocate",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET(GltkWidgetClass, size_allocate),
						NULL, NULL,
						g_cclosure_marshal_VOID__POINTER,
						G_TYPE_NONE, 1,
						G_TYPE_POINTER);
	
	signals[EVENT] = 
		g_signal_new(	"event",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, event),
						gltk_accum_event, NULL,
						g_cclosure_user_marshal_BOOLEAN__POINTER,
						G_TYPE_BOOLEAN, 1,
						G_TYPE_POINTER);

	signals[LONG_TOUCH_EVENT] = 
		g_signal_new(	"long-touch-event",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, long_touch_event),
						gltk_accum_event, NULL,
						g_cclosure_user_marshal_BOOLEAN__POINTER,
						G_TYPE_BOOLEAN, 1,
						G_TYPE_POINTER);

	signals[TOUCH_EVENT] = 
		g_signal_new(	"touch-event",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, touch_event),
						gltk_accum_event, NULL,
						g_cclosure_user_marshal_BOOLEAN__POINTER,
						G_TYPE_BOOLEAN, 1,
						G_TYPE_POINTER);

	signals[DRAG_EVENT] = 
		g_signal_new(	"drag-event",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, drag_event),
						gltk_accum_event, NULL,
						g_cclosure_user_marshal_BOOLEAN__POINTER,
						G_TYPE_BOOLEAN, 1,
						G_TYPE_POINTER);

	signals[MULTI_DRAG_EVENT] = 
		g_signal_new(	"multi-drag-event",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, multi_drag_event),
						gltk_accum_event, NULL,
						g_cclosure_user_marshal_BOOLEAN__POINTER,
						G_TYPE_BOOLEAN, 1,
						G_TYPE_POINTER);

	signals[PINCH_EVENT] = 
		g_signal_new(	"pinch-event",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, pinch_event),
						gltk_accum_event, NULL,
						g_cclosure_user_marshal_BOOLEAN__POINTER,
						G_TYPE_BOOLEAN, 1,
						G_TYPE_POINTER);

	signals[ROTATE_EVENT] = 
		g_signal_new(	"rotate-event",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, rotate_event),
						gltk_accum_event, NULL,
						g_cclosure_user_marshal_BOOLEAN__POINTER,
						G_TYPE_BOOLEAN, 1,
						G_TYPE_POINTER);

	signals[CLICK_EVENT] = 
		g_signal_new(	"click-event",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, click_event),
						gltk_accum_event, NULL,
						g_cclosure_user_marshal_BOOLEAN__POINTER,
						G_TYPE_BOOLEAN, 1,
						G_TYPE_POINTER);

	signals[RENDER] = 
		g_signal_new(	"render",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, render),
						NULL, NULL,
						g_cclosure_user_marshal_NONE__NONE,
						G_TYPE_NONE, 0);

	signals[FIND_DROP_TARGET] = 
		g_signal_new(	"find-drop-target",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, find_drop_target),
						gltk_accum_find_widget, NULL,
						g_cclosure_user_marshal_POINTER__STRING_BOXED,
						G_TYPE_POINTER, 2,
						G_TYPE_STRING, GLTK_TYPE_RECTANGLE);
	
	signals[DROP_ITEM] = 
		g_signal_new(	"drop-item",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(GltkWidgetClass, drop_item),
						NULL, NULL,
						g_cclosure_user_marshal_NONE__STRING_POINTER,
						G_TYPE_NONE, 2,
						G_TYPE_STRING, G_TYPE_POINTER);
	
	gobject_class->dispose = gltk_widget_dispose;
	gobject_class->finalize = gltk_widget_finalize;
	gobject_class->set_property = gltk_widget_set_property;
	gobject_class->get_property = gltk_widget_get_property;

	klass->size_request = gltk_widget_real_size_request;
	klass->size_allocate = gltk_widget_real_size_allocate;
	klass->event = gltk_widget_real_event;
	klass->long_touch_event = NULL;
	klass->touch_event = NULL;
	klass->drag_event = NULL;
	klass->multi_drag_event = NULL;
	klass->pinch_event = NULL;
	klass->rotate_event = NULL;
	klass->click_event = NULL;
	klass->render = gltk_widget_render_default;

	klass->set_screen = gltk_widget_set_screen_default;
	klass->find_drop_target = gltk_widget_real_find_drop_target;
	klass->drop_item = NULL;

	klass->set_screen = gltk_widget_set_screen_default;
	klass->render = gltk_widget_render_default;
	
	properties[PROP_TARGET_TYPE] = 
		g_param_spec_string(	"target-type", "Target Type",
								"A string representing the type of drop target this widget supports",
								NULL, G_PARAM_READWRITE);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, properties);
}


static void
gltk_widget_init(GltkWidget* self)
{
	USING_PRIVATE(self);

	static GltkAllocation initialAllocation = {0, 0, -1, -1};

	self->screen = NULL;
	self->parentWidget = NULL;
	self->sizeRequest.width = -1;
	self->sizeRequest.height = -1;

	priv->allocation = initialAllocation;
	priv->targetType = NULL;
	priv->isVisible = TRUE;
}

static void
gltk_widget_dispose(GObject* gobject)
{
	GltkWidget* widget = GLTK_WIDGET(gobject);

	if (widget->parentWidget)
	{
		g_object_remove_weak_pointer(G_OBJECT(widget->parentWidget), (gpointer*)&widget->parentWidget);
		widget->parentWidget = NULL;
	}

	G_OBJECT_CLASS(gltk_widget_parent_class)->dispose(gobject);
}

static void
gltk_widget_finalize(GObject* gobject)
{
	G_OBJECT_CLASS(gltk_widget_parent_class)->finalize(gobject);
}

GltkWidget*
gltk_widget_new()
{
	GObject *gobject = g_object_new(GLTK_TYPE_WIDGET, NULL);

	return (GltkWidget*)gobject;
}

GltkWidget*
gltk_widget_get_parent(GltkWidget* widget)
{
	g_return_val_if_fail(GLTK_IS_WIDGET(widget), NULL);

	return widget->parentWidget;
}

void
gltk_widget_set_parent(GltkWidget* widget, GltkWidget* parent)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));
	g_return_if_fail(GLTK_IS_WIDGET(parent));
	g_return_if_fail(widget != parent);
	g_return_if_fail(!widget->parentWidget);

	g_object_add_weak_pointer(G_OBJECT(parent), (gpointer*)&widget->parentWidget);
	widget->parentWidget = parent;
}

void
gltk_widget_unparent(GltkWidget* widget)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));
	g_return_if_fail(GLTK_IS_WIDGET(widget->parentWidget));

	g_object_remove_weak_pointer(G_OBJECT(widget->parentWidget), (gpointer*)&widget->parentWidget);
	widget->parentWidget = NULL;
}


void
gltk_widget_set_screen(GltkWidget* widget, GltkScreen* screen)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));
	g_return_if_fail(!screen || GLTK_IS_SCREEN(screen));

	GLTK_WIDGET_GET_CLASS(widget)->set_screen(widget, screen);
}

void
gltk_widget_set_visible(GltkWidget* widget, gboolean visible)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));
	USING_PRIVATE(widget);

	if (priv->isVisible != (priv->isVisible = visible))
		gltk_widget_layout(widget);
}

gboolean
gltk_widget_get_visible(GltkWidget* widget)
{
	g_return_val_if_fail(GLTK_IS_WIDGET(widget), FALSE);
	USING_PRIVATE(widget);

	return priv->isVisible;
}

void
gltk_widget_set_size_request(GltkWidget* widget, GltkSize size)
{
	widget->sizeRequest = size;
}

void
gltk_widget_size_request(GltkWidget* widget, GltkSize* size)
{
	USING_PRIVATE(widget);

	if (!priv->isVisible)
	{
		size->width = 0;
		size->height = 0;
	}
	else
	{
		g_signal_emit(G_OBJECT(widget), signals[SIZE_REQUEST], 0, size);
	}
	
	if (widget->sizeRequest.width > -1)
		size->width = widget->sizeRequest.width;
	if (widget->sizeRequest.height > -1)
		size->height = widget->sizeRequest.height;
}

void
gltk_widget_size_allocate(GltkWidget* widget, GltkAllocation allocation)
{
	g_signal_emit(G_OBJECT(widget), signals[SIZE_ALLOCATE], 0, &allocation);
}

void
gltk_widget_update_allocation(GltkWidget* widget, GltkAllocation allocation)
{
	USING_PRIVATE(widget);

	g_return_if_fail(priv->allocation.width == allocation.width
			&& priv->allocation.height == allocation.height);

	priv->allocation = allocation;
}

GltkAllocation
gltk_widget_get_allocation(GltkWidget* widget)
{
	static GltkAllocation empty = {-1, -1, -1, -1};
	g_return_val_if_fail(GLTK_IS_WIDGET(widget), empty);
	USING_PRIVATE(widget);

	return priv->allocation;
}

GltkAllocation
gltk_widget_get_global_allocation(GltkWidget* widget)
{
	GltkAllocation res = {0,};
	g_return_val_if_fail(GLTK_IS_WIDGET(widget), res);

	res = gltk_widget_get_allocation(widget);

	while ((widget = gltk_widget_get_parent(widget)))
	{
		GltkAllocation parentAllocation = gltk_widget_get_allocation(widget);
		res.x += parentAllocation.x;
		res.y += parentAllocation.y;
	}
	return res;
}

void
gltk_widget_invalidate(GltkWidget* widget)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));

	if (widget->screen)
		gltk_screen_invalidate(widget->screen);
}

void
gltk_widget_layout(GltkWidget* widget)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));

	if (widget->screen)
		gltk_screen_layout(widget->screen);
}

void
gltk_widget_layout_now(GltkWidget* widget)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));

	gltk_widget_layout(widget);

	if (widget->screen)
		gltk_screen_flush_layout(widget->screen);
}

GltkWidget*
gltk_widget_find_drop_target(GltkWidget* widget, const gchar* type, const GltkRectangle* bounds)
{
	g_return_val_if_fail(GLTK_IS_WIDGET(widget), NULL);
	g_return_val_if_fail(type, NULL);

	GltkWidget* res = NULL;
	g_object_ref(widget);
	g_signal_emit(widget, signals[FIND_DROP_TARGET], 0, type, bounds, &res);
	g_object_unref(widget);
	return res;
}

void
gltk_widget_drop_item(GltkWidget* widget, const gchar* type, const gpointer data)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));
	g_return_if_fail(type);

	g_object_ref(widget);
	g_signal_emit(widget, signals[DROP_ITEM], 0, type, data);
	g_object_unref(widget);
}

void
gltk_widget_render(GltkWidget* widget)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));
	USING_PRIVATE(widget);

	if (priv->isVisible)
		g_signal_emit(widget, signals[RENDER], 0);
}

gboolean
gltk_widget_send_event(GltkWidget* widget, GltkEvent* event)
{
	g_return_val_if_fail(GLTK_IS_WIDGET(widget), FALSE);
	gboolean returnValue;
	g_signal_emit(G_OBJECT(widget), signals[EVENT], 0, event, &returnValue);
	return returnValue;
}

GQuark
gltk_widget_error_quark()
{
	return g_quark_from_static_string("gltk-widget-error-quark");
}

/*********************
 * Private Functions *
 *********************/

static void
gltk_widget_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
	GltkWidget* self = GLTK_WIDGET(object);
	USING_PRIVATE(self);

	switch (property_id)
	{
		case PROP_TARGET_TYPE:
			if (priv->targetType)
				g_free(priv->targetType);
			priv->targetType = g_value_dup_string(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
gltk_widget_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
	GltkWidget* self = GLTK_WIDGET(object);
	USING_PRIVATE(self);

	switch (property_id)
	{
		case PROP_TARGET_TYPE:
			g_value_set_string(value, priv->targetType);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
gltk_widget_real_size_request(GltkWidget* widget, GltkSize* size)
{
	g_return_if_fail(GLTK_IS_WIDGET(widget));

	widget->requisition = *size;
}

static void
gltk_widget_real_size_allocate(GltkWidget* widget, GltkAllocation* allocation)
{
	USING_PRIVATE(widget);

	//g_message("Widget allocation: %3d %3d %3d %3d", allocation->x, allocation->y, allocation->width, allocation->height);

	priv->allocation = *allocation;
}

static gboolean
gltk_widget_real_event(GltkWidget* widget, GltkEvent* event)
{
	gboolean returnValue = FALSE;

	GltkWidget* pWidget = widget;
	for (pWidget = widget; pWidget; pWidget = pWidget->parentWidget)
	{
		gboolean eventReturnValue = FALSE;
		switch (event->type)
		{
			case GLTK_LONG_TOUCH:
				g_signal_emit(G_OBJECT(pWidget), signals[LONG_TOUCH_EVENT], 0, event, &eventReturnValue);
				break;
			case GLTK_TOUCH:
				g_signal_emit(G_OBJECT(pWidget), signals[TOUCH_EVENT], 0, event, &eventReturnValue);
				break;
			case GLTK_DRAG:
				g_signal_emit(G_OBJECT(pWidget), signals[DRAG_EVENT], 0, event, &eventReturnValue);
				break;
			case GLTK_MULTI_DRAG:
				g_signal_emit(G_OBJECT(pWidget), signals[MULTI_DRAG_EVENT], 0, event, &eventReturnValue);
				break;
			case GLTK_PINCH:
				g_signal_emit(G_OBJECT(pWidget), signals[PINCH_EVENT], 0, event, &eventReturnValue);
				break;
			case GLTK_ROTATE:
				g_signal_emit(G_OBJECT(pWidget), signals[ROTATE_EVENT], 0, event, &eventReturnValue);
				break;
			case GLTK_CLICK:
				g_signal_emit(G_OBJECT(pWidget), signals[CLICK_EVENT], 0, event, &eventReturnValue);
				break;
			default:
				g_warning("Unhandled event type: %i", event->type);
		}
		if (!returnValue)
			returnValue = eventReturnValue;

		//only send certain events to 1 widget
		if (returnValue)
			switch (event->type)
			{
				case GLTK_LONG_TOUCH:
				case GLTK_DRAG:
					return returnValue;
				default:
					break;
			}
	}

	return returnValue;
}

static GltkWidget*
gltk_widget_real_find_drop_target(GltkWidget* widget, const gchar* type, const GltkRectangle* bounds)
{
	USING_PRIVATE(widget);

	//g_debug("My targetType: %s", priv->targetType);

	if (priv->targetType && !g_strcmp0(type, priv->targetType))
		return widget;
	return NULL;
}

static void
gltk_widget_set_screen_default(GltkWidget* widget, GltkScreen* screen)
{
	widget->screen = screen;
}

static void
gltk_widget_render_default(GltkWidget* widget)
{
	g_warning("A widget forgot to override the render method");
}

