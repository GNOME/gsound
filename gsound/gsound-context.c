/* gsound-context.c
 *
 * Copyright (C) 2013 Tristan Brindle <t.c.brindle@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * Section: GSoundContext
 * @short_description: GSound context object
 * @see_also: #ca_context
 * 
 * A #GSoundContext is used for playing system sounds. You first initialise
 * the context (using the #GInitable interface or gsound_context_new()),
 * and then call gsound_context_play_simple() (or gsound_context_play_full()) to
 * play sounds.
 * 
 * ##Simple Example
 * 
 * |[<!-- language="C" -->
 * GSoundContext *ctx = NULL;
 * GCancellable *cancellable;
 * GError *error = NULL;
 * 
 * cancellable = g_cancellable_new();
 * 
 * ctx = gsound_context_new(cancellable, &error);
 * if (error) {
 *     // handle error
 * }
 * 
 * gsound_context_play_simple(ctx, cancellable, &error,
 *                            GSOUND_ATTR_MEDIA_FILENAME, "/path/to/file",
 *                            // other attributes...
 *                            NULL);
 * ]| 
 * 
 * or, using the Python bindings:
 * 
 * |[<!-- language="Python" -->
 * from gi.repository import GSound, Gio
 * 
 * ctx = GSound.Context()
 * cancellable = Gio.Cancellable()
 * 
 * try:
 *     ctx.init(cancellable);
 *     ctx.play_simple(cancellable,
 *                    { GSound.ATTR_MEDIA_FILENAME : "/path/to/file })
 * except:
 *     # Handle error
 *     pass
 * ]|
 * 
 * or using Vala:
 * 
 * |[<!-- language="Vala" -->
 * try {
 *     var ctx = new GSound.Context();
 *     ctx.play_simple(null, GSound.ATTR_MEDIA_FILENAME, "/path/to/file");
 * } catch (Error e) {
 *     // handle error
 * }
 * 
 * #play_simple() and play_full()
 * 
 * The above example use the gsound_context_play_simple() method for
 * playing sounds. This is a "fire and forget" method which returns
 * immediately and does not block your program, and is suitable for most use
 * cases.
 * 
 * If you need to find out when the sound finished (for example to repeat the
 * sound) then you can use the `gsound_context_play_full()` version. This
 * is an asynchronous method using the standard GIO async pattern, which will
 * run the supplied #GAsyncReadyCallback when the sound server has finished.
 * 
 * ##Passing Attributes
 * 
 * GSound supplies information to the sound server by means of attributes.
 * Attributes can be set on the #GSoundContext itself using
 * gsound_context_change_attrs(), or supplied in a play() call. Attributes
 * set on the context will automatically applied to any subsequent play()
 * calls, unless overridden by that call.
 * 
 * In C and Vala, attributes are passed as %NULL-terminated list of
 * (attribute, value) pairs. When using GObject introspection, attributes are
 * typically passed using a language-specific associated array, for example
 * a dict in Python or an object in JavaScript.
 * 
 * 
 * 
 */

#include "gsound-context.h"

#include <canberra.h>

#include <stdarg.h>

static void gsound_context_initable_init (GInitableIface *iface);

struct _GSoundContext
{
  GObject     parent;

  ca_context *ca;
};

struct _GSoundContextClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (GSoundContext, gsound_context, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                gsound_context_initable_init))

G_DEFINE_QUARK (gsound - error - quark, gsound_error);

static gboolean
test_return (int code, GError **error)
{
  if (code == CA_SUCCESS)
    return TRUE;

  g_set_error_literal (error, GSOUND_ERROR, code, ca_strerror (code));
  return FALSE;
}

static void
hash_table_to_prop_list (GHashTable *ht, ca_proplist *pl)
{
  gpointer key, value;
  GHashTableIter iter;

  g_hash_table_ref (ht);

  g_hash_table_iter_init (&iter, ht);
  while (g_hash_table_iter_next (&iter, &key, &value))
    ca_proplist_sets (pl, key, value);

  g_hash_table_unref (ht);
}

