#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <time.h>  
#include <stdlib.h>
#include <algorithm>
#include "color.h"
#include "vec3.h"
#include "ray.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "rtweekend.h"
#include "camera.h"
#include "material.h"
#include "moving_sphere.h"
#include "aarect.h"
#include "box.h"
#include "constant_medium.h"
#include "bvh.h"

double hit_sphere(const point3& center, double radius, const ray& r) { //old
    vec3 oc = r.origin() - center;
    auto a = r.direction().length_squared();
    auto half_b = dot(oc, r.direction());
    auto c = oc.length_squared() - radius * radius;
    auto discriminant = half_b * half_b - a * c;

    if (discriminant < 0) {
        return -1.0;
    }
    else {
        return (-half_b - sqrt(discriminant)) / a;
    }
}

color ray_color(const ray& r, const color& background, const hittable& world, int depth) {
    hit_record rec;
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return color(0, 0, 0);

    // If the ray hits nothing, return the background color.
    if (!world.hit(r, 0.001, infinity, rec))
        return background;

    //surface normals for task 1 basic scene
    //vec3 N = rec.normal;
    //return 0.5 * color(N.x() + 1, N.y() + 1, N.z() + 1);
    //surface normals for task 1 basic scene

    ray scattered;
    color attenuation;
    color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);

    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        return emitted;

    return emitted + attenuation * ray_color(scattered, background, world, depth - 1);
}

hittable_list random_scene() {
    hittable_list world;

    //auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    //world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));
    //texture checker
    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(checker)));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    auto center2 = center + vec3(0, random_double(0, .5), 0);
                    world.add(make_shared<moving_sphere>(
                        center, center2, 0.0, 1.0, 0.2, sphere_material));
                }
                else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
                else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}

hittable_list two_spheres() {
    hittable_list objects;

    auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));

    objects.add(make_shared<sphere>(point3(0, -10, 0), 10, make_shared<lambertian>(checker)));
    objects.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    return objects;
}

hittable_list two_perlin_spheres() {
    hittable_list objects;

    auto pertext = make_shared<noise_texture>(4);
    objects.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
    objects.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

    return objects;
}

hittable_list earth() {
    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0, 0, 0), 2, earth_surface);

    return hittable_list(globe);
}

hittable_list simple_light() {
    hittable_list objects;

    auto pertext = make_shared<noise_texture>(4);
    objects.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
    objects.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

    auto difflight = make_shared<diffuse_light>(color(4, 4, 4));
    objects.add(make_shared<xy_rect>(3, 5, 1, 3, -2, difflight));

    return objects;
}

hittable_list cornell_box() {
    hittable_list objects;

    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    objects.add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
    objects.add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));
    objects.add(make_shared<xz_rect>(213, 343, 227, 332, 554, light));
    objects.add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
    objects.add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
    objects.add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));

    //additional 2 boxes
    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265, 0, 295));
    objects.add(box1);

    shared_ptr<hittable> box2 = make_shared<box>(point3(0, 0, 0), point3(165, 165, 165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130, 0, 65));
    objects.add(box2);

    return objects;
}

hittable_list cornell_smoke() {
    hittable_list objects;

    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(7, 7, 7));

    objects.add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
    objects.add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));
    objects.add(make_shared<xz_rect>(113, 443, 127, 432, 554, light));
    objects.add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
    objects.add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
    objects.add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));

    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265, 0, 295));

    shared_ptr<hittable> box2 = make_shared<box>(point3(0, 0, 0), point3(165, 165, 165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130, 0, 65));

    objects.add(make_shared<constant_medium>(box1, 0.01, color(0, 0, 0)));
    objects.add(make_shared<constant_medium>(box2, 0.01, color(1, 1, 1)));

    return objects;
}

