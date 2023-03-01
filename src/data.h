#include <Arduino.h>

void dataErase(int from, int to);
void dataCommit(void);
String dataReadAsString(int from, int to);
void dataSaveAsString(int offset, String value);