static int
var_args_to_prop_list (va_list args, ca_proplist *pl)
{
  while (TRUE)
    {
      const char *key;
      const char *val;
      int res;

      key = va_arg (args, const char*);
      if (!key)
        return CA_SUCCESS;

      val = va_arg (args, const char*);
      if (!val)
        return CA_ERROR_INVALID;

      res = ca_proplist_sets (pl, key, val);
      if (res != CA_SUCCESS)
        return res;
    }

  return CA_SUCCESS;
}

static void
on_ca_play_full_finished (ca_context *ca,
                          guint32     id,
                          int         error_code,
                          gpointer    user_data)
{
  GTask *task = user_data;

  if (error_code != CA_SUCCESS)
    {
      g_task_return_new_error (task,
                               GSOUND_ERROR,
                               error_code,
                               "%s",
                               ca_strerror (error_code));
    }
  else
    g_task_return_boolean (task, TRUE);

  g_object_unref (task);
}

static void
on_cancellable_cancelled (GCancellable  *cancellable,
                          GSoundContext *self)
{
  ca_context_cancel (self->ca, g_direct_hash (cancellable));
}

/**
 * gsound_context_new:
 * @cancellable: (allow-none): A #GCancellable, or %NULL
 * @error: Return location for error
 *
 * Returns: (transfer full): Creates and initializes a new #GSoundContext.
 */
GSoundContext *
gsound_context_new (GCancellable *cancellable, GError **error)
{
  return GSOUND_CONTEXT (g_initable_new (GSOUND_TYPE_CONTEXT,
                                         cancellable,
                                         error,
                                         NULL));
}

/**
 * gsound_context_open:
 * @context: A #GSoundContext
 * @error: Return location for error, or %NULL
 *
 * Returns: %TRUE if the output device was opened successfully, or %FALSE
 *          (populating @error)
 */
gboolean
gsound_context_open (GSoundContext *self, GError **error)
{
  g_return_val_if_fail (GSOUND_IS_CONTEXT (self), FALSE);

  return test_return (ca_context_open (self->ca), error);
}

/**
 * gsound_context_set_driver:
 * @context: A #GSoundContext
 * @driver: libcanberra driver to use
 * @error: Return location for error, or %NULL
 *
 * Returns: %TRUE if the libcanberra driver was set successfully
 */
gboolean
gsound_context_set_driver (GSoundContext *self,
                           const char    *driver,
                           GError       **error)
{
  g_return_val_if_fail (GSOUND_IS_CONTEXT (self), FALSE);

  return test_return (ca_context_set_driver (self->ca, driver), error);
}

/**
 * gsound_context_change_attrs: (skip)
 * @context: A #GSoundContext
 * @error: Return location for error
 * @...: %NULL terminated list of attribute name-value pairs
 *
 * Returns: %TRUE if attributes were updated successfully
 */
gboolean
gsound_context_change_attrs (GSoundContext *self,
                             GError       **error,
                             ...)
{
  ca_proplist *pl;
  va_list args;
  int res;

  g_return_val_if_fail (GSOUND_IS_CONTEXT (self), FALSE);

  if ((res = ca_proplist_create (&pl)) != CA_SUCCESS)
    return test_return (res, error);

  va_start (args, error);
  var_args_to_prop_list (args, pl);
  va_end (args);

  res = ca_context_change_props_full (self->ca, pl);

  g_clear_pointer (&pl, ca_proplist_destroy);

  return test_return (res, error);
}

/**
 * gsound_context_change_attrsv:
 * @context: A #GSoundContext
 * @attrs: (element-type utf8 utf8): Hash table of attributes to set
 * @error: Return location for error, or %NULL
 *
 * Returns: %TRUE if attributes were updated successfully
 */
gboolean
gsound_context_change_attrsv (GSoundContext *self,
                              GHashTable    *attrs,
                              GError       **error)
{
  ca_proplist *pl;
  int res;

  g_return_val_if_fail (GSOUND_IS_CONTEXT (self), FALSE);

  res = ca_proplist_create (&pl);
  if (!test_return (res, error))
    return FALSE;

  hash_table_to_prop_list (attrs, pl);

  res = ca_context_change_props_full (self->ca, pl);

  g_clear_pointer (&pl, ca_proplist_destroy);

  return test_return (res, error);
}

/**
 * gsound_context_play_simple: (skip)
 * @context: A #GSoundContext
 * @cancellable: (allow-none): A #GCancellable, or %NULL
 * @error: Return location for error, or %NULL
 * @...: Arguments
 *
 * Returns: %TRUE on success, or %FALSE, populating @error
 */
