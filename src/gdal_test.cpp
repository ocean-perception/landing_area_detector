/**
 * @file gdal_test.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief GDAL testing sandbox
 * @version 0.1
 * @date 2020-07-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <gdal_priv.h>
#include <cpl_conv.h> // for CPLMalloc()
// GDAL specific libraries

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

	double adfGeoTransform[6];

	cout << "Driver: " << poDataset->GetDriver()->GetDescription() << "/" << poDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) << endl;
	cout << "Size is X: " << poDataset->GetRasterXSize() << " Y: " << poDataset->GetRasterYSize() << " C: " << poDataset->GetRasterCount() << endl; 

	GDALRasterBand  *poBand;

	int             nBlockXSize, nBlockYSize;
	int             bGotMin, bGotMax;
	double          adfMinMax[2];

	poBand = poDataset->GetRasterBand(1);  // 1-indexed band number. We retrieve the first (and unique band)
	poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
	printf( "Block=%dx%d Type=%s, ColorInterp=%s\n",
			nBlockXSize, nBlockYSize,
			GDALGetDataTypeName(poBand->GetRasterDataType()),
			GDALGetColorInterpretationName(
				poBand->GetColorInterpretation()) );
	adfMinMax[0] = poBand->GetMinimum( &bGotMin );
	adfMinMax[1] = poBand->GetMaximum( &bGotMax );
	if( ! (bGotMin && bGotMax) )
		GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
	printf( "Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1] );
	if( poBand->GetOverviewCount() > 0 )
		printf( "Band has %d overviews.\n", poBand->GetOverviewCount() );
	if( poBand->GetColorTable() != NULL )
		printf( "Band has a color table with %d entries.\n",
				poBand->GetColorTable()->GetColorEntryCount() );

	cout << "Units: " << poBand->GetUnitType() << endl;

	int bGotNodata = FALSE;
    const double dfNoData = GDALGetRasterNoDataValue (GDALGetRasterBand( poDataset, 1 ), &bGotNodata);

	// TODO: deal with GMF_NODATA & Masks
	// See: https://gdal.org/development/rfc/rfc15_nodatabitmask.html#rfc-15
/*	The GDALRasterBand class will include a default implementation of GetMaskBand() that returns one of three default implementations.
    If a corresponding .msk file exists it will be used for the mask band.
    If the band has a nodata value set, an instance of the new GDALNodataMaskRasterBand class will be returned. GetMaskFlags() will return GMF_NODATA.
    If there is no nodata value, but the dataset has an alpha band that seems to apply to this band (specific rules yet to be determined)
	and that is of type GDT_Byte then that alpha band will be returned, and the flags GMF_PER_DATASET and GMF_ALPHA will be returned in the flags.
    If neither of the above apply, an instance of the new GDALAllValidRasterBand class will be returned that has 255 values for all pixels. The null flags will return GMF_ALL_VALID.
//*/

	if (bGotNodata == FALSE){
		cout << "Current band does not provide explicit no-data field definition" << endl;
	}
	else{
		if (CPLIsNan(dfNoData)){ //test if provided NoData is NaN
			cout << "NoData value: NaN --> " << dfNoData << endl;
		}
		else{
			cout << "NoData value: " << dfNoData << endl;
		}
	}
	//*/
	// GET UNITS NAME
	// UNIT CONVERSION FACTOR
	// NAMES AND ORDERING OF THE AXES

	if( poDataset->GetProjectionRef()  != NULL )
	    cout << "Projection is " << poDataset->GetProjectionRef() << endl;
	if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
	{
	    cout << "Origin = " <<  adfGeoTransform[0] << ", " << adfGeoTransform[3] << endl;
	    cout << "Pixel Size = " << adfGeoTransform[1] << ", " << adfGeoTransform[5] << endl;
	}

	GDALClose(poDataset);
   	return 0;
}


/* gdalinfo test.tiff output:

Driver: GTiff/GeoTIFF
Files: test.tif
Size is 2195, 1853
Coordinate System is:
GEOGCS["WGS 84",
    DATUM["WGS_1984",
        SPHEROID["WGS 84",6378137,298.257223563,
            AUTHORITY["EPSG","7030"]],
        AUTHORITY["EPSG","6326"]],
    PRIMEM["Greenwich",0],
    UNIT["degree",0.0174532925199433],
    AUTHORITY["EPSG","4326"]]
Origin = (-20.306601000000001,-24.709499000000001)
Pixel Size = (0.010000000000000,-0.010000000000000)
Metadata:
  AREA_OR_POINT=Area
Image Structure Metadata:
  INTERLEAVE=BAND
Corner Coordinates:
Upper Left  ( -20.3066010, -24.7094990) ( 20d18'23.76"W, 24d42'34.20"S)
Lower Left  ( -20.3066010, -43.2394990) ( 20d18'23.76"W, 43d14'22.20"S)
Upper Right (   1.6433990, -24.7094990) (  1d38'36.24"E, 24d42'34.20"S)
Lower Right (   1.6433990, -43.2394990) (  1d38'36.24"E, 43d14'22.20"S)
Center      (  -9.3316010, -33.9744990) (  9d19'53.76"W, 33d58'28.20"S)
Band 1 Block=2195x1 Type=Float32, ColorInterp=Gray
  NoData Value=0

*/


/* 
  To cite GDAL/OGR in publications use:

  GDAL/OGR contributors (2020). GDAL/OGR Geospatial Data Abstraction
  software Library. Open Source Geospatial Foundation. URL https://gdal.org

A BibTeX entry for LaTeX users is

  @Manual{,
    title = {{GDAL/OGR} Geospatial Data Abstraction software Library},
    author = {{GDAL/OGR contributors}},
    organization = {Open Source Geospatial Foundation},
    year = {2020},
    url = {https://gdal.org},
  }
 */