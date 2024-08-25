#ifndef CONTROLLER_TOOLS_H
#define CONTROLLER_TOOLS_H


class BoolTool{

public:
	inline static uint32_t resetFlag(uint32_t &flags, int ind){return flags&=~(1<<ind);}
	inline static uint32_t setFlag(uint32_t &flags, int ind){return flags |= (1<<ind);}
	inline static uint32_t setFlag(uint32_t &flags, int ind, bool val){return val?setFlag(flags,ind):resetFlag(flags,ind);}
	inline static uint32_t getFlag(uint32_t &flags, int ind){ return flags&(1<<ind); }
};

class CharTool{

public:
	static unsigned getNumber(const char *data, const size_t size);
	static char* numToCSTR(unsigned num, uint8_t digits, uint8_t base=10);
};


unsigned CharTool::getNumber(const char *data, const size_t size)
{
	unsigned res = 0;
	const char *end = data+size;
	char c;
	while(data != end){
		c = *(data++);
		if(c<'0' || c>'9') break;
		res *= 10;
		res += c-'0';
	}
	return res;
}

char *CharTool::numToCSTR(unsigned num, const uint8_t digits, const uint8_t base)
{
	static char buf[15];
    const char hex_chars[] = "0123456789ABCDEF";
    uint8_t d = digits;
    char tmp,
        * end = buf,
        * ptr = buf;
        
    do{
        *end = hex_chars[num % base];
        if(d < 2) break;
        d -= 1;
        num /= base;
        end += 1;
    }while(1);

    while (ptr < end) {
        tmp = *ptr;
        *ptr = *end;
        *end = tmp;
        ptr += 1;
        end -= 1;
    }
	buf[digits] = 0;
    return buf;
}












#endif