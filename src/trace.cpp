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

int main() {
    // image
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 600;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 50;
    const int max_depth = 10;
    
    const int rays_per_line = image_width * samples_per_pixel;

    std::cerr << "Rendering " << image_width << " by " << image_height << " pixels with " << samples_per_pixel << " samples per pixel\n";
    std::cerr << rays_per_line * image_height << " rays to cast\n";

    // world
    hittable_list world;
    init_world(world, 2);

    // camera
    camera cam(90, aspect_ratio);

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
