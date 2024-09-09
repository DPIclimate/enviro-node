#include "Utils.h"

#ifndef WOMBAT_FTP_STACK_H
#define WOMBAT_FTP_STACK_H

bool ftp_login(void);
bool ftp_logout(void);
bool ftp_get(const char * filename);
bool ftp_upload_file(const String& filename, size_t offset = 0, bool from_end = false);

#endif //WOMBAT_FTP_STACK_H