gboolean
gsound_context_play_simple (GSoundContext *self,
                            GCancellable  *cancellable,
                            GError       **error,
                            ...)
{
  ca_proplist *pl;
  va_list args;
  int res;

  g_return_val_if_fail (GSOUND_IS_CONTEXT (self), FALSE);

  if ((res = ca_proplist_create (&pl)) != CA_SUCCESS)
    return test_return (res, error);

  va_start (args, error);
  var_args_to_prop_list (args, pl);
  va_end (args);

  res = ca_context_play_full (self->ca,
                              g_direct_hash (cancellable),
                              pl, NULL, NULL);

  if (cancellable)
    g_cancellable_connect (cancellable,
                           G_CALLBACK (on_cancellable_cancelled),
                           g_object_ref (self),
                           g_object_unref);

  g_clear_pointer (&pl, ca_proplist_destroy);

  return test_return (res, error);
}

/**
 * gsound_context_play_simplev:
 * @context: A #GSoundContext
 * @attrs: (element-type utf8 utf8): Attributes
 * @cancellable: (allow-none): A #GCancellable
 * @error: Return location for error
 *
 * Returns: %TRUE on success, %FALSE on error
 *
 * Rename to: gsound_context_play_simple
 */
gboolean
gsound_context_play_simplev (GSoundContext *self,
                             GHashTable    *attrs,
                             GCancellable  *cancellable,
                             GError       **error)
{
  ca_proplist *pl;
  int res = ca_proplist_create (&pl);

  if (!test_return (res, error))
    return FALSE;

  hash_table_to_prop_list (attrs, pl);

  res = ca_context_play_full (self->ca,
                              g_direct_hash (cancellable),
                              pl, NULL, NULL);

  if (cancellable)
    g_cancellable_connect (cancellable,
                           G_CALLBACK (on_cancellable_cancelled),
                           g_object_ref (self),
                           g_object_unref);

  g_clear_pointer (&pl, ca_proplist_destroy);

  return test_return (res, error);
}

/**
 * gsound_context_play_full: (skip)
 * @context: A #GSoundContext
 * @cancellable: (allow-none): A #GCancellable, or %NULL
 * @callback: (scope async): callback
 * @user_data: User data passed to @callback
 * @...: Attributes
 *
 */
void
gsound_context_play_full (GSoundContext      *self,
                          GCancellable       *cancellable,
                          GAsyncReadyCallback callback,
                          gpointer            user_data,
                          ...)
{
  GError *inner_error = NULL;
  ca_proplist *proplist;
  va_list args;
  GTask *task;
  int res;

  task = g_task_new (self, cancellable, callback, user_data);

  res = ca_proplist_create (&proplist);
  if (!test_return (res, &inner_error))
    {
      g_task_return_error (task, inner_error);
      g_object_unref (task);
      return;
    }

  va_start (args, user_data);
  var_args_to_prop_list (args, proplist);
  va_end (args);

  res = ca_context_play_full (self->ca,
                              g_direct_hash (cancellable),
                              proplist,
                              on_ca_play_full_finished,
                              task);

  if (cancellable)
    g_cancellable_connect (cancellable,
                           G_CALLBACK (on_cancellable_cancelled),
                           g_object_ref (self),
                           g_object_unref);

  g_clear_pointer (&proplist, ca_proplist_destroy);

  if (!test_return (res, &inner_error))
    {
      g_task_return_error (task, inner_error);
      g_object_unref (task);
    }
}

/**
 * gsound_context_play_fullv:
 * @context: A #GSoundContext
 * @attrs: (element-type utf8 utf8): Attributes
 * @cancellable: (allow-none): A #GCancellable, or %NULL
 * @callback: (scope async): callback
 * @user_data: user_data
 *
 * Rename to: gsound_context_play_full
 */
