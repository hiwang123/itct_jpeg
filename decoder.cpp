#include "libs.h"

int extend(uint8_t len, uint16_t value){
	if(value & (1<<(len-1))){
		return value;
	}else{
		return (int)value-((1<<len)-1);
	}
}
void read_app0(void){
	uint16_t len;
	uint8_t *JFIF;
	read_uint16_t(&len);
	read_uints(&JFIF, 5);
	assert(strcmp((char *)JFIF, "JFIF")==0);
	read_uint16_t(&app0.ver);
	read_uint8_t(&app0.dens_unit);
	read_uint16_t(&app0.dens_x);
	read_uint16_t(&app0.dens_y);
	fseek(fp, len-14, SEEK_CUR);
}
void read_dqt(void){
	uint16_t Lq;
	uint8_t Tq, Pq;
	read_uint16_t(&Lq);
	Lq-=2;
	while(Lq){
		read_uint8_t(&Tq);
		Pq = Tq>>4;
		Tq &= 0xF;
		Lq-=1;
		if(Pq==0){ //8 bit
			for(int i=0;i<64;i++){
				read_uint8_t(&dqt.q_table_8[Tq][i]);
				Lq-=1;
			}
		}else{ //16 bit
			for(int i=0;i<64;i++){
				read_uint16_t(&dqt.q_table_16[Tq][i]);
				Lq-=1;
			}
		}
	}
}
void read_sof0(void){
	uint16_t len;
	read_uint16_t(&len);
	read_uint8_t(&sof0.P);
	read_uint16_t(&sof0.Y);
	read_uint16_t(&sof0.X);
	read_uint8_t(&sof0.Nf);
	uint8_t hv, c;
	for(int i=1;i<=sof0.Nf;i++){
		read_uint8_t(&c);
		read_uint8_t(&hv);
		sof0.comp[i].H = hv >> 4;
		sof0.comp[i].V = hv & 0xF;
		sof0.Hmax = max(sof0.Hmax, sof0.comp[i].H);
		sof0.Vmax = max(sof0.Vmax, sof0.comp[i].V);
		read_uint8_t(&sof0.comp[i].Tq); //Y: 0, Cr: 1, Cb: 1
	}
}
void read_dht(void){
	uint16_t len;
	read_uint16_t(&len);
	len-=2;
	uint8_t TcTh, Th, value;
	uint16_t code;
	uint8_t l[16];
	while(len){
		read_uint8_t(&TcTh);
		TcTh = (TcTh>>3) | (TcTh&1); // Tc(AC, DC), Th(ID)
		assert(TcTh<4);
		for(int i=0; i<16; i++)
			read_uint8_t(&l[i]);
		len-=17;
		code = 0;
		// build huffman table (key = pair(bit, codeword), val = value)
		for(int i=0; i<16; i++){
			for(int j=0;j<l[i];j++){
				read_uint8_t(&value);
				dht[TcTh].huffman_table[pair(i+1,code)] = (1<<8)|value;
				code++;
				len--;
			}
			code <<= 1;
		}
	}
}
void read_sos(void){
	uint16_t len;
	read_uint16_t(&len);
	read_uint8_t(&sos.Ns);
	for(int i=0; i<sos.Ns; i++){
		uint8_t Cs;
		read_uint8_t(&Cs); //1,2,3 = Y,Cr,Cb
		uint8_t TdTa;
		read_uint8_t(&TdTa);
		sos.comp[Cs].Td = TdTa>>4;
		sos.comp[Cs].Ta = TdTa&0xF;
	}
	uint8_t ss, se, alah;
	read_uint8_t(&ss);
	read_uint8_t(&se);
	read_uint8_t(&alah);
	assert(ss == 0x00 && se == 0x3F && alah == 0x00);
}
void read_block(int block[], int id, int *predictor){
	int dc_id = (_DC<<1) | sos.comp[id].Td;
	int ac_id = (_AC<<1) | sos.comp[id].Ta;
	uint8_t bit, len, find = 0;
	uint16_t key=0, diff, rrrr, ssss;
	// read a 8*8 block to mcu

	// read from DC huffman table
	for(int b=0;b<17;b++){
		bit = get_next_bit();
		key = (key<<1) | bit;
		uint16_t it = dht[dc_id].huffman_table[pair(b+1, key)];
		if( it>>8){
			len = it&0xFF;
			find = 1;
			break;
		}
	}
	assert(find==1);
	diff = 0;
	for(int i=0;i<len;i++)
		diff = (diff<<1) | get_next_bit();
	block[0] = *predictor + extend(len, diff); //DC(n)=DC(n-1)+extend(diff)
	*predictor = block[0];
	
	int now=1;
	while(now<64){
		key = find = 0;
		//read from AC huffman table
		for(int b=0;b<17;b++){
			bit = get_next_bit();
			key = (key<<1) | bit;
			uint16_t it = dht[ac_id].huffman_table[pair(b+1, key)];
			if( it>>8){
				len = it&0xFF;
				find = 1;
				break;
			}
		}
		assert(find==1);
		if(len == 0x00){ //EOB
			while(now<64) block[now++]=0;
			return ;
		}
		//read_len: (# of zeros, # of bits to read)
		rrrr = len>>4;
		len&=0xF;
		now+=rrrr;
		if(now>64) return ;
		
		ssss = 0;
		for(int i=0;i<len;i++)
			ssss = (ssss<<1) | get_next_bit();
		block[now++] = extend(len, ssss);
	}
	assert(now==64);
}
void block_dqt(int block[], int id){
	int tq_id = sof0.comp[id].Tq;
	for(int i=0;i<64;i++)
		block[i] *= dqt.q_table_8[tq_id][i];
}
void zigzag(int block[], int mat[][8]){
	int now=0;
	for(int k=0;k<8;k++){
		if(k&1){
			for(int i=0, j=k; i<=k;i++, j--)
				mat[i][j]=block[now++];
		}else{
			for(int j=0, i=k; j<=k;j++, i--)
				mat[i][j]=block[now++];
		}
	}
	for(int k=1;k<8;k++){
		if(k&1){
			for(int i=7, j=k;j<8;i--, j++)
				mat[i][j]=block[now++];
		}else{
			for(int j=7, i=k; i<8; j--, i++)
				mat[i][j]=block[now++];
		}
	}
}
double t[8], a[4], b[4];
void idct_1d(double *x, double *y){
	//ref: http://blog.sina.com.cn/s/blog_4e19c4c80100gjbf.html
	t[0] = x[0] * c[4];
	t[1] = x[2] * c[2];
	t[2] = x[2] * c[6];
	t[3] = x[4] * c[4];
	t[4] = x[6] * c[6];
	t[5] = x[6] * c[2];
	a[0] = t[0] + t[1] + t[3] + t[4];
	a[1] = t[0] + t[2] - t[3] - t[5];
	a[2] = t[0] - t[2] - t[3] + t[5];
	a[3] = t[0] - t[1] + t[3] - t[4];
	b[0] = x[1]*c[1] + x[3]*c[3] + x[5]*c[5] + x[7]*c[7];
	b[1] = x[1]*c[3] - x[3]*c[7] - x[5]*c[1] - x[7]*c[5];
	b[2] = x[1]*c[5] - x[3]*c[1] + x[5]*c[7] + x[7]*c[3];
	b[3] = x[1]*c[7] - x[3]*c[5] + x[5]*c[3] - x[7]*c[1];
	y[0*8] = a[0] + b[0];
	y[7*8] = a[0] - b[0];
	y[1*8] = a[1] + b[1];
	y[6*8] = a[1] - b[1];
	y[2*8] = a[2] + b[2];
	y[5*8] = a[2] - b[2];
	y[3*8] = a[3] + b[3];
	y[4*8] = a[3] - b[3];
}
double dmat[8][8];
void idct(int mat[][8]){
	double tmp[64];
	for(int i=0;i<8;i++)
		for(int j=0;j<8;j++)
			dmat[i][j]=mat[i][j];
	for(int i=0;i<8;i++) idct_1d((double *)dmat[i], tmp+i); // row -> col
	for(int i=0;i<8;i++) idct_1d(tmp+8*i, (double *)dmat+i); //row -> col
	for(int i=0;i<8;i++)
		for(int j=0;j<8;j++)
			mat[i][j]=round(dmat[i][j]/4);
}
void print(int mat[][8]){
	for(int i=0;i<8;i++){
		for(int j=0;j<8;j++)
			printf("%d ",mat[i][j]);
		printf("\n");
	}
	printf("\n");
}
void read_mcu(int y,int x){
	int block[64];
	int mat[8][8];
	// get Y, Cb, Cr blocks for a MCU
	for(int i=1;i<=sof0.Nf;i++){ //1,2,3 = Y,Cr,Cb
		//[4:1:1] or [1:1:1] in a MCU block
		int scale_v = sof0.Vmax/sof0.comp[i].V, scale_h = sof0.Hmax/sof0.comp[i].H;
		for(int iy=0; iy<sof0.comp[i].V; iy++) 
			for(int ix=0; ix<sof0.comp[i].H; ix++){
				// decode a block and do dqt, zigzag, idct
				memset(block, 0, sizeof(block));
				read_block(block, i, &mcu.pred[i]);
				block_dqt(block, i);
				zigzag(block, mat);
				//print(mat); /*debug*/
				idct(mat);
				//print(mat); /*debug*/
				// save to real pixel
				for(int yy=0;yy<8;yy++)
					for(int xx=0;xx<8;xx++)
						fill_pixel(8*(y*sof0.comp[i].V+iy)+yy, 8*(x*sof0.comp[i].H+ix)+xx,
							scale_v, scale_h, i, mat[yy][xx]);
			}
	}
}
