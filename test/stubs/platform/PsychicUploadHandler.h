#pragma once
#include "PsychicHttp.h"
class PsychicUploadHandler : public PsychicHandler {
public:
    template<typename T> void onUpload(T cb) {}
    template<typename T> void onRequest(T cb) {}
};
