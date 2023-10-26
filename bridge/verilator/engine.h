#ifndef FIONA_PHOTONIC_ENGINE
#define FIONA_PHOTONIC_ENGINE

// Python Development Header
#include <Python.h>
#include <abstract.h>
#include <boolobject.h>
#include <import.h>
#include <listobject.h>
#include <longobject.h>
#include <object.h>
#include <tupleobject.h>
#include <unicodeobject.h>

// Standard Library Header
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <utility>

// System-Verilog Direct Programming Interface
#include <svdpi.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void init_python_env();
extern void deinit_python_env();

extern void array_handle1(char*, char*, svOpenArrayHandle, svOpenArrayHandle);
extern void array_handle2(char*, char*, svOpenArrayHandle, svOpenArrayHandle, svOpenArrayHandle, 
                          svBitVecVal*, svBitVecVal*, svBitVecVal*, svBitVecVal*, svBitVecVal*, svBitVecVal*);

#ifdef __cplusplus
}
#endif


#endif /* FIONA_PHOTONIC_ENGINE */
