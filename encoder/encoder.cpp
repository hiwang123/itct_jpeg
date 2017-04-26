#include "libs.h"

void write_app0(void){
	uint8_t JFIF[6];
	write_uint16_t(0x0010);
	JFIF[0]=0x4A, JFIF[1]=0x46, JFIF[2]=0x49, JFIF[3]=0x46, JFIF[4]=0x00;
	write_uints(JFIF, 5); //JFIF
	write_uint16_t(0x0101); //version
	write_uint8_t(0x00); //dens unit
	write_uint16_t(0x01); //dens x ??
	write_uint16_t(0x01); //dens y ??
	write_uint8_t(0x00); //thumb h
	write_uint8_t(0x00); //thumb  v
}
void write_dqt(void){
	for(int i=0;i<2;i++){ //Y, CrCb
		write_uint16_t(0xFFDB);
		write_uint16_t(0x0043);
		write_uint8_t(0 | i);
		for(int j=0;j<64;j++)
			write_uint8_t(q_table_8[i][j]);
	}
}
void write_sof0(void){
	uint16_t len;
	write_uint16_t(0x0011);
	write_uint8_t(0x08);
	write_uint16_t(bmp.V);
	write_uint16_t(bmp.H);
	write_uint8_t(0x03);
	for(int i=1;i<=3;i++){
		write_uint8_t(i);
		write_uint8_t(comp[i].H<<4 | comp[i].V);
		write_uint8_t(comp[i].Tq); //Y: 0, Cr: 1, Cb: 1
	}
}
void write_dht(void){ 
	int n=sizeof(std_dht);
	//standard dht table, include tag
	for(int i=0;i<n;i++)
		write_uint8_t(std_dht[i]);
}
void write_sos(void){
	write_uint16_t(0x000C);
	write_uint8_t(0x03); //Ns
	for(int i=1; i<=3; i++){
		//Cs
		write_uint8_t(i); //1,2,3 = Y,Cr,Cb
		//TdTa
		write_uint8_t(comp[i].Td<<4 | comp[i].Ta);
	}
	//ss, se, alah
	write_uint8_t(0x00); write_uint8_t(0x3F); write_uint8_t(0x00); 
}
double PI = acos(-1);
void dct(int mat[][8]){
	double tmp[8][8];
	for(int i=0;i<8;i++)
		for(int j=0;j<8;j++){
			double sum=0;
			for(int k=0;k<8;k++)
				sum+=(double)mat[i][k]*cos((2*k+1)*j*PI/16);
			tmp[i][j] = (j==0 ? (1.0/sqrt(2)):1.0)*sum/2;
		}
	for(int i=0;i<8;i++)
		for(int j=0;j<8;j++){
			double sum=0;
			for(int k=0;k<8;k++)
				sum+=tmp[k][j]*cos((2*k+1)*i*PI/16);
			mat[i][j] = round((i==0 ? (1.0/sqrt(2)):1.0)*sum/2);
		}
}
void azigzag(int mat[][8], int block[]){
	int now=0;
	for(int k=0;k<8;k++){
		if(k&1){
			for(int i=0, j=k; i<=k;i++, j--)
				block[now++]=mat[i][j];
		}else{
			for(int j=0, i=k; j<=k;j++, i--)
				block[now++]=mat[i][j];
		}
	}
	for(int k=1;k<8;k++){
		if(k&1){
			for(int i=7, j=k;j<8;i--, j++)
				block[now++]=mat[i][j];
		}else{
			for(int j=7, i=k; i<8; j--, i++)
				block[now++]=mat[i][j];
		}
	}
}
void block_qt(int block[], int id){
	int tq_id = comp[id].Tq;
	for(int i=0;i<64;i++){
		block[i] = round((double)block[i]/q_table_8[tq_id][i]);
	}
}
void write_block_bits(char *s, int len, int val){
	for(int i=0;i<strlen(s);i++)
		write_next_bit((uint8_t)(s[i]-'0'));
	for(int i=len-1;i>=0;i--)
		write_next_bit((val>>i) & 1);
}
int get_len(int val){
	val=abs(val);
	int ret=0;
	while(val) ret++,val>>=1;
	return ret;
}
int get_val(int len, int val){
	return val>=0 ? val:val+((1<<len)-1);
}
void block_encode(int block[], int id, int *predictor){
	int dc_id = comp[id].Td;
	int ac_id = comp[id].Ta;
	int len, dc, rrrr, ssss, ac;
	//DC
	len = get_len(block[0]-*predictor);
	dc = get_val(len, block[0]-*predictor);
	write_block_bits((char *)std_dc[dc_id][len], len, dc);
	*predictor = block[0];
	//AC
	int last=0;
	for(int i=1;i<64;i++)
		if(block[i]!=0)
			last=i;
	rrrr=0;
	for(int i=1;i<=last;i++){
		if(block[i]==0 && rrrr<15) rrrr++;
		else{
			ssss = get_len(block[i]);
			ac = get_val(ssss, block[i]);
			len = rrrr<<4 | ssss;
			write_block_bits((char *)std_ac[ac_id][len], ssss, ac);
			rrrr=0;
		}
	}
	//EOB
	len = 0x00;
	write_block_bits((char *)std_ac[ac_id][len], 0, 0);
}
void print(int mat[][8]){
	for(int i=0;i<8;i++){
		for(int j=0;j<8;j++)
			printf("%d ",mat[i][j]);
		printf("\n");
	}
}
void gen_mcu(int y, int x){
	int block[64];
	int mat[8][8];
	for(int i=1; i<=3; i++){
		for(int iy=0;iy<8;iy++)
			for(int ix=0;ix<8; ix++){
				if(y-iy>=0 && x+ix<bmp.H)
					mat[iy][ix] = bmp.p[y-iy][x+ix].YCrCb[i];
				else
					mat[iy][ix] = 0;
				mat[iy][ix]-=128;
			}
		dct(mat);
		azigzag(mat, block);
		block_qt(block, i);
		block_encode(block,i,&mcu.pred[i]);
	}
}
