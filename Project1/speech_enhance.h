#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define	FS		    8000
#define	FRM_LEN     (FS/100)     //10ms
#define	DELAY		24
#define	NUM_CHAN	16
#define	NUM_STAGE	(5+(FS/8000))
#define	FFT_LEN		(2<<NUM_STAGE)
#define BARK_NUM    25

#define TIMBRE_ENHANCE_ON   1
#define TIMBRE_ENHANCE_OFF  0
#define STATISTIC_NOISE_SUPPRESS_ON   1
#define STATISTIC_NOISE_SUPPRESS_OFF  0
#define ACTIVE_NOISE_SUPPRESS_ON   1
#define ACTIVE_NOISE_SUPPRESS_OFF  0
#define PERCEPTION_ENHANCE_ON   1
#define PERCEPTION_ENHANCE_OFF  0

typedef struct
{
	float send_pre_emp_mem; 
	float send_de_emp_mem;
	short send_frame_cnt;
	float send_in_overlap[DELAY];
	float send_ch_enrg[NUM_CHAN];
	float send_ch_noise[NUM_CHAN];
	float send_ch_enrg_long_db[NUM_CHAN];
	short send_hyster_cnt;
	short send_last_update_cnt;
	short send_update_cnt;
	float send_out_overlap[FFT_LEN-FRM_LEN];

	float receive_pre_emp_mem; 
	float receive_de_emp_mem;
	short receive_frame_cnt;
	float receive_in_overlap[DELAY];
	float receive_ch_enrg[NUM_CHAN];
	float receive_ch_noise[NUM_CHAN];
	float receive_ch_enrg_long_db[NUM_CHAN];
	short receive_hyster_cnt;
	short receive_last_update_cnt;
	short receive_update_cnt;
	float receive_out_overlap[FFT_LEN-FRM_LEN];

	float air_pre_emp_mem; 
	float air_de_emp_mem;
	short air_frame_cnt;
	float air_in_overlap[DELAY];
	float air_ch_enrg[NUM_CHAN];
	float air_ch_noise[NUM_CHAN];
	float air_ch_enrg_long_db[NUM_CHAN];
	short air_hyster_cnt;
	short air_last_update_cnt;
	short air_update_cnt;
	float air_out_overlap[FFT_LEN-FRM_LEN];

	float cab[FFT_LEN];
	float paa[FFT_LEN/2];

	short timbre_enhance_flag;
	short statistic_noise_suppress_flag;
	short active_noise_suppress_flag;
	short perception_enhance_flag;
} ENHANCE_ST;

void speech_enhance_init(ENHANCE_ST *st);
void speech_enhance_config(ENHANCE_ST *st, short timbre_enhance_flag, short statistic_noise_suppress_flag, short active_noise_suppress_flag, short perception_enhance_flag);
void speech_enhance(ENHANCE_ST *st, short *bone_send_in, short *send_out, short *receive_in, short *receive_out, short *air_in);