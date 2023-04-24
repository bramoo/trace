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
#include <vector>

#define DEFAULT_WIDTH 800
#define DEFAULT_SAMPLES 100
#define DEFAULT_DEPTH 50
#define TILESIZE 32

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

hittable_list three_balls() {
    hittable_list world;
    auto material_ground = std::make_shared<lambertian>(colour(0.8, 0.8, 0.0));
    auto material_center = std::make_shared<lambertian>(colour(0.1, 0.2, 0.5));
    auto material_left   = std::make_shared<dielectric>(1.5);
    auto material_right  = std::make_shared<metal>(colour(0.8, 0.6, 0.2), 0.0);

    world.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),  -0.4, material_left));
    world.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));

    return world;
}

hittable_list two_balls() {
    hittable_list world;

    auto R = cos(pi/4);
    auto material_left = make_shared<lambertian>(colour(0, 0, 1));
    auto material_right = make_shared<lambertian>(colour(1, 0, 0));

    world.add(make_shared<sphere>(point3(-R, 0, -1), R, material_left));
    world.add(make_shared<sphere>(point3( R, 0, -1), R, material_right));

    return world;
}

hittable_list random_balls() {
    hittable_list world;

    auto ground_mat = make_shared<lambertian>(colour(0.2, 0.6, 0.7));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_mat));
    
    for (int a = -11; a < 11; a++) {
	for (int b = -11; b < 11; b++) {
	    auto choose_mat = random_double();
	    point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

	    if ((center - point3(4, 0.2, 0)).length() > 0.9) {
		shared_ptr<material> sphere_mat;

		if (choose_mat < 0.8) {
		    // diffuse
		    auto albedo = colour::random() * colour::random();
		    sphere_mat = make_shared<lambertian>(albedo);
		    world.add(make_shared<sphere>(center, 0.2, sphere_mat));
		} else if (choose_mat < 0.95) {
		    // metal
		    auto albedo = colour::random(0.5, 1);
		    auto fuzz = random_double(0, 0.5);
		    sphere_mat = make_shared<metal>(albedo, fuzz);
		    world.add(make_shared<sphere>(center, 0.2, sphere_mat));
		} else {
		    // glass
		    sphere_mat = make_shared<dielectric>(1.5);
		    world.add(make_shared<sphere>(center, 0.2, sphere_mat));
		}
	    }
	}
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));
    
    auto material2 = make_shared<lambertian>(colour(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(colour(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}

void renderImage(std::vector<colour>& image, const camera& cam, const hittable_list& world, int image_width, int image_height, int samples_per_pixel, int max_depth) {
    // target tile size
    const int tiles_x = image_width / TILESIZE;
    const int tiles_y = image_height / TILESIZE;
    const int tile_count = tiles_x * tiles_y;

    // stretched tile size
    const double tsize_x = double(image_width) / tiles_x;
    const double tsize_y = double(image_height) / tiles_y;

    std::cerr << "Actual tile size: " << tsize_x << " by " << tsize_y << '\n';

    std::chrono::time_point<std::chrono::steady_clock> tile_start, tile_end;
    std::chrono::duration<double> diff;
    int ray_count;
    int rps;
    // for each tile
    for (int tile = 0; tile < tile_count; tile++) {
	// tile offsets
	const int tx = tile % tiles_x;
	const int ty = tile / tiles_x;
	const int start_x = static_cast<int>(tsize_x * tx);
	const int start_y = static_cast<int>(tsize_y * ty);
	const int end_x = static_cast<int>(tsize_x * (tx + 1));
	const int end_y = static_cast<int>(tsize_y * (ty + 1));

	// timing tile start
	std::cerr << "\rTiles remaining: " << tile_count - tile << std::flush;
	ray_count = 0;
	tile_start = std::chrono::steady_clock::now();

	// for each pixel in tile
	for (int y = start_y; y < end_y; y++) {
	for (int x = start_x; x < end_x; x++) {
	    colour pixel_colour(0, 0, 0);
	    for (int s = 0; s < samples_per_pixel; s++) {
		auto u = double(x + random_double()) / (image_width - 1);
		auto v = 1.0 - double(y + random_double()) / (image_height - 1);
		ray r = cam.get_ray(u, v);
		pixel_colour += ray_colour(r, world, max_depth);
		ray_count++;
	    }
	    image[image_width*y+x] = pixel_colour / samples_per_pixel;
	}
	}

	// timing tile end
	tile_end = std::chrono::steady_clock::now();
	diff = tile_end - tile_start;
	rps = ray_count / 1000.0 / diff.count();
	std::cerr << " [" << rps << " krps]" << ' ' << std::flush;
    }
}

int main(int argc, char *argv[]) {
    // args
    int image_width = DEFAULT_WIDTH;
    int samples_per_pixel = DEFAULT_SAMPLES;
    int max_depth = DEFAULT_DEPTH;

    switch (argc) {
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
    //const auto aspect_ratio = 16.0 / 9.0;
    const auto aspect_ratio = 3.0 / 2.0;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    
    const int rays_per_line = image_width * samples_per_pixel;
    const int total_rays = rays_per_line * image_height;

    std::cerr << "Rendering " << image_width << " by " << image_height << " pixels with " << samples_per_pixel << " samples per pixel\n";
    std::cerr << total_rays << " rays to cast\n";

    // world
    hittable_list world = three_balls();

    // camera
    // point3 lookfrom(13, 2, 3);
    point3 lookfrom(3, 1, 5);
    point3 lookat(0, 0, -1);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 7.0;
    auto aperture = 0.1;

    camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);

    // render
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
    const int pixel_count = image_height * image_width;
    std::vector<colour> image(pixel_count);

    auto start = std::chrono::steady_clock::now();
    renderImage(image, cam, world, image_width, image_height, samples_per_pixel, max_depth);
    auto end = std::chrono::steady_clock::now();

    std::cerr << "\nDone.\n";
    std::chrono::duration<double> diff = end - start;
    int krps = total_rays / 1000.0 / diff.count();
    std::cerr << diff.count() << " seconds [" << krps << " krps]\n";

    for (colour c : image) {
	std::cout << static_cast<int>(256 * clamp(c.x(), 0.0, 0.999)) << ' '
		  << static_cast<int>(256 * clamp(c.y(), 0.0, 0.999)) << ' '
		  << static_cast<int>(256 * clamp(c.z(), 0.0, 0.999)) << '\n';
    }
}