hittable_list final_scene() {
    hittable_list boxes1;
    auto ground = make_shared<lambertian>(color(0.48, 0.83, 0.53));

    const int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            auto w = 100.0;
            auto x0 = -1000.0 + i * w;
            auto z0 = -1000.0 + j * w;
            auto y0 = 0.0;
            auto x1 = x0 + w;
            auto y1 = random_double(1, 101);
            auto z1 = z0 + w;

            boxes1.add(make_shared<box>(point3(x0, y0, z0), point3(x1, y1, z1), ground));
        }
    }

    hittable_list objects;

    objects.add(make_shared<bvh_node>(boxes1, 0, 1));

    auto light = make_shared<diffuse_light>(color(7, 7, 7));
    objects.add(make_shared<xz_rect>(123, 423, 147, 412, 554, light));

    auto center1 = point3(400, 400, 200);
    auto center2 = center1 + vec3(30, 0, 0);
    auto moving_sphere_material = make_shared<lambertian>(color(0.7, 0.3, 0.1));
    objects.add(make_shared<moving_sphere>(center1, center2, 0, 1, 50, moving_sphere_material));

    objects.add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
    objects.add(make_shared<sphere>(
        point3(0, 150, 145), 50, make_shared<metal>(color(0.8, 0.8, 0.9), 1.0)
        ));

    auto boundary = make_shared<sphere>(point3(360, 150, 145), 70, make_shared<dielectric>(1.5));
    objects.add(boundary);
    objects.add(make_shared<constant_medium>(boundary, 0.2, color(0.2, 0.4, 0.9)));
    boundary = make_shared<sphere>(point3(0, 0, 0), 5000, make_shared<dielectric>(1.5));
    objects.add(make_shared<constant_medium>(boundary, .0001, color(1, 1, 1)));

    auto emat = make_shared<lambertian>(make_shared<image_texture>("earthmap.jpg"));
    objects.add(make_shared<sphere>(point3(400, 200, 400), 100, emat));
    auto pertext = make_shared<noise_texture>(0.1);
    objects.add(make_shared<sphere>(point3(220, 280, 300), 80, make_shared<lambertian>(pertext)));

    hittable_list boxes2;
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    int ns = 1000;
    for (int j = 0; j < ns; j++) {
        boxes2.add(make_shared<sphere>(point3::random(0, 165), 10, white));
    }

    objects.add(make_shared<translate>(
        make_shared<rotate_y>(
            make_shared<bvh_node>(boxes2, 0.0, 1.0), 15),
        vec3(-100, 270, 395)
        )
    );

    return objects;
}

hittable_list task1_basic_scene()
{
    hittable_list objects;
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_1 = make_shared<lambertian>(color(0.1, 0.2, 0.5)); //blue
    auto material_2 = make_shared<lambertian>(color(1, 0.7, 1)); //pink
    auto material_3 = make_shared<lambertian>(color(1, 0.5, 0)); //orange
    auto material_4 = make_shared<lambertian>(color(0.9, 0, 0.2)); //redish
    auto material_metal = make_shared<metal>(color(0.4, 0.4, 0.4), 0.0); //metal

    objects.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    objects.add(make_shared<sphere>(point3(-3.0, 1.0, -10.0), 2, material_1));
    objects.add(make_shared<sphere>(point3(-1.5, 0.0, -4.5), 0.5, material_2));
    objects.add(make_shared<sphere>(point3( 0.0, 0.5, -5.0), 1, material_metal));
    objects.add(make_shared<sphere>(point3(1, -0.25, -4.0), 0.3, material_4));
    objects.add(make_shared<sphere>(point3(0.0, 2, -5.0), 0.5, material_3));
    return objects;
}

hittable_list task3_more_shapes()
{
    hittable_list objects;
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_1 = make_shared<lambertian>(color(0.1, 0.2, 0.5)); //blue

    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(2,2,2), material_1);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(-1, -1, -10));
    objects.add(box1);

    //objects.add(make_shared<xy_rect>(-1, 1, -1, 1, -6, material_1));

    objects.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    return objects;
}

hittable_list task4_diffuse_and_metals()
{
    hittable_list objects;
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_1 = make_shared<lambertian>(color(0.1, 0.2, 0.5)); //blue
    auto material_2 = make_shared<lambertian>(color(1, 0.7, 1)); //pink
    auto material_3 = make_shared<lambertian>(color(1, 0.5, 0)); //orange
    auto material_4 = make_shared<lambertian>(color(0.9, 0, 0.2)); //redish
    auto material_metal = make_shared<metal>(color(0.4, 0.4, 0.4), 0.0); //metal
    auto material_gold = make_shared<metal>(color(0.83, 0.68, 0.21), 0.0); //gold

    objects.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    objects.add(make_shared<sphere>(point3(3.0, 1.0, -10.0), 2, material_gold));
    objects.add(make_shared<sphere>(point3(-1.5, 0.0, -4.5), 0.5, material_2));
    objects.add(make_shared<sphere>(point3(0.0, 0.5, -5.0), 1, material_metal));
    objects.add(make_shared<sphere>(point3(1, -0.25, -4.0), 0.3, material_4));
    objects.add(make_shared<sphere>(point3(0.0, 2, -5.0), 0.5, material_3));
    
    
    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(2, 2, 2), material_1);
    box1 = make_shared<rotate_y>(box1, 45);
    box1 = make_shared<translate>(box1, vec3(-5, -0.75, -8));
    objects.add(box1);

    
    return objects;
}

