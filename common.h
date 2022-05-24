#ifndef COMMON_H
#define COMMON_H

bool send_all(int fd, char *buffer, int size);
bool receive_all(int fd, char *buffer, int size);


#endif
