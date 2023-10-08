#ifndef __SERVER_H
#define __SERVER_H

#include <Arduino.h>

void setupServer(const char *ssid, const char *password);
void processServerClient();
void registerRoute(arduino::String route, void (*routeCallback)(arduino::String value));

#endif