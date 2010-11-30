#ifdef GTK_PATCH

#include <gtk/gtkliststore.h>

/*
 * copy&paste from gtk+-2.1.0
 */
#define VALID_ITER(iter, list_store) (iter!= NULL && iter->user_data != NULL && list_store->stamp == iter->stamp)

gboolean
gtk_list_store_iter_is_valid (GtkListStore *list_store,
                              GtkTreeIter  *iter)
{
  GList *list;

  g_return_val_if_fail (GTK_IS_LIST_STORE (list_store), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  if (!VALID_ITER (iter, list_store))
    return FALSE;

  if (iter->user_data == list_store->root)
    return TRUE;
  if (iter->user_data == list_store->tail)
    return TRUE;

  for (list = ((GList *)list_store->root)->next; list; list = list->next)
    if (list == iter->user_data)
      return TRUE;

  return FALSE;
}

#endif /* GTK_PATCH */

