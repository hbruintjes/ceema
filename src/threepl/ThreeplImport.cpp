//
// Created by Torsten (ttl) 23.08.2019
//

#include "threepl/ThreeplImport.h"
#include "threepl/prpl/buddy.h"
#include "threepl/prpl/chat.h"
#include "MessageHandler.h"
#include "types/bytes.h"
#include "libpurple/debug.h"

ThreeplImport::ThreeplImport (PurpleConnection* connection,
                              ThreeplConnection* tpl_connection,
                              char* password, char* backup_file)
  : m_connection (connection), m_tpl_connection (tpl_connection),
    m_password (password), m_backup_file (backup_file)
{
}

ThreeplImport::~ThreeplImport (void)
{
}

ThreeplImport::imported ThreeplImport::import_contacts (void)
{
  ThreeplImport::imported retval = {-1,-1};

  if (! open_file ("contacts.csv"))
    return retval;

  retval = scan_contacts_file ();

  close_file ();

  return retval;
}

ThreeplImport::imported ThreeplImport::import_groups (void)
{
  ThreeplImport::imported retval = {-1,-1};

  if (! open_file ("groups.csv"))
    return retval;

  retval = scan_groups_file ();

  close_file ();

  return retval;
}

ThreeplImport::imported ThreeplImport::scan_contacts_file (void)
{
  // the states while scanning the csv formatted contacts file,
  // there is a difference of 2 since we also have ',' between '"'
  enum states {
    NEW_LINE = 0,
    PARSE_ID     =  1,
    PARSE_PUBKEY =  3,
    PARSE_VRFY   =  5,
    PARSE_ACID   =  7,
    PASRE_TACID  =  9,
    PARSE_FNAME  = 11,
    PARSE_LNAME  = 13,
    PARSE_ALIAS  = 15,
    PARSE_COLOR  = 17,
    PARSE_HIDDEN = 19
  };

  // definition of required string variables for storing contact data
  std::string id ("");
  std::string alias ("");
  std::string fname (""), lname ("");

  // the initial state
  int state = NEW_LINE;

  // no imported contact do far, read the first character of contacts file
  imported buddies = {0,0};
  char c[2];
  zip_int64_t read = zip_fread (m_file, &c, 1);

  // skip header and read first char of first data line
  while (((read = zip_fread (m_file, &c, 1)) > 0) && (c[0] != '\n'));
  read = zip_fread (m_file, &c, 1);

  // loop over all remaining data lines, where only on char is read at a time
  while (read > 0) {

    // check the read char
    switch (c[0]) {

      case '"':
        // csv: next char is delimiter or the next data field,
        // increase state
        state++;
        break;

      case '\n': {
        // new line: data line ready, get buddy information
        state = NEW_LINE;
        
        // check for buddy already existing
        if (purple_find_buddy (m_connection->account,
                               const_cast<char*> (id.c_str ()))) {

            buddies.b_old ++;

        } else {

          // if this is the first item, add group "Threema"
          PurpleGroup * group = purple_group_new ("Threema");
          purple_blist_add_group (group, NULL);

          if (alias.empty () || (alias == id)) {
            if ( (! fname.empty ()) && (! lname.empty ()))
              alias = fname + " " + lname;  // make sure there is a real alias
            } else { if (! fname.empty ())
                alias = fname;
              else if (! lname.empty ())
                alias = lname;
            }

          PurpleBuddy* buddy = purple_buddy_new (m_connection->account,
                                                 const_cast<char*> (id.c_str ()),
                                                 const_cast<char*> (alias.c_str ()));

          // add buddy to list and increase the import counter
          purple_account_add_buddy_with_invite (m_connection->account, buddy, NULL);
          purple_blist_add_buddy (buddy, NULL, group, NULL);
          buddies.b_new ++;

        }

        // reset the string for storing the new data
        id = "";
        alias = "";
        fname = "",
        lname = "";

        break;
      }

      default:
        // normal char, add to target, which belongs to the current state
        switch (state) {
          case PARSE_ID:
            id += c[0];
            break;
          case PARSE_FNAME:
            fname += c[0];
            break;
          case PARSE_LNAME:
            lname += c[0];
            break;
          case PARSE_ALIAS:
            alias += c[0];
            break;
        } // switch (state)

    } // switch (c[0])

    // read next char and repeat the loop if we have still one
    read = zip_fread (m_file, &c, 1);

  }

  return buddies;
}