hittable_list task5_refraction()
{
    hittable_list objects;
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_1 = make_shared<lambertian>(color(0.1, 0.2, 0.5)); //blue
    auto material_2 = make_shared<lambertian>(color(1, 0.7, 1)); //pink
    auto material_3 = make_shared<lambertian>(color(1, 0.5, 0)); //orange
    auto material_4 = make_shared<lambertian>(color(0.9, 0, 0.2)); //redish
    auto material_metal = make_shared<metal>(color(0.4, 0.4, 0.4), 0.0); //metal
    auto material_gold = make_shared<metal>(color(0.83, 0.68, 0.21), 0.0); //gold

    auto material_glass = make_shared<dielectric>(1.52);
    auto material_diamond = make_shared<dielectric>(2.418);


    objects.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    objects.add(make_shared<sphere>(point3(3.0, 1.0, -10.0), 2, material_gold));
    
    objects.add(make_shared<sphere>(point3(0.0, 0.5, -5.0), 1, material_metal));
    objects.add(make_shared<sphere>(point3(0.0, 2, -5.0), 0.5, material_3));

    objects.add(make_shared<sphere>(point3(-1.5, 0.0, -4.5), 0.5, material_glass));
    objects.add(make_shared<sphere>(point3(1.1, -0.25, -3.5), 0.4, material_glass));
    objects.add(make_shared<sphere>(point3(2.5, 0.37, -5.0), -0.75, material_glass));


    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(2, 2, 2), material_1);
    box1 = make_shared<rotate_y>(box1, 45);
    box1 = make_shared<translate>(box1, vec3(-5, -0.75, -8));
    objects.add(box1);


    return objects;
}

hittable_list task6_lights()
{
    hittable_list objects;
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_1 = make_shared<lambertian>(color(0.1, 0.2, 0.5)); //blue
    auto material_2 = make_shared<lambertian>(color(1, 0.7, 1)); //pink
    auto material_3 = make_shared<lambertian>(color(1, 0.5, 0)); //orange
    auto material_4 = make_shared<lambertian>(color(0.9, 0, 0.2)); //redish
    auto material_metal = make_shared<metal>(color(0.4, 0.4, 0.4), 0.0); //metal
    auto material_gold = make_shared<metal>(color(0.83, 0.68, 0.21), 0.0); //gold
    auto material_glass = make_shared<dielectric>(1.52); //glass

    objects.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    objects.add(make_shared<sphere>(point3(3.0, 1.0, -10.0), 2, material_gold));
    //objects.add(make_shared<sphere>(point3(-1.5, 0.0, -4.5), 0.5, material_2));

    objects.add(make_shared<sphere>(point3(0.0, 0.5, -5.0), 1, material_metal));

    //objects.add(make_shared<sphere>(point3(1, -0.25, -4.0), 0.3, material_4));
    objects.add(make_shared<sphere>(point3(0.0, 2, -5.0), 0.5, material_3));

    objects.add(make_shared<sphere>(point3(2.5, 0.25, -5.0), 0.75, material_glass));

    auto difflight = make_shared<diffuse_light>(color(1, 1, 1));
    auto difflightpink = make_shared<diffuse_light>(color(1, 0.75, 1));

    objects.add(make_shared<sphere>(point3(-1.5, 0.0, -4.5), 0.5, difflight));
    objects.add(make_shared<sphere>(point3(1, -0.25, -4.0), 0.3, difflightpink));

    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(2, 2, 2), material_1);
    box1 = make_shared<rotate_y>(box1, 45);
    box1 = make_shared<translate>(box1, vec3(-5, -0.75, -8));
    objects.add(box1);


    return objects;
}

