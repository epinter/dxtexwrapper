#pragma once

#include "DirectXTex.h"
#include "Data.h"

extern "C" [[maybe_unused]] __declspec(dllexport) Data convertDDSToTIFF(unsigned int size, const void *data);

void printErrorDescription(HRESULT hr);
