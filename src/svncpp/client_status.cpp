/*
 * ====================================================================
 * Copyright (c) 2002-2007 The RapidSvn Group.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library (in the file LGPL.txt); if not, 
 * write to the Free Software Foundation, Inc., 51 Franklin St, 
 * Fifth Floor, Boston, MA  02110-1301  USA
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://rapidsvn.tigris.org/.
 * ====================================================================
 */
#if defined( _MSC_VER) && _MSC_VER <= 1200
#pragma warning( disable: 4786 )// debug symbol truncated
#endif

// Stdlib (for strcmp)
#include "string.h"

// Subversion api
#include "svn_client.h"
#include "svn_sorts.h"
//#include "svn_utf.h"

// svncpp
#include "svncpp/client.hpp"
#include "svncpp/dirent.hpp"
#include "svncpp/exception.hpp"
#include "svncpp/pool.hpp"
#include "svncpp/status.hpp"
#include "svncpp/targets.hpp"
#include "svncpp/url.hpp"

namespace svn
{
  static svn_error_t *
  logReceiver (void *baton,
                   apr_hash_t * changedPaths,
                   svn_revnum_t rev,
                   const char *author,
                   const char *date, 
                   const char *msg, 
                   apr_pool_t * pool)
  {
    LogEntries * entries = (LogEntries *) baton;
    entries->insert (entries->begin (), LogEntry (rev, author, date, msg));

    if (changedPaths != NULL)
    {
      LogEntry &entry = entries->front ();

      for (apr_hash_index_t *hi = apr_hash_first (pool, changedPaths);
           hi != NULL;
           hi = apr_hash_next (hi))
      {
        char *path;
        void *val;
        apr_hash_this (hi, (const void **)&path, NULL, &val);

        svn_log_changed_path_t *log_item = reinterpret_cast<svn_log_changed_path_t *> (val);
        
        entry.changedPaths.push_back (
              LogChangePathEntry (path,
                                  log_item->action,
                                  log_item->copyfrom_path,
                                  log_item->copyfrom_rev) );
      }
    }

    return NULL;
  }

  static void StatusEntriesFunc (void *baton,
                                 const char *path,
                                 svn_wc_status2_t *status)
  {
    StatusEntries * entries = static_cast<StatusEntries *>(baton);

    entries->push_back (Status (path, status));
  }

  static StatusEntries
  localStatus (const char * path,
               const bool descend,
               const bool get_all,
               const bool update,
               const bool no_ignore,
               Context * context,
               const bool ignore_externals)
  {
    svn_error_t *error;
    StatusEntries entries;
    svn_revnum_t revnum;
    Revision rev (Revision::HEAD);
    Pool pool;

    error = svn_client_status2 (
      &revnum,    // revnum
      path,       // path
      rev,        // revision
      StatusEntriesFunc, // status func
      &entries,   // status baton
      descend,    // recurse
      get_all,
      update,     // need 'update' to be true to get repository lock info
      no_ignore,
      ignore_externals, // ignore_externals
      *context,   // client ctx
      pool);

    if (error!=NULL)
      throw ClientException (error);

    return entries;
  }
  
  static Status 
  dirEntryToStatus (const char * path, const DirEntry & dirEntry)
  {
    Pool pool;

    svn_wc_entry_t * e =
      static_cast<svn_wc_entry_t *> (
        apr_pcalloc (pool, sizeof (svn_wc_entry_t)));

    std::string url (path);
    url += "/";
    url += dirEntry.name ();

    e->name = dirEntry.name ();
    e->revision = dirEntry.createdRev ();
    e->url = url.c_str ();
    e->kind = dirEntry.kind ();
    e->schedule = svn_wc_schedule_normal;
    e->text_time = dirEntry.time ();
    e->prop_time = dirEntry.time ();
    e->cmt_rev = dirEntry.createdRev ();
    e->cmt_date = dirEntry.time ();
    e->cmt_author = dirEntry.lastAuthor ();

    svn_wc_status2_t * s =
      static_cast<svn_wc_status2_t *> (
        apr_pcalloc (pool, sizeof (svn_wc_status2_t)));
    s->entry = e;
    s->text_status = svn_wc_status_normal;
    s->prop_status = svn_wc_status_normal;
    s->locked = 0;
    s->switched = 0;
    s->repos_text_status = svn_wc_status_normal;
    s->repos_prop_status = svn_wc_status_normal;

    return Status (url.c_str (), s);
  }

  static StatusEntries
  remoteStatus (Client * client,
                const char * path,
                const bool descend,
                const bool get_all,
                const bool update,
                const bool no_ignore,
                Context * context)
  {
    Revision rev (Revision::HEAD);
    DirEntries dirEntries = client->list (path, rev, descend);
    DirEntries::const_iterator it;
    
    StatusEntries entries;

    for (it = dirEntries.begin (); it != dirEntries.end (); it++)
    {
      const DirEntry & dirEntry = *it;

      entries.push_back (dirEntryToStatus (path, dirEntry));
    }

    return entries;
  }

  StatusEntries 
  Client::status (const char * path,
                  const bool descend,
                  const bool get_all,
                  const bool update,
                  const bool no_ignore,
                  const bool ignore_externals) throw (ClientException)
  {
    if (Url::isValid (path))
      return remoteStatus (this, path, descend, get_all, update, 
                           no_ignore, m_context);
    else
      return localStatus (path, descend, get_all, update, 
                          no_ignore, m_context, ignore_externals);
  }

  Status 
  Client::singleStatus (const char * path) throw (ClientException)
  {
    Revision rev (Revision::HEAD);
    StatusEntries entries = status (path, false);

    // now seach the returned list of entries for @a path
    StatusEntries::const_iterator it=entries.begin ();

    for(; it!=entries.end (); it++)
    {
      Status status=*it;

      if (strcmp (status.path (), path) == 0)
        return status;
    }

    // if we come to this point we havent found a thing
    return Status ();
  }

  const LogEntries *
  Client::log (const char * path, const Revision & revisionStart, 
               const Revision & revisionEnd, bool discoverChangedPaths,
               bool strictNodeHistory ) throw (ClientException)
  {
    Pool pool;
    Targets target (path);
    LogEntries * entries = new LogEntries ();
    svn_error_t *error;
    int limit = 0;

    error = svn_client_log2 (
      target.array (pool), 
      revisionStart.revision (), 
      revisionEnd.revision (), 
      limit,
      discoverChangedPaths ? 1 : 0,
      strictNodeHistory ? 1 : 0,
      logReceiver,
      entries, 
      *m_context, // client ctx
      pool);

    if (error != NULL)
    {
      delete entries;
      throw ClientException (error);
    }

    return entries;
  }

  Entry
  Client::info (const char * path)
  {
    Pool pool;
    svn_wc_adm_access_t * adm_access;

    svn_error_t * error =
      svn_wc_adm_probe_open (&adm_access, NULL, path, FALSE,
                             FALSE, pool);
    if (error != NULL)
      throw ClientException (error);

    const svn_wc_entry_t * entry;
    error = svn_wc_entry (&entry, path, adm_access, FALSE, pool);

    if (error != NULL)
      throw ClientException (error);

    // entry may be NULL
    return Entry( entry );
  }
}

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../../rapidsvn-dev.el")
 * end:
 */