hittable_list task7_get_creative() {
    hittable_list objects;
    int pyramid_base_length =10;
    double sphere_size = 1;
    auto checker = make_shared<checker_texture>(color(1, 0.2, 0.2), color(0.9, 0.9, 0.9));
    objects.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(checker)));

    //box
    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    objects.add(make_shared<xz_rect>(-10, 30, -30, 10, 20, white)); //top
    objects.add(make_shared<xz_rect>(-10, 30, -30, 10, 0, white)); //bottom
    
    objects.add(make_shared<xz_rect>(0, 10, -10, 0, 19.8, light)); //light on ceiling

    objects.add(make_shared<xy_rect>(-10, 30, 0, 20, -30, red)); //left wall
    objects.add(make_shared<yz_rect>(0, 20, -30, 10, 30, green)); //right wall

    for (int ix = 0; ix < pyramid_base_length; ix++)
    {
        for (int iz = 0; iz < pyramid_base_length; iz++)
        {
            for (int iy = 0; iy < pyramid_base_length-iz-ix; iy++)
            {

                auto choose_mat = random_double();

                shared_ptr<material> sphere_material;

                if (choose_mat < 0.25) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    objects.add(make_shared<sphere>(point3(0 + (2 * ix) + iy, sphere_size + iy * sqrt(2), 0 - (2 * iz) - iy), 1, sphere_material));
                }
                else if (choose_mat < 0.65) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    objects.add(make_shared<sphere>(point3(0 + (2 * ix) + iy, sphere_size + iy * sqrt(2), 0 - (2 * iz) - iy), 1, sphere_material));
                }
                
                else if (choose_mat < 0.75) //light
                {
                    sphere_material = make_shared<diffuse_light>(vec3((0.1 * (rand() % 10 + 1)), (0.1 * (rand() % 10 + 1)), (0.1 * (rand() % 100 + 1))));
                    objects.add(make_shared<sphere>(point3(0 + (2 * ix) + iy, sphere_size + iy * sqrt(2), 0 - (2 * iz) - iy), 1, sphere_material));
                }

                else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    objects.add(make_shared<sphere>(point3(0 + (2 * ix) + iy, sphere_size + iy * sqrt(2), 0 - (2 * iz) - iy), 1, sphere_material));
                }
            }
            
        }

    }
    return objects;
    }

