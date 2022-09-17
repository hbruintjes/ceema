//
// Created by Torsten (ttl) 23.08.2019
//

#include "threepl/ThreeplConnection.h"
#include "zip.h"

class ThreeplImport {

  public:

    typedef struct {
      int b_new;
      int b_old;
    } imported;

    ThreeplImport (PurpleConnection* connection,
                   ThreeplConnection* tpl_connection,
                   char* password, char* backup_file);
    
    ~ThreeplImport (void);
      
    imported import_contacts (void);
    imported import_groups (void);

  private:

    bool open_file (const char* file_name);
    imported scan_contacts_file (void);
    imported scan_groups_file (void);
    void close_file (void);

    PurpleConnection* m_connection;
    ThreeplConnection* m_tpl_connection;
    char* m_password;
    char* m_backup_file;
    zip_t* m_backup;
    zip_file_t* m_file;

};
