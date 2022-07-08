#ifndef _H_HITTABLE_LIST_
#define _H_HITTABLE_LIST_

#include <vector>
#include <memory>
#include "hittable.h"
#include "floattype.h"

using std::shared_ptr;

class hittable_list : public hittable
{
public:

	hittable_list() {}

	hittable_list(shared_ptr<hittable> object)
	{
		add(object);
	}

	virtual ~hittable_list()
	{
		clear();
	}

	void add(shared_ptr<hittable> object)
	{
		objects.push_back(object);
	}

	void clear()
	{
		objects.clear();
	}

	virtual bool hit(const ray& r, FLOAT_TYPE tmin, FLOAT_TYPE tmax, hit_record& record) const override;


public:
	std::vector<shared_ptr<hittable>> objects;

};

#endif //_H_HITTABLE_LIST_