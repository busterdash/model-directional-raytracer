#include <iostream>
#include <cmath>
#include <cstring>
#include <string>
#include <iomanip>
#include <sys/time.h>
#include <pthread.h>
#include "raytracer.hpp"
#include "windows_bitmap.hpp"
#include "smd_model_reader.hpp"
#define NUM_THREADS 4

float const PI = 3.141593f;
int resolution = 512;
float spread = 1.0f;
int img_width = 512;
int img_height = 512;

void point_trans_rot_z(float angle, float* x, float* y, float* z);
void point_trans_rot_y(float angle, float* x, float* y, float* z);
void perform_raytrace(std::string smd_in, std::string bmp_out, float sun_pitch, float sun_yaw);

int main(int argc, char** argv)
{
	std::string inpath;
	std::string outpath;
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
	
	perform_raytrace(inpath,outpath,pitch,yaw);
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

struct worker_arguments
{
	vertex* sun_parent;
	smd_model_reader* smr;
	raster_image* img;
	int x_start;
        int y_start;
        int x_end;
        int y_end;
        float sun_dist;
        float pitch;
        float yaw;
        float spz;
        float spx;
        float spy;

	worker_arguments(smd_model_reader* smrx, raster_image* imgx, int thread_index)
	{
		smr = smrx;
		img = imgx;
		x_start = 0;
		y_start = thread_index*resolution/NUM_THREADS;
		x_end = resolution;
		y_end = (thread_index+1)*resolution/NUM_THREADS;
	}
	
	void sun_setup(vertex* sun_parentx, float sun_distx, float pitchx, float yawx, float spzx, float spxx, float spyx)
	{
		sun_parent = sun_parentx;
		sun_dist = sun_distx;
		pitch = pitchx;
		yaw = yawx;
		spz = spzx;
		spx = spxx;
		spy = spyx;
	}
};

void* raytrace_worker(void* arguments)
{
	worker_arguments* args = (worker_arguments*)arguments;
	vertex* hit = new vertex();
	vertex* sun = new vertex(args->sun_parent->x,
				 args->sun_parent->y,
				 args->sun_parent->z,
				 args->sun_parent->nx,
				 args->sun_parent->ny,
				 args->sun_parent->nz,
				 args->sun_parent->u,
				 args->sun_parent->v);

	float closest_tri = -1;
	
	for (int py = args->y_start; py < args->y_end; py++)
        {
                for (int px = args->x_start; px < args->x_end; px++) //Create several rays.
                {
			float ox, oy, oz;
                        ox = 0.0f; 
			oy = (float)(px-(resolution/2))/(resolution/8)*spread;
			oz = (float)(py-(resolution/2))/(resolution/8)*spread;
                        point_trans_rot_y(-args->pitch,&ox,&oy,&oz);
                        point_trans_rot_z(args->yaw,&ox,&oy,&oz);
                        (*sun).x = args->spx + ox;
                        (*sun).y = args->spy + oy;
                        (*sun).z = args->spz + oz;

                        for (unsigned int i = 0; i < args->smr->get_triangle_count(); i++) //Check each triangle against ray.
                        {
                                float dist;
                                bool rayhit = raytracer::get_intersection(sun, args->smr->get_triangle(i), hit, &dist);

                                if (rayhit)
                                {
                                        if (dist < closest_tri || closest_tri < 0)
                                        {
                                                raytracer::transform_trace_to_uv(args->smr->get_triangle(i), hit);
                                                closest_tri = dist;
                                        }
                                }
                        }

                        if (closest_tri > 0) //Evaluates to false if we didn't hit anything.
                        {
                                args->img->set_pixel((unsigned int)floor(hit->u*img_width), (unsigned int)floor(img_height-hit->v*img_height), 0xffffff);
                        }

                        closest_tri = -1.0f;
                }
        }

	delete hit;
	delete sun;
}

void perform_raytrace(std::string smd_in, std::string bmp_out, float sun_pitch, float sun_yaw)
{
	windows_bitmap* wb = new windows_bitmap(bmp_out,img_width,img_height);
	smd_model_reader* smr = new smd_model_reader(smd_in);
	
	const float radianizer = PI/180;
	float sun_dist = smr->get_max_coordinate() + 1.0f; //Add one so that sun rays are a little farther than geometry.
	float pitch = sun_pitch - 90.0f;
	float inv_pitch = sun_pitch + 90.0f;
	float yaw = sun_yaw + 180.0f;
	float sdx = cos(yaw*radianizer) * cos(pitch*radianizer); //Sun direction.
	float sdy = sin(yaw*radianizer) * cos(pitch*radianizer);
	float sdz = sin(pitch*radianizer);
	float spz = sin(inv_pitch*radianizer) * sun_dist;
	float spx = cos(yaw*radianizer) * cos(inv_pitch*radianizer) * sun_dist;
	float spy = sin(yaw*radianizer) * cos(inv_pitch*radianizer) * sun_dist;
	vertex* hit = new vertex();
	vertex* sun = new vertex(spx,spy,spz,sdx,sdy,sdz,0.0f,0.0f);
	float closest_tri = -1.0f;

	worker_arguments* wa[NUM_THREADS];
	pthread_t threads[NUM_THREADS];
	int t;

	//call sunsetup for the above
	//pull in hit and sun because shared not individual

	for(t = 0; t < NUM_THREADS; t++) 
	{
		//wb function for this
		wa[t] = new worker_arguments(smr, wb->get_dib()->get_image(), t);
		wa[t]->sun_setup(sun, sun_dist, pitch, yaw, spz, spx, spy);
	}
	for (t = 0; t < NUM_THREADS; t++)
        {
                pthread_create(&threads[t],NULL,raytrace_worker,(void*)wa[t]);
        }

        for (t = 0; t < NUM_THREADS; t++)
        {
                pthread_join(threads[t],NULL);
                delete wa[t];
        }

	

	wb->save(); //This stays outside any raytrace operations, or else it won't write the image at all.

	delete wb;
	delete smr;
	delete sun;
	delete hit;
}

