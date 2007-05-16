// This file is part of fityk program. Copyright (C) 2003 Stefan Krumm  
// Licence: GNU General Public License version 2
// $Id$


#include <fstream>
#include <sstream>

#include "fileroutines.h"
#include "common.h"
#include "ui.h"

void load_siemensbruker_filetype (std::string filename, Data *data)
{
	float biggest=0, smallest=99999, average=0;
  	char ctest[140];

	FILE *stream;
  	stream=fopen(filename.c_str(),"rb");
	if(!stream)
	  throw ExecuteError("Bad luck! Couldn't open the file: " + filename);

	// let's have a look at the first bytes.
	// Siemens files tell us something there...
	frstr(0,6,stream,ctest);
	if((strncmp("RAW2",ctest,4)!=0) &&  (strncmp("RAW1.0",ctest,5)!=0) &&  (strncmp("RAW ",ctest,4)!=0)){
	    if(strncmp("RAW_1",ctest,5)==0)
	         throw ExecuteError("This looks like a STOE raw file.");
	    else
	         throw ExecuteError("This is not a valid SIEMENS DIFFRAC AT RAW-file!");
   	}
	else{
		if(strncmp("RAW2",ctest,4)==0){
			//cerr<<"Reading old Siemens Raw format!"<<endl;
			double start=frfloat(256+16,stream);
			//scan->setStartAngle(start);
			short nop;
		    fseek(stream,258,0);
    	    fread(&nop, 2, 1, stream);
			int numofpoints=int(nop);
			//scan->setNumOfPoints(double(numofpoints));
			double stepsize=frfloat(256+12,stream);
			//scan->setStepSize(double(stepsize));
			//cerr<<"The end: "<<double(numofpoints-1)*stepsize<<endl;

			//? double xmax=start+(double(numofpoints-1))*stepsize;
            //scan->setEndAngle(xmax);
            //scan->setEndAngle(stepsize*double(numofpoints)+start);
			//cerr<<start<<"  "<<xmax<<"  "<<stepsize<<"  "<<numofpoints<<"      "<<endl;

			/*lamda1=frfloat(190,stream);
			lamda2=frfloat(194,stream);
			lamratio=frfloat(198,stream);
			lamda=(lamda1*2+lamda2)/3;
			ogrenzey=5000;*/
			fseek(stream,256,0);
    	    fread(&nop, 2, 1, stream);
			int dataoffset=256+int(nop);
			for(int z=0;z<=numofpoints;z++)	{
			    double x=stepsize*double(z)+start;
	  			fseek(stream,dataoffset+z*4,0);
			    float y;
			    fread(&y, 4, 1, stream);
	            if(y>=biggest)biggest=y;
                if (y<=smallest && y>0)smallest=y;
                average+=y;
	            //scanDataPoint* p = new scanDataPoint(x,double(y),0.00);
                //scan->appendDataPoint(p);
                data->add_point (Point (x, y));

  			}
	        //scan->setMaxIntensity(double(biggest));
            //scan->setMinIntensity(double(smallest));
            //scan->setAverageIntensity(double(average/double(numofpoints)));
		}
	    if(strncmp("RAW1.0",ctest,6)==0){
	       //cerr<<"Reading latest Siemens/Bruker/Axs format..."<<endl;
	       fseek(stream,728,0);
    	   double start;
	       fread(&start, 8, 1, stream);
		   //scan->setStartAngle(start);
		   int numofpoints;
		   fseek(stream,716,0);
    	   fread(&numofpoints, 4, 1, stream);
		   //scan->setNumOfPoints(double(numofpoints));
    	   double stepsize;
    	   fseek(stream,888,0);
	       fread(&stepsize, 8, 1, stream);
		   //scan->setStepSize(stepsize);
		   //cerr<<"The end: "<<double(numofpoints-1)*stepsize<<endl;
		   //? double xmax=start+(double(numofpoints-1))*stepsize;
           //scan->setEndAngle(xmax);
           //scan->setEndAngle(stepsize*double(numofpoints)+start);
			//cerr<<start<<"  "<<xmax<<"  "<<stepsize<<"  "<<numofpoints<<"      "<<endl;

			/*lamda1=frfloat(190,stream);
			lamda2=frfloat(194,stream);
			lamratio=frfloat(198,stream);
			lamda=(lamda1*2+lamda2)/3;
			ogrenzey=5000;*/
		    int dataoffset=1016;
			for(int z=0;z<=numofpoints;z++)	{
			    double x=stepsize*double(z)+start;
	  			fseek(stream,dataoffset+z*4,0);
			    float y;
			    fread(&y, 4, 1, stream);
	            if(y>=biggest)biggest=y;
                if(y<=smallest && y>0)smallest=y;
                average+=y;
	           //scanDataPoint* p = new scanDataPoint(x,double(y),0.00);
               //scan->appendDataPoint(p);
			   data->add_point (Point (x, y));
			}
	        //scan->setMaxIntensity(double(biggest));
            //scan->setMinIntensity(double(smallest));
            //scan->setAverageIntensity(double(average/double(numofpoints)));
	/*
	fseek(stream,888,0);
	fread(&doubledum, 8, 1, stream);
	schritt=float(doubledum);
        stepsize=schritt;
	xmax=xmin+(anzahl-1)*schritt;
	xwert[0]=xmin;

	fseek(stream,624,0);
	fread(&doubledum, 8, 1, stream);
	lamda1=float(doubledum);

	fseek(stream,632,0);
	fread(&doubledum, 8, 1, stream);
	lamda2=float(doubledum);

	fseek(stream,648,0);
	fread(&doubledum, 8, 1, stream);

	lamratio=float(doubledum);
	lamda=(lamda1*2+lamda2)/3;
	ogrenzey=5000;
	datenoffset=1016;//256+frint(256,stream);
	if(!ywert)
          BWCCMessageBox(0,"To few memory available!\nStop concurrent programs or split file!","Memory Allocation Error",MB_OK);
	SetCursor(LoadCursor(0,IDC_WAIT));
      	for(z=0;z<=anzahl;z++)
	{
	  //ywert[z]=frfloat(z*4+datenoffset,stream);
	  xwert[z]=xmin+z*schritt;
		  if(z>15000)break;
	}
	fseek(stream,datenoffset,0);
	fread(ywert, 4, z, stream);
		}
	if(strncmp("RAW ",ctest,4)==0)

	{   //hier kommts nochmalfuer das uralte Format
	   //	MessageBox(0,"Lese uraltes Raw","",MB_OK);
	fseek(stream,24,0);
	fread(&floatdum, 4, 1, stream);
	xmin=float(floatdum);
	fseek(stream,4,0);
	fread(&longdum, 4, 1, stream);
	anzahl=int(longdum);
	sprintf(ctest,"%d",anzahl);
	//MessageBox(0,ctest,"",MB_OK);

	fseek(stream,12,0);
	fread(&floatdum, 4, 1, stream);
	schritt=float(floatdum);
        stepsize=schritt;
	xmax=xmin+anzahl*schritt;
	xwert[0]=xmin;

	fseek(stream,72,0);
	fread(&floatdum, 4, 1, stream);
	lamda1=float(floatdum);

	fseek(stream,76,0);
	fread(&floatdum, 4, 1, stream);
	lamda2=float(floatdum);


	lamratio=.5;
	lamda=(lamda1*2+lamda2)/3;
  	ogrenzey=5000;


	//datenoffset=1016;//256+frint(256,stream);
	if(!ywert)
          BWCCMessageBox(0,"To few memory available!\nStop concurrent programs or split file!","Memory Allocation Error",MB_OK);
	SetCursor(LoadCursor(0,IDC_WAIT));
      	for(z=0;z<=anzahl;z++)
	{
	  //ywert[z]=frfloat(z*4+datenoffset,stream);
	  xwert[z]=xmin+z*schritt;
          if(z>15000)break;
	}
	fseek(stream,156,0);
	fread(ywert, 4, z, stream);
        }*/
	}
 } //end of if not a raw file

}




int frint(int pos,FILE *stream)
{
   int dum;
   fseek(stream,pos,0);
   fread(&dum,sizeof(int),1,stream);
   return(dum);
}

short frshort(int pos,FILE *stream)
{
   short dum;
   fseek(stream,pos,0);
   fread(&dum,sizeof(short),1,stream);
   return(dum);
}
float frfloat(int pos,FILE *stream)
{
   float dum;
   fseek(stream,pos,0);
   fread(&dum,4,1,stream);
   return(dum);
}

void frstr(int pos,int cnt,FILE *stream,char *dum)
{

   strcpy(dum,"");
   fseek(stream,pos,0);
   fread(&dum,1,cnt,stream);
   dum[cnt]=0;
}

