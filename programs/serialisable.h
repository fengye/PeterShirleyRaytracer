#ifndef _H_SERIALISABLE_
#define _H_SERIALISABLE_

class serialisable
{
public:
	virtual void serialise(void* buf, uint8_t* out_size) const = 0;
	virtual void deserialise(const void* buf, uint8_t size) = 0;
	
};

#endif //_H_SERIALISABLE_