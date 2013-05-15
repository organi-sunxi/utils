#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H

#define LOGMESSAGE()		do {int fd;\
								if ((fd = open("logMessage", O_WRONLY |\
												O_CREAT | O_APPEND,\
												0777)) < 0) {\
									return;\
								}\
								write(fd, "LOGMESSAGE\n", 11);\
								close(fd);\
							}while(0)


#endif //logmessage.h
