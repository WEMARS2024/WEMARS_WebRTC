#pragma once
#include "CameraSource.hpp"

//WIP
class CameraManager{ 
public:

    CameraSource initCamera();
    bool closeCamera();

private:
    bool registerCamera();
    bool deRegisterCamera();
};