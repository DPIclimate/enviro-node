#ifndef WOMBAT_FTP_STACK_H
#define WOMBAT_FTP_STACK_H

bool ftp_login(void);
bool ftp_logout(void);
bool ftp_get(const char * filename);
bool ftp_upload_file();

#endif //WOMBAT_FTP_STACK_H
