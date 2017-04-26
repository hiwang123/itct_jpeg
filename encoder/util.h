
#define _DC  0
#define _AC  1

#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)
#define abs(a) (a<0?-a:a)
#define clip(a) max(min(a,255),0)

struct Pixel{
	uint8_t YCrCb[4];
};
struct Bmp{
	uint32_t V, H;
	Pixel **p;
};
struct Comp{
	int Tq, Td, Ta;
	int H, V;
};
struct Bits{
	int cnt;
	uint8_t now;
};
struct Mcu{
	int pred[4];  //predictor, DC(n-1)
};

extern Bits bits;
extern FILE *fs, *fd;
extern Bmp bmp;
extern Comp comp[4];
extern Mcu mcu;

void init(char *src, char *dst);
void write_uint16_t(uint16_t num);
void write_uint8_t(uint8_t num);
void write_uints(uint8_t *ptr, int len);
void write_next_bit(uint8_t b);
void padding_bits(void);
