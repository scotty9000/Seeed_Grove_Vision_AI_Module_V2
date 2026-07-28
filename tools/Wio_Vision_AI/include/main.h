#pragma once

#include <Arduino.h>
// Updated signature to handle the text credentials, the long image string, and a photo caption
void sendTelegramPhotoAlert(const char* token, const char* chat, String base64ImageStr, String captionText);

