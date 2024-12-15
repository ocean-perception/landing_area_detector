# Landing Area Detector

This project is a C++ implementation of the Generalized Landing Aread Detection algorithm part of a pipeline to estimate terrain landability for field robotics applications such as underwater, areial (drone) and planetary landers. It computes a terrain landability map from the vehicle (AUV/ROV/Drone/Lander) characteristics and a high-resolution terrain evelation data (GeoTIFF or unstructured XYZ files). The approach is based on the algorithm described in:

> **Autonomous Landing of Underwater Vehicles Using High-Resolution Bathymetry**  
> [https://ieeexplore.ieee.org/document/8897695]

## Key Features

- **Multi-threaded processing:** Utilizes OpenMP for parallel acceleration.
- **Optional CUDA support:** Enable GPU acceleration with `-DUSE_CUDA=ON`.
- **GeoTIFF & Raster Image Processing:** Uses GDAL and OpenCV for image I/O and transformations.
- **3D Geometry Computation:** Employs CGAL for plane fitting, convex hulls, and alpha-shapes.
- **Configuration via YAML:** Easily define vehicle and environment parameters without modifying the code.

## Dependencies

- **CMake:** >= 3.23
- **C++ Standard:** C++17 or newer
- **OpenCV:** >= 4.6
- **GDAL:** Latest recommended
- **Boost:** >= 1.74
- **CGAL:** Latest recommended
- **OpenMP:** For parallel processing, 2.0 on Windows and 4.0 on Linux
- **yaml-cpp:** For experiment configuration

**Optional:**
- **CUDA:** If `USE_CUDA=ON` is specified during configuration, currently on experimentation.

## Supported Platforms

- **Linux (e.g. Ubuntu 20.X, 22.X and 24.04)**: Fully supported.
- **Windows (e.g. via Git-Bash)**: Should be compatible, using standard CMake platform detection and toolchains.

## Building

```bash
mkdir build
cd build
cmake ..
make
```

CMake will  automatically detect the necessary dependencies and build the project. If CMAKE_BUILD_TYPE is not set, it will default to `Release` for speed performance. If you want to see the CMake configuration, run:

```bash
cmake -L ..
```

To enable CUDA support (experimental):

```bash
cmake -DUSE_CUDA=ON ..
make
```

**Note:**  
- Ensure all dependencies are available on your system (via package managers or from source).
- You can adjust the installation directory and other parameters using standard CMake variables and options.

## Installation (optional)

After building, install the executables to `$HOME/bin`:

```bash
make install
```

This will copy the executables to `$HOME/bin` folder. Please add this folder to your system's PATH environment variable.

## Executables

The project builds three main executable targets:

- **land**: Main landability calculation pipeline  
- **tiff2rugosity**: Rugosity calculation from GeoTIFF  
- **img.resample**: Image resampling utility

Each tool integrates versioning information from the git repository (including commit hash) and is configured to handle various input formats as specified via YAML configuration files.

## Usage

Please refer to the specific tool's documentation for detailed usage instructions. Each tool provide a help option (`-h` or `--help`) to display usage information. For example:

```bash
./land --help
```

will display the help message for the `land` tool:
```bash
mad_test
	Version:	4.0.1
	Git hash:	ee4e570
	Mode:		Release
	OpenCV:		4.7.0
  ./land {OPTIONS}

    lad_test - testing module part of [landing-area-detection] pipeline Compatible interface with geoTIFF bathymetry
    datasets via GDAL + OpenCV

  OPTIONS:

      -h, --help                        Display this help menu
      --input=[input]                   Input bathymetry map. TIFF file or XYZ point collection
      -o[output], --output=[output]     Output file basename that will be used as prefix of all exported layers
      -p[path], --outpath=[path]        (NOT-IMPLEMENTED) Output folder path that will contain files will be exported
      --verbose=[verbose]               Define verbosity level, 0 - 3
      --nowait
      --saveintermediate=[value]        Define if to export intermediate rotation-identependent maps (Lane A & B)
      --terrainonly                     Run terrain only calculations (maps for Lane A & B which are AUV independent)
      --nthreads=[number]               Define max number of threads
      --config=[file.yaml]              Provides path to file with user defied configuration
      --meta=[ratio]                    Recompute metacenter distance from vehicle height
      --alpharadius=[alpha]             Search radius for alpha Shape concave hull algorithm
      --rotation=[rotation]             Vehicle rotation in degrees. Defined as ZERO heading NORTH, positive CCW
      --rotstep=[angle]                 Rotation step resolution in degrees.
      --int=[param]                     User defined parameter INTEGER for testing purposes
      --float=[param]                   User defined parameter FLOAT for testing purposes
      --robotheight=[height]            User defined robot height in meters
      --robotwidth=[width]              User defined robot width in meters
      --robotlength=[length]            User defined robot length in meters
      --prot_size=[size]                Size threshold [cm] to consider a protrusion an obstacle
      --height_th=[height]              Height threshold [m] to determine high obstacles
      --slope_th=[slope]                Slope threshold [deg] to determine high slope areas
      --ground_th=[length]              Minimum height [m] to consider an obstacle
      --valid_th=[ratio]                Minimum ratio of required valid pixels to generate PNG
      --slope_algorithm=[method]        Select terrain slope calculation algorithm: PLANE | CONVEX
```