int main(int argc, char const* argv[])
{
    // Image
    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 600;
    int samples_per_pixel = 10; //for antialising
    int max_depth = 50;  // for shadows

    hittable_list world;
    point3 lookfrom;
    point3 lookat;
    auto vfov = 40.0;
    auto aperture = 0.0;
    //ambient light - or dark
    color background(0, 0, 0);
    auto dist_to_focus = 10.0;

    switch (0) {
    case 1:
        world = random_scene();
        background = color(0.70, 0.80, 1.00);
        lookfrom = point3(13, 2, 3);
        lookat = point3(0, 0, 0);
        vfov = 20.0;
        aperture = 0.1;
        break;

    case 2:
        world = two_spheres();
        background = color(0.70, 0.80, 1.00);
        lookfrom = point3(13, 2, 3);
        lookat = point3(0, 0, 0);
        vfov = 20.0;
        break;
    case 3:
        world = two_perlin_spheres();
        background = color(0.70, 0.80, 1.00);
        lookfrom = point3(13, 2, 3);
        lookat = point3(0, 0, 0);
        vfov = 20.0;
        break;
    case 4:
        world = earth();
        background = color(0.70, 0.80, 1.00);
        lookfrom = point3(13, 2, 3);
        lookat = point3(0, 0, 0);
        vfov = 20.0;
        break;
    case 5:
        world = simple_light();
        samples_per_pixel = 400;
        background = color(0, 0, 0);
        lookfrom = point3(26, 3, 6);
        lookat = point3(0, 2, 0);
        vfov = 20.0;
        break;
    case 6:
        world = cornell_box();
        aspect_ratio = 1.0;
        image_width = 600;
        samples_per_pixel = 200;
        background = color(0, 0, 0);
        lookfrom = point3(278, 278, -800);
        lookat = point3(278, 278, 0);
        vfov = 40.0;
        break;
    case 7:
        world = cornell_smoke();
        aspect_ratio = 1.0;
        image_width = 600;
        samples_per_pixel = 200;
        lookfrom = point3(278, 278, -800);
        lookat = point3(278, 278, 0);
        vfov = 40.0;
        break;
    case 8:
        world = final_scene();
        aspect_ratio = 1.0;
        image_width = 800;
        samples_per_pixel = 100;
        background = color(0, 0, 0);
        lookfrom = point3(478, 278, -600);
        lookat = point3(278, 278, 0);
        vfov = 40.0;
        break;
    case 9: //Task 1 [Required]: Basic Scene (15Pt)
        world = task1_basic_scene();
        image_width = 600;
        samples_per_pixel = 10;
        background = color(0.8, 1, 1);
        //angle 1
        lookfrom = point3(0, 2, 1);
        lookat = point3(0, 1.5, -1);
        //angle 2
        //lookfrom = point3(1, 4.5, 1);
        //lookat = point3(0.5, 3.5, -1);
        vfov = 40.0;
        break;
    case 10: //Task 3 [Optional]: More Shapes (5Pt) 
        world = task3_more_shapes();
        image_width = 600;
        samples_per_pixel = 1;
        background = color(0.8, 1, 1);
        //angle 1
        lookfrom = point3(0, 2, 1);
        lookat = point3(0, 1.5, -1);
        //angle 2
        //lookfrom = point3(1, 4.5, 1);
        //lookat = point3(0.5, 3.5, -1);
    case 11: //Task 4 [Required]: Diffuse and Metal Materials (25Pt) 
        world = task4_diffuse_and_metals();
        image_width = 600;
        samples_per_pixel = 500;
        background = color(0.8, 1, 1);
        //angle 1
        lookfrom = point3(0, 2, 1);
        lookat = point3(0, 1.5, -1);
        //angle 2
        //lookfrom = point3(1, 4.5, 1);
        //lookat = point3(0.5, 3.5, -1);
        vfov = 40.0;
        break;
    case 12: //Task 5 [Optional]: Refraction (10Pt)  
        world = task5_refraction();
        image_width = 600;
        samples_per_pixel = 500;
        background = color(0.8, 1, 1);
        //angle 1
        lookfrom = point3(0, 2, 1);
        lookat = point3(0, 1.5, -1);
        //angle 2
        //lookfrom = point3(1, 4.5, 1);
        //lookat = point3(0.5, 3.5, -1);
        vfov = 40.0;
        break;
    case 13: //Task 6 [Required]: Lights (15Pt)  
        world = task6_lights();
        image_width = 600;
        samples_per_pixel = 500;
        //background = color(0.1, 0.1, 0.1);
        background = color(0, 0, 0);
        //angle 1
        lookfrom = point3(0, 2, 1);
        lookat = point3(0, 1.5, -1);
        //angle 2
        //lookfrom = point3(1, 4.5, 1);
        //lookat = point3(0.5, 3.5, -1);
        vfov = 40.0;
        break;
    default:
    case 14: //Task 7 [Optional]: Let’s get creative! (10Pt)  
        world = task7_get_creative();
        int pyramid_base_len = 10;
        aspect_ratio = 16.0/9.0;
        image_width = 1920;
        samples_per_pixel = 500;
        //background = color(0.8, 1, 1);
        background = color(0, 0, 0);
        lookfrom = point3(-25, 15, 25);
        lookat = point3(5,7,-5);
        vfov = 45.0;
        break;
    }


    // Camera
    //camera cam(90.0, aspect_ratio);
    //camera cam(point3(-2, 2, 1), point3(0, 0, -1), vec3(0, 1, 0), 90, aspect_ratio);
    //camera cam(point3(-2,2,1), point3(0,0,-1), vec3(0,1,0), 20, aspect_ratio);

    /*
    * blur
    point3 lookfrom(3, 3, 2);
    point3 lookat(0, 0, -1);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = (lookfrom - lookat).length();
    auto aperture = 2.0;

    camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);
    */


    

    vec3 vup(0, 1, 0);
    
    camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);
    //scene
    auto R = cos(pi / 4);

    //material samples
    /*
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
    auto material_left = make_shared<dielectric>(1.5);
    auto material_right = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

    world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), -0.4, material_left));
    world.add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));
    */


    //setting up file
    int image_height = static_cast<int>(image_width / aspect_ratio);
    std::ofstream imageFile("output.ppm");
    imageFile << "P3\n" << image_width << " " << image_height << "\n255\n";
    
    // main loop & renderer
    for (int j = image_height - 1; j >= 0; --j)
    {
        std::cerr << "\rPicture Progress: " << (int)(((image_height - j) / (float)image_height) * 100) << '%' << std::flush;
        for (int i = 0; i < image_width; ++i)
        {
            color pixel_color(0, 0, 0);
            
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width - 1);
                auto v = (j + random_double()) / (image_height - 1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, background, world, max_depth);
            }
            
            
            // antialiasing grid
            /*
                auto scaling = 1.0 / samples_per_pixel;

                for (double sx = 0; sx <= 1; sx = sx + scaling)
                {
                    for (double sy = 0; sy <= 1; sy = sy + scaling)
                    {
                        auto u = ((i + sx) / (image_width - 1));
                        auto v = ((j + sy) / (image_height - 1));
                        ray r = cam.get_ray(u, v);
                        pixel_color += ray_color(r, background, world, max_depth);

                    }
                }
            
            */

            //for random sample antializing 
            write_color(imageFile, pixel_color, samples_per_pixel);
            //for grid sample antializing
            //write_color(imageFile, pixel_color, pow(samples_per_pixel,2));
        }
    }
    imageFile.close();
    return 0;
}
