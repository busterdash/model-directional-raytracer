/*==============================================================================================

    main.cpp
    Copyright 2019 Buster Schrader

    This file is part of The Sunlight Machine.

    The Sunlight Machine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    The Sunlight Machine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with The Sunlight Machine.  If not, see <https://www.gnu.org/licenses/>.
    
==============================================================================================*/

#include <iostream>
#include <cmath>
#include <cstring>
#include <string>
#include <iomanip>
#include <sys/time.h>
#include <stdlib.h>
#include "raytracer.hpp"
#include "../lodepng/lodepng.h"
#include "smd_model_reader.hpp"

int resolution = 512;
float const PI = 3.141593f;
float spread = 1.0f;

void point_trans_rot_z(float angle, float* x, float* y, float* z);
void point_trans_rot_y(float angle, float* x, float* y, float* z);
void point_trans_rot_yz(float pitch, float yaw, float* x, float* y, float* z);
void perform_raytrace(std::string smd_in, std::string png_out, int tex_width, int tex_height, float sun_pitch, float sun_yaw);

//Returns the current time in microseconds.
long long start_timer()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

//Prints the time elapsed since the specified time.
long long stop_timer(long long start_time, std::string name)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long end_time = tv.tv_sec * 1000000 + tv.tv_usec;
    std::cout << std::setprecision(5);    
    std::cout << name << ": " << ((float) (end_time - start_time)) / (1000 * 1000) << " sec\n";
    return end_time - start_time;
}

int main(int argc, char** argv)
{
    std::string inpath;
    std::string outpath;
    int img_width = 512;
    int img_height = 512;
    float yaw = 15.0f;
    float pitch = 30.0f;

    for (int i = 1; i < argc; i++) //Deal with flags.
    {
        if (!strcasecmp(argv[i],"-in"))
            inpath = std::string(argv[i+1]);
        else if (!strcasecmp(argv[i],"-out"))
            outpath = std::string(argv[i+1]);
        else if (!strcasecmp(argv[i],"-width"))
            img_width = atoi(argv[i+1]);
        else if (!strcasecmp(argv[i],"-height"))
            img_height = atoi(argv[i+1]);
        else if (!strcasecmp(argv[i],"-yaw"))
            yaw = atof(argv[i+1]);
        else if (!strcasecmp(argv[i],"-pitch"))
            pitch = atof(argv[i+1]);
        else if (!strcasecmp(argv[i],"-resolution"))
            resolution = atoi(argv[i+1]);
        else if (!strcasecmp(argv[i],"-spread"))
            spread = atof(argv[i+1]);
    }
    
    perform_raytrace(inpath, outpath, img_width, img_height, pitch, yaw);
    return 0;
}

void point_trans_rot_z(float angle, float* x, float* y, float* z)
{
    float ox, oy, oz, pc, ps;
    ox = *x;
    oy = *y;
    oz = *z;
    pc = cos(angle*PI/180);
    ps = sin(angle*PI/180);
    *x = (ox * pc) + (oy * -ps);
    *y = (ox * ps) + (oy * pc);
    *z = oz;
}

void point_trans_rot_y(float angle, float* x, float* y, float* z)
{
    float ox, oy, oz, pc, ps;
    ox = *x;
    oy = *y;
    oz = *z;
    pc = cos(angle*PI/180);
    ps = sin(angle*PI/180);
    *x = (ox * pc) + (oz * ps);
    *y = oy;
    *z = (ox * -ps) + (oz * pc);
}

unsigned int get_image_index(unsigned int x, unsigned int y, unsigned int img_width)
{
    return (y*img_width*4 + x*4);
}

void apply_aliasing(std::vector<unsigned char> &img, unsigned int tex_width, unsigned int tex_height)
{
    unsigned int pos = 0;
    
    for (unsigned int y = 0; y < tex_height; y++)
    {
        for (unsigned int x = 0; x < tex_width; x++)
        {
            pos = get_image_index(x,y,tex_width);
            
            if (x > 1 && x < tex_width-1 && img[pos] == 0xff)
            {
                pos = get_image_index(x-1,y,tex_width);
                
                if (img[pos] != 0xff) {
                    img[pos] += 0x28;
                    img[pos+1] += 0x28;
                    img[pos+2] += 0x28;
                }
                
                pos = get_image_index(x+1,y,tex_width);
                
                if (img[pos] != 0xff) {
                    img[pos] += 0x28;
                    img[pos+1] += 0x28;
                    img[pos+2] += 0x28;
                }
            }
            
            pos = get_image_index(x,y,tex_width);
            
            if (y > 1 && y < tex_height-1 && img[pos] == 0xff)
            {
                pos = get_image_index(x,y-1,tex_width);
                
                if (img[pos] != 0xff) {
                    img[pos] += 0x28;
                    img[pos+1] += 0x28;
                    img[pos+2] += 0x28;
                }
                
                pos = get_image_index(x,y+1,tex_width);
                
                if (img[pos] != 0xff) {
                    img[pos] += 0x28;
                    img[pos+1] += 0x28;
                    img[pos+2] += 0x28;
                }
            }
        }
    }   
}

