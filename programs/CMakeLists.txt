cmake_minimum_required(VERSION 3.2)
project(raytracer)

set(SOURCE_FILES main.cc color.h vec3.h ray.h hittable.h sphere.h sphere.cc)

add_executable(raytracer ${SOURCE_FILES})
