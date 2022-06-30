#include "hittable_list.h"

bool hittable_list::hit(const ray& r, double tmin, double tmax, hit_record& record) const
{
	hit_record tmp_record;
	bool hit_anything = false;
	auto closest_so_far = tmax;

	for(const auto& obj : objects)
	{
		if (obj->hit(r, tmin, closest_so_far, tmp_record))
		{
			hit_anything = true;
			closest_so_far = tmp_record.t;
			record = tmp_record;
		}
	}
	
	return hit_anything;
}
