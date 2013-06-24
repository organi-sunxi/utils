#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H

extern FILE *plogfile;
#define LOG(...)	if(plogfile)	{fprintf(plogfile, __VA_ARGS__);}

#endif //logmessage.h
