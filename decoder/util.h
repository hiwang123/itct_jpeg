
#define _DC  0
#define _AC  1

#define pair(a,b) ((a<<16)|b)
#define max(a,b) (a>b ? a:b)
#define min(a,b) (a<b ? a:b)
#define clip(a) max(min(a,255),0)

struct Bits{
	int cnt;
	uint8_t now;
};

struct App0{
	uint16_t ver;
	uint8_t dens_unit;
	uint16_t dens_x, dens_y;
};

struct Dqt{
	uint8_t q_table_8[2][64];
	uint16_t q_table_16[2][64];
};

struct CompSof0{
	uint8_t H, V, Tq;
};

struct Sof0{
	uint8_t P;
	uint16_t Y, X;
	uint8_t Hmax, Vmax;
	uint8_t Nf;
	CompSof0 comp[4];
};

struct Dht{
	uint16_t huffman_table[1114120];
};

struct CompSos{
	uint8_t Td, Ta;
};

struct Sos{
	uint8_t Ns;
	CompSos comp[4];
};

struct Mcu{
	int pred[4];  //predictor, DC(n-1)
};

struct Pixel{
	int YCrCb[4];
};

struct Bmp{
	int V, H;
	Pixel **p;
};

extern App0 app0;
extern Dqt dqt;
extern Sof0 sof0;
extern Dht dht[4];
extern Sos sos;
extern Mcu mcu;
extern Bits bits;
extern FILE* fp;
extern Bmp bmp;
extern double c[8]; //cosine

void init(char *filename);
void init_bmp(int H,int W);
void read_uint8_t(uint8_t* ptr);
void read_uint16_t(uint16_t* ptr);
void read_uints(uint8_t **ptr, int size);
uint8_t get_next_bit(void);
void fill_pixel(int st_v, int st_h, int scale_v, int scale_h, int chnl, int val);
void output_bmp(char *filename);
