// This file is part of fityk program. Copyright (C) 2003 Stefan Krumm  


#ifndef FILEROUTINES__H__
#define FILEROUTINES__H__


#include <iostream>
#include <sstream>
#include <stdio.h>

#include "data.h"


void load_siemensbruker_filetype (std::string filename, Data *data);
int frint(int pos, FILE *stream);
char *frstr(int pos,int cnt,FILE *stream);
float frfloat(int pos,FILE *stream);
int frshort(short pos,FILE *stream);


#endif
