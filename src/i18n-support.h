/*
* Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
* All rights reserved.
*
* The program is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public License as
* published by the Free Software Foundation; either version 2 of the
* License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this program; see the file COPYING.LIB.  If not,
* write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/
/*
 *   @NOTATION@
 *    */
                                                                                                                                 
/*
* Handles all of the internationalization configuration options.
* Author: Tom Tromey <tromey@creche.cygnus.com>
*/
                                                                                                                                 
#ifndef __I18N_SUPPORT_H__
#define __I18N_SUPPORT_H__
                                                                                                                                 
#include "config.h"
                                                                                                                                 
#ifdef ENABLE_NLS
#    include <libintl.h>
#    ifdef EXPLICIT_TRANSLATION_DOMAIN
#        undef _
#        define _(String) dgettext (EXPLICIT_TRANSLATION_DOMAIN, String)
#    else
#        define _(String) gettext (String)
#    endif
#    ifdef gettext_noop
#        define N_(String) gettext_noop (String)
#    else
#        define N_(String) (String)
#    endif
#else
/* Stubs that do something close enough.  */
#    define textdomain(String) (String)
#    define gettext(String) (String)
#    define dgettext(Domain,Message) (Message)
#    define dcgettext(Domain,Message,Type) (Message)
#    define bindtextdomain(Domain,Directory) (Domain)
#    define _(String) (String)
#    define N_(String) (String)
#endif
                                                                                                                                 
#endif /* __I18N_SUPPORT_H__ */

