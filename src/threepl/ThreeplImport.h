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
                   char* password, char* backup_file);
    
    ~ThreeplImport (void);
      
    imported import_contacts (void);

  private:

    bool open_contacts_file (void);
    imported scan_contacts_file (void);
    void close_contacts_file (void);

    PurpleConnection* m_connection;
    char* m_password;
    char* m_backup_file;
    zip_t* m_backup;
    zip_file_t* m_contacts;

};
