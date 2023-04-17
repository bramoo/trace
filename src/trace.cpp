#include "common.h"

#include "camera.h"
#include "colour.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "vec3.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#define DEFAULT_WIDTH 800
#define DEFAULT_SAMPLES 100
#define DEFAULT_DEPTH 50
#define DEFAULT_SCENE 1

colour ray_colour(const ray& r, const hittable& world, int depth) {
    hit_record rec;

    if (depth <= 0) {
	return colour(0, 0, 0);
    }

    if (world.hit(r, 0.001, infinity, rec)) {
	ray scattered;
	colour attenuation;
	if (rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
	    return attenuation * ray_colour(scattered, world, depth - 1);
	}

	return colour(0, 0, 0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5*(unit_direction.y() + 1.0);
    return (1.0-t)*colour(1.0, 1.0, 1.0) + t*colour(0.5, 0.7, 1.0);
}

void init_world(hittable_list& world, int scene_id) {
    switch(scene_id){
	case 1:
	    {
		auto material_ground = std::make_shared<lambertian>(colour(0.8, 0.8, 0.0));
		auto material_center = std::make_shared<lambertian>(colour(0.1, 0.2, 0.5));
		auto material_left   = std::make_shared<dielectric>(1.5);
		auto material_right  = std::make_shared<metal>(colour(0.8, 0.6, 0.2), 0.0);

		world.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
		world.add(make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
		world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
		world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),  -0.4, material_left));
		world.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));
	    }
	    break;
	
	case 2:
	    {
		auto R = cos(pi/4);
		auto material_left = make_shared<lambertian>(colour(0, 0, 1));
		auto material_right = make_shared<lambertian>(colour(1, 0, 0));
		world.add(make_shared<sphere>(point3(-R, 0, -1), R, material_left));
		world.add(make_shared<sphere>(point3( R, 0, -1), R, material_right));
	    }
	    break;
    }
}

int main(int argc, char *argv[]) {
    // args
    int image_width = DEFAULT_WIDTH;
    int samples_per_pixel = DEFAULT_SAMPLES;
    int max_depth = DEFAULT_DEPTH;
    int scene_id = DEFAULT_SCENE;

    switch (argc) {
	case 5:
	    scene_id = std::stoi(argv[4]);
	    if (scene_id == 0) scene_id = DEFAULT_SCENE;
	case 4:
	    max_depth = std::stoi(argv[3]);
	    if (max_depth == 0) max_depth = DEFAULT_DEPTH;
	case 3:
	    samples_per_pixel = std::stoi(argv[2]);
	    if (samples_per_pixel == 0) samples_per_pixel = DEFAULT_SAMPLES;
	case 2:
	    image_width = std::stoi(argv[1]);
	    if (image_width == 0) image_width = DEFAULT_WIDTH;
    }

    // image
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    
    const int rays_per_line = image_width * samples_per_pixel;

    std::cerr << "Rendering " << image_width << " by " << image_height << " pixels with " << samples_per_pixel << " samples per pixel\n";
    std::cerr << rays_per_line * image_height << " rays to cast\n";

    // world
    hittable_list world;
    init_world(world, scene_id);

    // camera
    camera cam(point3(-2, 2, 1), point3(0, 0, -1), vec3(0, 1, 0), 20, aspect_ratio);

    // render
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    std::chrono::time_point<std::chrono::steady_clock> start, end;
    std::chrono::duration<double> diff;
    int rps;
    for (int j = image_height-1; j >= 0; --j) {
	std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
	start = std::chrono::steady_clock::now();
	for (int i = 0; i < image_width; ++i) {
	    colour pixel_colour(0, 0, 0);
	    for (int s = 0; s < samples_per_pixel; ++s) {
		auto u = double(i + random_double()) / (image_width-1);
		auto v = double(j + random_double()) / (image_height-1);
		ray r = cam.get_ray(u, v);
		pixel_colour += ray_colour(r, world, max_depth);
	    }
	    write_colour(std::cout, pixel_colour, samples_per_pixel);
	}
	end = std::chrono::steady_clock::now();
	diff = end - start;
	rps = rays_per_line / 1000.0 / diff.count();
	std::cerr << " [" << rps << " krps]" << ' ' << std::flush;
    }

    std::cerr << "\nDone.\n";
}
