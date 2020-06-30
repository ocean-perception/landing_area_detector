#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()

///Basic C and C++ libraries
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <vector>

using namespace std;

int main()
{
    GDALDataset  *poDataset;
    GDALAllRegister();
//    poDataset = (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );
    poDataset = (GDALDataset *) GDALOpen( "test.tif", GA_ReadOnly );
    if( poDataset == NULL )
    {
    	cout << "Error when opening file" << endl;
    	return -1;
    }

   	cout << "GDAL succesfully read test.tif in CWD" << endl;
   	return 0;
}