void
gsound_context_play_fullv (GSoundContext      *self,
                           GHashTable         *attrs,
                           GCancellable       *cancellable,
                           GAsyncReadyCallback callback,
                           gpointer            user_data)
{
  GError *inner_error = NULL;
  ca_proplist *proplist;
  GTask *task;
  int res;

  task = g_task_new (self, cancellable, callback, user_data);

  res = ca_proplist_create (&proplist);
  if (!test_return (res, &inner_error))
    {
      g_task_return_error (task, inner_error);
      g_object_unref (task);
      return;
    }

  hash_table_to_prop_list (attrs, proplist);

  res = ca_context_play_full (self->ca,
                              g_direct_hash (cancellable),
                              proplist,
                              on_ca_play_full_finished,
                              task);

  if (cancellable)
    g_cancellable_connect (cancellable,
                           G_CALLBACK (on_cancellable_cancelled),
                           g_object_ref (self),
                           g_object_unref);

  g_clear_pointer (&proplist, ca_proplist_destroy);

  if (!test_return (res, &inner_error))
    {
      g_task_return_error (task, inner_error);
      g_object_unref (task);
    }
}

/**
 * gsound_context_play_full_finish:
 * @context: A #GSoundContext
 * @result: Result object returned to the callback of
 *   gsound_context_play_full()
 * @error: Return location for error
 *
 * Returns: %TRUE if playing finished successfully
 */
gboolean
gsound_context_play_full_finish (GSoundContext *self,
                                 GAsyncResult  *result,
                                 GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * gsound_context_cache: (skip)
 * @context: A #GSoundContext
 * @error: Return location for error, or %NULL
 * @...: attributes
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
gsound_context_cache (GSoundContext *self,
                      GError       **error,
                      ...)
{
  ca_proplist *pl;
  va_list args;
  int res;

  g_return_val_if_fail (GSOUND_IS_CONTEXT (self), FALSE);

  if ((res = ca_proplist_create (&pl)) != CA_SUCCESS)
    return test_return (res, error);

  va_start (args, error);
  var_args_to_prop_list (args, pl);
  va_end (args);

  res = ca_context_cache_full (self->ca, pl);

  g_clear_pointer (&pl, ca_proplist_destroy);

  return test_return (res, error);
}

/**
 * gsound_context_cachev:
 * @context: A #GSoundContext
 * @attrs: (element-type utf8 utf8): Hash table of attrerties
 * @error: Return location for error, or %NULL
 *
 * Returns: %TRUE on success
 *
 * Rename to: gsound_context_cache
 */
gboolean
gsound_context_cachev (GSoundContext *self,
                       GHashTable    *attrs,
                       GError       **error)
{
  ca_proplist *proplist;
  int res = ca_proplist_create (&proplist);

  if (!test_return (res, error))
    return FALSE;

  hash_table_to_prop_list (attrs, proplist);

  res = ca_context_cache_full (self->ca, proplist);

  g_clear_pointer (&proplist, ca_proplist_destroy);

  return test_return (res, error);
}

static gboolean
gsound_context_real_init (GInitable    *initable,
                          GCancellable *cancellable,
                          GError      **error)
{
  GSoundContext *self = GSOUND_CONTEXT (initable);
  int success;
  ca_proplist *pl;

  if (self->ca)
    return TRUE;

  success = ca_context_create (&self->ca);

  if (!test_return (success, error))
    return FALSE;

  /* Set a couple of attributes here if we can */
  ca_proplist_create (&pl);

  ca_proplist_sets (pl, CA_PROP_APPLICATION_NAME, g_get_application_name ());
  if (g_application_get_default ())
    {
      GApplication *app = g_application_get_default ();
      ca_proplist_sets (pl, CA_PROP_APPLICATION_ID,
                        g_application_get_application_id (app));
    }

  success = ca_context_change_props_full (self->ca, pl);

  g_clear_pointer (&pl, ca_proplist_destroy);

  if (!test_return (success, error))
      g_clear_pointer (&self->ca, ca_context_destroy);

  return TRUE;
}

static void
gsound_context_finalize (GObject *obj)
{
  GSoundContext *self = GSOUND_CONTEXT (obj);

  g_clear_pointer (&self->ca, ca_context_destroy);

  G_OBJECT_CLASS (gsound_context_parent_class)->finalize (obj);
}

static void
gsound_context_class_init (GSoundContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gsound_context_finalize;
}

static void
gsound_context_init (GSoundContext *self)
{
}

static void
gsound_context_initable_init (GInitableIface *iface)
{
  iface->init = gsound_context_real_init;
}
