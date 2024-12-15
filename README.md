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

After building and installing, run the executables from `$HOME/bin`. For example:

```bash
$HOME/bin/land --config config.yaml --input bathymetry.tif
$HOME/bin/tiff2rugosity --input dem.tif --output rugosity_map.tif
$HOME/bin/img.resample --input large_image.tif --output resized_image.tif
```
