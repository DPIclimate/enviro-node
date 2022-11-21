//
// Created by David Taylor on 16/11/2022.
//

#ifndef WOMBAT_CLI_H
#define WOMBAT_CLI_H

extern void cliInitialise(void);
extern void repl(Stream& io);

extern bool sdi12_pt(Stream* sdi12_stream);


#endif //WOMBAT_CLI_H
