/**
 * Forward-Declarations aller Web-Page-Handler.
 *
 * Wird von airrohr-firmware.ino inkludiert, damit die `server.on()`-Registrierungen
 * die Funktionen sehen, die in web/pages/*.cpp leben.
 *
 * Issue #18 Phase B
 */
#pragma once

void webserver_reset();
