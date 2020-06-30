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

#include <stdio.h>

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

	double        adfGeoTransform[6];
	printf( "Driver: %s/%s\n",
	        poDataset->GetDriver()->GetDescription(),
	        poDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
	printf( "Size is %dx%dx%d\n",
	        poDataset->GetRasterXSize(), poDataset->GetRasterYSize(),
	        poDataset->GetRasterCount() );
	if( poDataset->GetProjectionRef()  != NULL )
	    printf( "Projection is `%s'\n", poDataset->GetProjectionRef() );
	if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
	{
	    printf( "Origin = (%.6f,%.6f)\n",
	            adfGeoTransform[0], adfGeoTransform[3] );
	    printf( "Pixel Size = (%.6f,%.6f)\n",
	            adfGeoTransform[1], adfGeoTransform[5] );
	}

   	return 0;
}