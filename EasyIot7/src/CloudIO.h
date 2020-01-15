#ifndef CLOUDIO_H
#define CLOUDIO_H
void connectoToCloudIO();
void notifyStateToCloudIO(const char *topic,const char *state);
#endif