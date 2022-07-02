cmake_minimum_required(VERSION 3.2)
project(raytracer)

set(SOURCE_FILES main.cc color.h vec3.h ray.h hittable.h sphere.h sphere.cc hittable_list.h hittable_list.cc raytracer.h camera.h)

add_executable(raytracer ${SOURCE_FILES})
