//
// Created by Torsten (ttl) 23.08.2019
//

#include "threepl/ThreeplImport.h"
#include "threepl/prpl/buddy.h"

ThreeplImport::ThreeplImport (PurpleConnection* connection,
                              char* password, char* backup_file)
  : m_connection (connection), m_password (password), m_backup_file (backup_file)
{
}

ThreeplImport::~ThreeplImport (void)
{
}

ThreeplImport::imported ThreeplImport::import_contacts (void)
{
  ThreeplImport::imported retval = {-1,-1};

  if (! open_contacts_file ())
    return retval;

  retval = scan_contacts_file ();

  close_contacts_file ();

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
  zip_int64_t read = zip_fread (m_contacts, &c, 1);
  
  // skip header and read first char of first data line
  while (((read = zip_fread (m_contacts, &c, 1)) > 0) && (c[0] != '\n'));
  read = zip_fread (m_contacts, &c, 1);

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
    read = zip_fread (m_contacts, &c, 1);

  }

  return buddies;
}


// private functions

bool ThreeplImport::open_contacts_file (void)
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

  m_contacts = zip_fopen (m_backup, "contacts.csv", ZIP_FL_UNCHANGED);

  if (! m_contacts) {
    purple_notify_error(m_connection,
                       "Error imorting contacts and groups",
                       "Could not read contacts from file",
                       m_backup_file);
    return false;
  }

  return true;
}
  
void ThreeplImport::close_contacts_file (void)
{
  zip_fclose (m_contacts);
  zip_close (m_backup);

}