void perform_raytrace(std::string smd_in, std::string png_out, int tex_width, int tex_height, float sun_pitch, float sun_yaw)
{
    std::vector<unsigned char> img;
    img.resize(tex_width * tex_height * 4);
    
    //Clear the image to a gray color.
    for (unsigned int y = 0; y < tex_height; y++)
    {
        for (unsigned int x = 0; x < tex_width; x++)
        {
            img[4*tex_width*y + 4*x + 0] = 0x5f;
            img[4*tex_width*y + 4*x + 1] = 0x5f;
            img[4*tex_width*y + 4*x + 2] = 0x5f;
            img[4*tex_width*y + 4*x + 3] = 0xff;
        }
    }
    
    smd_model_reader* smr = new smd_model_reader(smd_in);
    
    const float radianizer = PI / 180;
    float sun_dist = smr->get_max_coordinate() + 1.0f; //Add one so that sun rays are a little farther than geometry.
    float pitch = sun_pitch - 90.0f;
    float inv_pitch = sun_pitch + 90.0f;
    float yaw = sun_yaw + 180.0f;
    float closest_tri = -1.0f;
    
    //Sun direction.
    float sdx = cos(yaw*radianizer) * cos(pitch*radianizer);
    float sdy = sin(yaw*radianizer) * cos(pitch*radianizer);
    float sdz = sin(pitch*radianizer);
    
    //Sun position.
    float spx = cos(yaw*radianizer) * cos(inv_pitch*radianizer) * sun_dist;
    float spy = sin(yaw*radianizer) * cos(inv_pitch*radianizer) * sun_dist;
    float spz = sin(inv_pitch*radianizer) * sun_dist;
    
    vertex* hit = new vertex();
    vertex* sun = new vertex(spx, spy, spz, sdx, sdy, sdz, 0.0f, 0.0f);
    long long start_raytracing = start_timer();
    
    int prev_progress = 0;
    int progress = 0;
    
    unsigned int tex_x = 0;
    unsigned int tex_y = 0;
    unsigned int tex_pos = 0;
    
    //Create a grid of rays.
    for (int py = 0; py < resolution; py++) 
    {
        for (int px = 0; px < resolution; px++)
        {
            //Calculate location of the ray's spawn position within the grid.
            float ox = 0.0f;
            float oy = (float)(px-(resolution/2))/(resolution/8)*spread;
            float oz = (float)(py-(resolution/2))/(resolution/8)*spread;
            point_trans_rot_y(-pitch,&ox,&oy,&oz);
            point_trans_rot_z(yaw,&ox,&oy,&oz);
            (*sun).x = spx + ox;
            (*sun).y = spy + oy;
            (*sun).z = spz + oz;
            
            //Check each triangle against a ray to determine which triangle gets illuminated.
            for (unsigned int i = 0; i < smr->get_triangle_count(); i++)
            {
                float dist;
                bool rayhit = raytracer::get_intersection(sun, smr->get_triangle(i), hit, &dist);
            
                if (rayhit)
                {
                    //If we found a closer triangle or we haven't found one yet...
                    if (dist < closest_tri || closest_tri < 0)
                    {
                        raytracer::transform_trace_to_uv(smr->get_triangle(i), hit);
                        closest_tri = dist;
                    }
                }
            }
            
            if (closest_tri > 0) //Evaluates to false if we didn't hit anything.
            {
                tex_x = (unsigned int)floor(hit->u*tex_width);
                tex_y = (unsigned int)floor(tex_height-hit->v*tex_height);
                tex_pos = 4*tex_width*tex_y + 4*tex_x;
                
                img[tex_pos + 0] = 0xff;
                img[tex_pos + 1] = 0xff;
                img[tex_pos + 2] = 0xff;
                img[tex_pos + 3] = 0xff;
            }
            
            closest_tri = -1.0f; //Reset for the next ray.
        }
        
        if (prev_progress != progress)
        {
            std::cout << "Progress: " << prev_progress << "%\r";
            progress = prev_progress;
        }
        
        prev_progress = (floor(((float)py / (float)resolution) * 20.0f) / 20.0f) * 100.0f;
    }
    
    apply_aliasing(img, tex_width, tex_height);
    
    stop_timer(start_raytracing, "Ray-simulation time");
    
    unsigned int error = lodepng::encode(png_out.c_str(), img, tex_width, tex_height); //Write the image file.
    
    if (error)
        std::cout << "Could not write " << png_out << " to file!";

    delete smr;
    delete sun;
    delete hit;
}

