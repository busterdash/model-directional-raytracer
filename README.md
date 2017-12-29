# Sunlight Machine

### There are two versions of this project:
* Serial - Runs on Windows or Linux.
* Threaded - Runs exclusively on Linux; uses POSIX threads. To use, please switch out main.cpp with the version within the 'threaded' folder. Must then be compiled with the flag: ```-lpthread```

## Components

### SMD Reader:
* So far it doesn't keep track of bones, it will only read root bones.
* It doesn't care about materials. For this project we're only concerned about geometry.
* Accessing a value is done like this: ```triangle->v[0]->u;```

### Bitmap FileIO:
* Submodule can be read about here: https://github.com/busterdash/basic-bitmap-fileio
* Writes windows bitmaps with a DIB of type ```BITMAPINFOHEADER```.
* Images compatible with Windows 3.1 and above.

### Raytracer:
* Tosses a two-dimensional grid of photons at the input model.
* Each photon that hits the model is converted to a coordinate on the texture and plotted by the Bitmap Writer.

## Cloning
```
git clone https://github.com/busterdash/sunlight-machine
cd sunlight-machine
git submodule update --init
```

## Compiling
* For simplicity, use Orwell Dev-C++ to compile on Windows.
* I will provide a makefile in the future.

## Running
* ./Raytracer -in "model.smd" -out "texture.bmp"

|Additional Flags|Default Value|
|----------------|-------------|
|-width [image_width]|512|
|-height [image_height]|512|
|-yaw [sun_angle]|15.0|
|-pitch [sun_angle]|30.0|
|-resolution [sqrt_of_number_of_rays]|512|
|-spread [scale]|1.0|