ThreeplImport::imported ThreeplImport::scan_groups_file (void)
{
  // the states while scanning the csv formatted contacts file,
  // there is a difference of 2 since we also have ',' between '"'
  enum states {
    NEW_LINE = 0,
    PARSE_ID      =  1,
    PARSE_OWNER   =  3,
    PARSE_NAME    =  5,
    PARSE_CTIME   =  7,
    PARSE_MEMBERS =  9,
    PARSE_DEL     = 11
  };

  // definition of required string variables for storing contact data
  std::string id ("");
  std::string owner ("");
  std::string name ("");
  std::string member ("");
  std::vector<ceema::client_id> members;

  // the initial state
  int state = NEW_LINE;

  // no imported contact do far, read the first character of contacts file
  imported groups = {0,0};
  char c[2];
  zip_int64_t read = zip_fread (m_file, &c, 1);
  GroupStore g_store = m_tpl_connection->group_store ();

  // skip header and read first char of first data line
  while (((read = zip_fread (m_file, &c, 1)) > 0) && (c[0] != '\n'));
  read = zip_fread (m_file, &c, 1);

  // loop over all remaining data lines, where only on char is read at a time
  while (read > 0) {

    // check the read char
    switch (c[0]) {

      case '"':
        // csv: next char is delimiter or the next data field,
        // increase state
        state++;
        break;

      case '\n': {
        // new line: data line ready, get buddy information
        state = NEW_LINE;

        members.push_back (ceema::client_id::fromString (member.c_str ()));

        ceema::client_id cid = ceema::client_id::fromString (owner.c_str ());
        ceema::byte_array <16> id_ba;
        strncpy (reinterpret_cast<char*>(id_ba.data()), id.c_str (), 16);
        ceema::group_id gid (ceema::hex_decode(id_ba));

        ThreeplGroup* chat = g_store.find_group (cid, gid, false);

        if (chat) {

          groups.b_old ++;

          chat->set_name (name);
          chat->set_members (members);

          PurpleChat* pchat = chat->find_blist_chat (m_connection->account);
          chat->update_blist_chat (pchat);

        } else {

          PurpleGroup* group = purple_group_new ("Threema Groups");
          purple_blist_add_group (group, NULL);

          chat = g_store.add_group (cid, gid);
          groups.b_new ++;

          chat->set_name (name);
          chat->set_members (members);

          PurpleChat* pchat = purple_chat_new (m_connection->account, id.c_str (),
                          threepl_chat_info_defaults (m_connection, chat->chat_name ().c_str ()));
          purple_blist_add_chat (pchat, group, NULL);

        }

        g_store.update_chat (m_connection->account, *chat);

        // reset the string for storing the new data
        id = "";
        owner = "";
        name = "";
        member = "";
        members.clear ();

        break;
      }

      default:
        // normal char, add to target, which belongs to the current state
        switch (state) {
          case PARSE_ID:
            id += c[0];
            break;
          case PARSE_OWNER:
            owner += c[0];
            break;
          case PARSE_NAME:
            name += c[0];
            break;
          case PARSE_MEMBERS:
            if (c[0] == ';') {
              members.push_back (ceema::client_id::fromString (member.c_str ()));
              member = "";
            } else
              member += c[0];
            break;
        } // switch (state)

    } // switch (c[0])

    // read next char and repeat the loop if we have still one
    read = zip_fread (m_file, &c, 1);

  }

  g_store.update_chats (m_connection->account);

  return groups;
}

// private functions

bool ThreeplImport::open_file (const char* file_name)
{
  int err = 0;
  m_backup = zip_open(m_backup_file, ZIP_RDONLY, &err);

  if (err) {
    purple_notify_error(m_connection,
                       "Error imorting contacts and groups",
                       "Could not open file",
                       m_backup_file);
    return false;
  }

  zip_set_default_password (m_backup, m_password);

  m_file = zip_fopen (m_backup, file_name, ZIP_FL_UNCHANGED);

  if (! m_file) {
    const char *err_msg = zip_strerror (m_backup);
    std::string err_file = std::string ("Could not read file ") + std::string (file_name);
    purple_notify_error(m_connection,
                       "Error imorting data",
                       err_file.c_str (),
                       err_msg);
    return false;
  }

  return true;
}
  
void ThreeplImport::close_file (void)
{
  zip_fclose (m_file);
  zip_close (m_backup);

}