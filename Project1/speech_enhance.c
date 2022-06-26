#include "speech_enhance.h"

#define	PI		     	3.141592653589793
#define	TRUE			1
#define	FALSE			0
#define	LO_CHAN			0
#define	MID_CHAN		5
#define	HI_CHAN			15
#define	UPDATE_THLD		35
#define	METRIC_THLD		45
#define	INDEX_THLD		12
#define	SETBACK_THLD	12
#define	SNR_THLD		6
#define	INDEX_CNT_THLD	5
#define	UPDATE_CNT_THLD	50
#define	NORM_ENRG		(1.0f)	//use (32768.0 * 32768.0) for fractional
#define	NOISE_FLOOR		(1.0f / NORM_ENRG)
#define	MIN_CHAN_ENRG	(0.0625f / NORM_ENRG)
#define	INE			    (16.0f / NORM_ENRG)
#define	MIN_GAIN		(-13.0f)
#define	GAIN_SLOPE		0.39
#define	CNE_SM_FAC		0.1f
#define	CEE_SM_FAC		0.55f
#define	EMP_FAC		    0.8
#define	HYSTER_CNT_THLD	6    		// forced update constants...
#define	HIGH_TCE_DB		(50.0f)		// 50 - 10log10(NORM_ENRG)
#define	LOW_TCE_DB		(30.0f)		// 30 - 10log10(NORM_ENRG)
#define	HIGH_ALPHA		0.99f
#define	LOW_ALPHA		0.50f
#define	DEV_THLD		28.0

#define	SEND_AMP_SCALE	   (1.5f)
#define	RECEIVE_AMP_SCALE  (0.06167f)

const short bark_hertz_sup[BARK_NUM]={100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500, 22050};
/* The channel table is defined below. In this table, the lower and higher frequency coefficients for each of the 16 channels are specified.  
The table excludes the coefficients with numbers 0 (DC), 1, and N/2 (Foldover frequency). For these coefficients, the gain is always set at 1.0 (0 dB). */
short ch_tbl[BARK_NUM]; //short ch_tbl[NUM_CHAN+1] = {2, 4, 6, 8, 10, 12, 14, 17, 20, 23, 27, 31, 36, 42, 49, 56, 64};

//float send_mapping[FFT_LEN / 2 + 1] = { 0.1f, 0.1f, 0.05f, 0.05f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.9f, 0.5f, 0.1f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.03f, 0.05f, 0.07f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f};
//float send_mapping[FFT_LEN / 2 + 1] = { 0.1f, 0.5f, 0.05f, 0.05f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.0f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.8f };
//2 
// 0.165294f, 19.439783f, 0.339882f, 0.601918f, 0.291381f, 0.172748f, 0.143580f, 0.228155f, 0.316107f, 0.506351f, 0.639738f, 0.729481f, 1.605611f, 2.547345f, 3.145179f, 3.900484f, 3.746487f, 3.281047f, 3.378426f, 3.436468f, 3.911623f, 4.056800f, 3.970558f, 4.776396f, 6.491835f, 6.352293f, 7.256085f, 7.300703f, 6.907291f, 7.297028f, 7.493710f, 8.743755f, 7.985112f, 7.312907f, 5.218859f, 4.531967f, 6.681504f, 9.351245f, 10.098378f, 7.876450f, 6.573970f, 8.048247f, 10.290954f, 9.833525f, 7.855881f, 5.618460f, 4.571082f, 4.974785f, 5.077844f, 4.987084f, 3.914375f, 2.407781f, 2.931524f, 4.348115f, 5.168255f, 6.092302f, 6.984823f, 6.487634f, 6.292389f, 6.895742f, 8.509615f, 11.095323f, 12.804659f, 14.636696f, 18.058093f
//float send_mapping[FFT_LEN / 2 + 1] = { 0.165294f, 19.439783f, 0.339882f, 0.601918f, 0.291381f, 0.172748f, 0.143580f, 0.228155f, 0.316107f, 0.506351f, 0.639738f, 0.729481f, 1.605611f, 2.547345f, 3.145179f, 3.900484f, 3.746487f, 3.281047f, 3.378426f, 3.436468f, 3.911623f, 4.056800f, 3.970558f, 4.776396f, 6.491835f, 6.352293f, 7.256085f, 7.300703f, 6.907291f, 7.297028f, 7.493710f, 8.743755f, 7.985112f, 7.312907f, 5.218859f, 4.531967f, 6.681504f, 9.351245f, 10.098378f, 7.876450f, 6.573970f, 8.048247f, 10.290954f, 9.833525f, 7.855881f, 5.618460f, 4.571082f, 4.974785f, 5.077844f, 4.987084f, 3.914375f, 2.407781f, 2.931524f, 4.348115f, 5.168255f, 6.092302f, 6.984823f, 6.487634f, 6.292389f, 6.895742f, 8.509615f, 11.095323f, 12.804659f, 14.636696f, 18.058093f };
//float send_mapping[FFT_LEN / 2 + 1] = { 0.165294f, 0.1f, 0.339882f, 0.601918f, 0.291381f, 0.172748f, 0.143580f, 0.228155f, 0.316107f, 0.506351f, 0.639738f, 0.729481f, 1.605611f, 2.547345f, 3.145179f, 3.900484f, 3.746487f, 3.281047f, 3.378426f, 3.436468f, 2.911623f, 3.056800f, 2.970558f, 3.776396f, 3.491835f, 3.352293f, 4.256085f, 4.300703f, 2.907291f, 3.297028f, 2.493710f, 1.743755f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.8f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f };
//float send_mapping[FFT_LEN / 2 + 1] = { 0.165294f, 0.1f, 0.339882f, 0.601918f, 0.291381f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.605611f, 2.547345f, 3.145179f, 3.900484f, 3.746487f, 3.281047f, 3.378426f, 3.436468f, 2.911623f, 3.056800f, 2.970558f, 1.776396f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.8f, 0.5f, 0.3f, 0.2f, 0.2f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f };
//float send_mapping[FFT_LEN / 2 + 1] = { 0.165294f, 0.1f, 0.339882f, 0.601918f, 0.291381f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.605611f, 1.547345f, 2.145179f, 2.900484f, 2.746487f, 2.281047f, 2.378426f, 2.436468f, 2.911623f, 3.056800f, 2.970558f, 1.776396f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.8f, 0.5f, 0.3f, 0.2f, 0.2f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f };
float send_mapping[FFT_LEN / 2 + 1] = { 0.165294f, 0.1f, 0.339882f, 0.601918f, 0.291381f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.605611f, 2.547345f, 3.145179f, 3.900484f, 3.746487f, 3.281047f, 3.378426f, 3.436468f, 2.911623f, 3.056800f, 2.970558f, 1.776396f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

//float send_mapping[FFT_LEN / 2 + 1];
//1
//float send_mapping[FFT_LEN/2+1]={0.679115f, 16.705442f, 0.633488f, 0.724088f, 1.109552f, 2.162452f, 2.489466f, 2.538038f, 2.712506f, 4.112272f, 4.395670f, 3.580610f, 2.830832f, 1.941734f, 1.866886f, 1.701211f, 2.234871f, 2.598520f, 2.877721f, 2.636879f, 2.598262f, 3.550008f, 4.064141f, 3.694385f, 2.666634f, 3.050226f, 2.556629f, 3.886579f, 3.631677f, 6.062041f, 8.556084f, 10.354410f, 7.085280f, 6.845908f, 9.474546f, 10.009386f, 10.124274f, 12.385788f, 12.400662f, 10.929418f, 6.604003f, 7.157397f, 8.474338f, 6.606052f, 8.606502f, 14.382243f, 22.976209f, 25.673787f, 17.586146f, 18.556010f, 16.033992f, 17.441560f, 19.127387f, 16.829576f, 17.348109f, 18.305007f, 17.378802f, 19.899981f, 26.051218f, 28.111084f, 28.127010f, 22.854004f, 21.333104f, 22.174496f, 18.898522f};
//3
//float send_mapping[FFT_LEN / 2 + 1] = {0.616623f, 79.136993f, 1.300180f, 2.129382f, 1.025397f, 0.608275f, 0.506808f, 0.822787f, 1.130682f, 1.797493f, 2.269682f, 2.604778f, 5.559717f, 8.942189f, 10.879800f, 13.648147f, 13.145476f, 11.602342f, 11.840329f, 12.206965f, 14.162971f, 14.637497f, 14.308392f, 17.567221f, 24.894111f, 24.410282f, 27.349846f, 27.789869f, 25.985559f, 28.123170f, 28.622032f, 33.362007f, 30.512929f, 28.108494f, 20.353468f, 17.542636f, 25.718196f, 36.714846f, 40.300261f, 32.747610f, 27.775031f, 33.619792f, 42.734055f, 40.948840f, 32.226249f, 22.900821f, 18.163715f, 18.940066f, 19.019489f, 19.301923f, 15.066222f, 9.154601f, 11.225320f, 16.873175f, 20.414088f, 24.439227f, 27.887969f, 26.488399f, 25.602123f, 27.937573f, 34.301683f, 44.189189f, 51.297847f, 58.585865f, 73.536157f, };
//4
//float send_mapping[FFT_LEN / 2 + 1] = { 0.098547f, 8.766829f, 0.278216f, 0.722564f, 0.242722f, 0.139343f, 0.139058f, 0.176253f, 0.196501f, 0.237971f, 0.304257f, 0.359830f, 0.517530f, 0.582611f, 0.567243f, 0.561834f, 0.625319f, 0.693872f, 0.802203f, 0.903304f, 0.973096f, 1.229256f, 1.618994f, 2.107956f, 2.615035f, 2.704446f, 2.309685f, 2.257525f, 2.360202f, 2.252804f, 2.343152f, 2.342670f, 2.426314f, 2.669378f, 2.851231f, 3.147871f, 3.617262f, 3.734644f, 3.855321f, 4.061499f, 4.220835f, 3.873858f, 3.350594f, 2.933339f, 3.100467f, 3.023930f, 2.462709f, 2.425707f, 2.466360f, 2.170229f, 2.165141f, 2.387136f, 2.672269f, 3.262435f, 4.097551f, 4.711580f, 4.728617f, 4.798533f, 5.973477f, 7.219868f, 7.592217f, 7.404726f, 7.593904f, 8.304799f, 8.850334f };
float receive_mapping[FFT_LEN/2+1]={1.0000f,   42.1697f,   18.8365f,    1.0000f,    7.9433f,   14.1254f,   15.8489f,   16.7880f,   16.7880f,   16.7880f,   16.7880f,   16.7880f,   16.7880f,   15.8489f,   14.9624f,   14.1254f,   13.3352f,   12.3027f,   11.8850f,   11.2202f,   11.2202f,   10.9648f,   10.8393f,   11.2202f,   12.5893f,   14.1254f,   13.3352f,   11.8850f,   11.8850f,   10.3514f,   10.2329f,   12.3027f,   14.1254f,   15.8489f,   16.7880f,   17.7828f,   18.8365f,   19.9526f,   22.3872f,   23.7137f,   25.1189f,   26.6073f,   28.1838f,   29.5121f,   31.6228f,   31.6228f,   32.3594f,   33.4965f,   34.6737f,   35.4813f,   35.4813f,   35.4813f,   35.4813f,   35.8922f,   36.3078f,   36.7282f,   37.1535f,   38.0189f,   38.4592f,   39.3550f,   39.8107f,   40.7380f,   41.2098f,   42.1697f,   42.6580f};

/* The voice metric table is defined below.  It is a non-linear table with a deadband near zero. It maps the SNR index (quantized SNR value) to a number that is a measure of voice quality. */ 
const short	vm_tbl [90] = {
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 13, 14, 15, 15, 16, 17, 17, 18, 19, 20, 20, 21, 22, 23, 24, 24, 25, 26, 27, 28, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50};
float window [DELAY+FRM_LEN];
double phs_tbl[FFT_LEN];

/*****************************************************************
1) This function is designed to the N-point FFT of a real sequence with a N/2-point complex FFT. The FFT size can be specified through "define" statements.
2) In the c_fft function, the FFT values are divided by 2 after each stage of computation thus dividing the final FFT values by N/2. No multiplying factor is used for the IFFT. 
   This is somewhat different from the usual definition of FFT where the factor 1/N, is used for the IFFT and not the FFT. No factor is used in the r_fft function.
3) Much of the code for the FFT and IFFT parts in r_fft and c_fft functions are similar and can be combined. However, they are kept separate here to speed up the execution.
*****************************************************************/
/* FFT/IFFT function for complex sequences */
/* The decimation-in-time complex FFT/IFFT is implemented below. The input complex numbers are presented as real part followed by imaginary part for each sample. 
The counters are therefore incremented by two to access the complex valued samples. */
void c_fft (float *bone_buf, short isign)
{
    short i, j, k, ii, jj, kk, ji, kj;
    float ftmp, re7, im7;

	/* Rearrange the input array in bit reversed order */
    for (i = j = 0; i < FFT_LEN-2; i += 2)
	{
        if (j > i)
	    {   
            ftmp = bone_buf[i];
            bone_buf[i] = bone_buf[j];
            bone_buf[j] = ftmp;
            
            ftmp = bone_buf[i+1];
            bone_buf[i+1] = bone_buf[j+1];
            bone_buf[j+1] = ftmp;
        }        
        k = FFT_LEN/2;
        while (j >= k)
	    {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    /* The FFT part */
    if (isign == 1)
    {
        for (i = 0; i < NUM_STAGE; i++) //i is stage counter
	    {
            jj = 2 << i;	//FFT size
            kk = 4 << i;	//2*FFT size
            ii = (short)(FFT_LEN >> 1 >> i);	//2*number of FFT's
            
            for (j = 0; j < jj; j += 2) //j is sample counter
	        {
                ji = j * ii;  //ji is phase table index
                for (k = j; k < FFT_LEN; k += kk) //k is butterfly top
				{	
					kj = k + jj; //kj is butterfly bottom
					
					// Butterfly computations
					re7 = bone_buf[kj] * (float)phs_tbl[ji] - bone_buf[kj+1] * (float)phs_tbl[ji+1];
					im7 = bone_buf[kj+1] * (float)phs_tbl[ji] + bone_buf[kj] * (float)phs_tbl[ji+1];
					bone_buf[kj] = (bone_buf[k] - re7) / 2;
					bone_buf[kj+1] = (bone_buf[k+1] - im7) / 2;
					bone_buf[k] = (bone_buf[k] + re7) / 2;
					bone_buf[k+1] = (bone_buf[k+1] + im7) / 2;
				}
            }
        }
    }  
    else //The IFFT part
    {
        for (i = 0; i < NUM_STAGE; i++) //i is stage counter
		{    
			jj = 2 << i;	//FFT size
			kk = 4 << i;	//2*FFT size
			ii = (short)(FFT_LEN >> 1 >> i);	//2*number of FFT's
    
			for (j = 0; j < jj; j += 2) //j is sample counter
			{    
				ji = j * ii;	//ji is phase table index
				for (k = j; k < FFT_LEN; k += kk) //k is butterfly top
				{    
					kj = k + jj;				//kj is butterfly bottom
    
					//Butterfly computations
					re7 = bone_buf[kj] * (float)phs_tbl[ji] + bone_buf[kj+1]*(float)phs_tbl[ji+1];
					im7 = bone_buf[kj+1] * (float)phs_tbl[ji] - bone_buf[kj]*(float)phs_tbl[ji+1];
					bone_buf[kj] = bone_buf[k] - re7;
					bone_buf[kj+1] = bone_buf[k+1] - im7;
					bone_buf[k] = bone_buf[k] + re7;
					bone_buf[k+1] = bone_buf[k+1] + im7;
				}
			}
		}
    }
}

void r_fft (float *bone_buf, short isign)
{
	int	i, j;
    float re8, im8, re9, im9;    
   
    /* The FFT part */
    if (isign == 1)
	{
		/* Perform the complex FFT */
		c_fft (bone_buf, isign);
    
		/* First, handle the DC and foldover frequencies */
		re8 = bone_buf[0];
		re9 = bone_buf[1];
		bone_buf[0] = re8 + re9;
		bone_buf[1] = re8 - re9;

        /* Now, handle the remaining positive frequencies */
        for (i = 2; i <= FFT_LEN/2; i += 2)
		{
			j = FFT_LEN - i;
		    re8 = bone_buf[i] + bone_buf[j];
		    im8 = bone_buf[i+1] - bone_buf[j+1];
		    re9 = bone_buf[i+1] + bone_buf[j+1];
       	    im9 = bone_buf[j] - bone_buf[i];

		    bone_buf[i] = (re8 + (float)phs_tbl[i] * re9 - (float)phs_tbl[i+1] * im9) / 2;
		    bone_buf[i+1] = (im8 + (float)phs_tbl[i] * im9 + (float)phs_tbl[i+1] * re9) / 2;
		    bone_buf[j] = (re8 + (float)phs_tbl[j] * re9 + (float)phs_tbl[j+1] * im9) / 2;
		    bone_buf[j+1] = (-im8 - (float)phs_tbl[j] * im9 + (float)phs_tbl[j+1] * re9) / 2;
		}
	}
    else /* The IFFT part */
	{
		/* First, handle the DC and foldover frequencies */
		re8 = bone_buf[0];
		re9 = bone_buf[1];
		bone_buf[0] = (re8 + re9) / 2;
		bone_buf[1] = (re8 - re9) / 2;

		/* Now, handle the remaining positive frequencies */
		for (i = 2; i <= FFT_LEN/2; i += 2)
		{
			j = FFT_LEN - i;
			re8 = bone_buf[i] + bone_buf[j];
			im8 = bone_buf[i+1] - bone_buf[j+1];
			re9 = -bone_buf[i+1] - bone_buf[j+1];
			im9 = -bone_buf[j] + bone_buf[i];

			bone_buf[i] = (re8 + (float)phs_tbl[i] * re9 + (float)phs_tbl[i+1] * im9) / 2;
			bone_buf[i+1] = (im8 + (float)phs_tbl[i] * im9 - (float)phs_tbl[i+1] * re9) / 2;
			bone_buf[j] = (re8 + (float)phs_tbl[j] * re9 - (float)phs_tbl[j+1] * im9) / 2;
			bone_buf[j+1] = (-im8 - (float)phs_tbl[j] * im9 - (float)phs_tbl[j+1] * re9) / 2;
		}
		/* Perform the complex IFFT */
		c_fft (bone_buf, isign);
	}
}

void speech_enhance_init(ENHANCE_ST *st)
{
	short i;
	double theta;

	for (i=0; i<BARK_NUM; i++)
	{
		ch_tbl[i]=(short)(0.017733f*bark_hertz_sup[i]+1.138f);
	}

	/* use smoothed trapezoidal window */
	for (i=0; i<DELAY; i++)
	{
		window[FRM_LEN+DELAY-1-i] = window[i] = (float)pow(sin((2*i+1)*(float)PI/(4*DELAY)), 2);
	}
	for (i=DELAY; i<FRM_LEN; i++)
	{
		window[i] = 1;
	}

    for (i = 0; i < FFT_LEN/2; i++)
    {
        theta = i * (- 2*PI / FFT_LEN);
        phs_tbl[2*i] = (float)cos(theta);
        phs_tbl[2*i+1] = (float)sin(theta);
    }

	for (i=0; i<FFT_LEN/2+1; i++)
	{
		//send_mapping[i] *= SEND_AMP_SCALE;
		receive_mapping[i] *= RECEIVE_AMP_SCALE;
	}

	st->send_frame_cnt=0;
	st->send_hyster_cnt=0;
	st->send_last_update_cnt=0;
	st->send_update_cnt=0;
	for (st->send_pre_emp_mem=i=0; i<DELAY; i++)
	{
		st->send_in_overlap[i]=0;
	}
	for (i=0; i<NUM_CHAN; i++)
	{
		st->send_ch_enrg_long_db[i] = st->send_ch_noise[i] = st->send_ch_enrg[i] = 0;
	}
	for (st->send_de_emp_mem=i=0; i<FFT_LEN-FRM_LEN; i++)
	{
		st->send_out_overlap[i]=0;
	}

	st->receive_frame_cnt=0;
	st->receive_hyster_cnt=0;
	st->receive_last_update_cnt=0;
	st->receive_update_cnt=0;
	for (st->receive_pre_emp_mem=i=0; i<DELAY; i++)
	{
		st->receive_in_overlap[i]=0;
	}
	for (i=0; i<NUM_CHAN; i++)
	{
		st->receive_ch_enrg_long_db[i] = st->receive_ch_noise[i] = st->receive_ch_enrg[i] = 0;
	}
	for (st->receive_de_emp_mem=i=0; i<FFT_LEN-FRM_LEN; i++)
	{
		st->receive_out_overlap[i]=0;
	}

	st->air_frame_cnt=0;
	st->air_hyster_cnt=0;
	st->air_last_update_cnt=0;
	st->air_update_cnt=0;
	for (st->air_pre_emp_mem=i=0; i<DELAY; i++)
	{
		st->air_in_overlap[i]=0;
	}
	for (i=0; i<NUM_CHAN; i++)
	{
		st->air_ch_enrg_long_db[i] = st->air_ch_noise[i] = st->air_ch_enrg[i] = 0;
	}
	for (st->air_de_emp_mem=i=0; i<FFT_LEN-FRM_LEN; i++)
	{
		st->air_out_overlap[i]=0;
	}

	for (i=0; i<FFT_LEN; i++)
	{
		st->cab[i]=0;
	}
	for (i=0; i<FFT_LEN/2; i++)
	{
		st->paa[i]=0;
	}
	/*int num = 0;
	
	for (num = 0; num < FFT_LEN / 2 + 1; num++)
	{
		send_mapping[num] = 1.0f;
	}*/

	st->timbre_enhance_flag = TIMBRE_ENHANCE_ON;
	st->statistic_noise_suppress_flag = STATISTIC_NOISE_SUPPRESS_ON;
	st->active_noise_suppress_flag = ACTIVE_NOISE_SUPPRESS_ON;
	st->perception_enhance_flag = PERCEPTION_ENHANCE_ON;
}

void speech_enhance_config(ENHANCE_ST *st, short timbre_enhance_flag, short statistic_noise_suppress_flag, short active_noise_suppress_flag, short perception_enhance_flag)
{
	st->timbre_enhance_flag = timbre_enhance_flag;
	st->statistic_noise_suppress_flag = statistic_noise_suppress_flag;
	st->active_noise_suppress_flag = active_noise_suppress_flag;
    st->perception_enhance_flag = perception_enhance_flag;
}

/* The noise supression function */
void speech_enhance(ENHANCE_ST *st, short *bone_send_in, short *send_out, short *receive_in, short *receive_out, short *air_in)
{
	short i, j, vm_sum, update_flag, index_cnt, ch_snr[NUM_CHAN];
    float bone_buf[FFT_LEN], enrg, tne, tce, alpha, gain, ch_enrg_dev, vv, ch_enrg_db[NUM_CHAN];
	float receive_buf[FFT_LEN], air_buf[FFT_LEN];
	float vr, vi;

	/* Preemphasize the input data and store in the data buffer with appropriate delay */
	bone_buf[DELAY] = (float)(bone_send_in[0] - EMP_FAC * st->send_pre_emp_mem);
    for (i = 1; i < FRM_LEN; i++)
	{
		bone_buf[DELAY+i] = (float)(bone_send_in[i] - EMP_FAC * bone_send_in[i-1]);
	}
    st->send_pre_emp_mem = bone_send_in[FRM_LEN-1];
    /* update send_in_overlap buffer */
	for (i = 0; i < DELAY; i++)
	{
		bone_buf[i] = st->send_in_overlap[i];
	}
    for (i = 0; i < DELAY; i++)
	{
		st->send_in_overlap[i] = bone_buf[FRM_LEN+i];
	}
    /* Apply window to frame prior to FFT */
    for (i = 0; i < FRM_LEN+DELAY; i++)
	{
		bone_buf[i] *= window[i];
	}
	for (i = DELAY + FRM_LEN; i < FFT_LEN; i++)
	{
		bone_buf[i] = 0;
	}


	/* Preemphasize the input data and store in the data buffer with appropriate delay */
	receive_buf[DELAY] = (float)(receive_in[0] - EMP_FAC * st->receive_pre_emp_mem);
    for (i = 1; i < FRM_LEN; i++)
	{
		receive_buf[DELAY+i] = (float)(receive_in[i] - EMP_FAC * receive_in[i-1]);
	}
    st->receive_pre_emp_mem = receive_in[FRM_LEN-1];
    /* update send_in_overlap buffer */
	for (i = 0; i < DELAY; i++)
	{
		receive_buf[i] = st->receive_in_overlap[i];
	}
    for (i = 0; i < DELAY; i++)
	{
		st->receive_in_overlap[i] = receive_buf[FRM_LEN+i];
	}
    /* Apply window to frame prior to FFT */
    for (i = 0; i < FRM_LEN+DELAY; i++)
	{
		receive_buf[i] *= window[i];
	}
	for (i = DELAY + FRM_LEN; i < FFT_LEN; i++)
	{
		receive_buf[i] = 0;
	}

	/* Preemphasize the input data and store in the data buffer with appropriate delay */
	air_buf[DELAY] = (float)(air_in[0] - EMP_FAC * st->air_pre_emp_mem);
    for (i = 1; i < FRM_LEN; i++)
	{
		air_buf[DELAY+i] = (float)(air_in[i] - EMP_FAC * air_in[i-1]);
	}
    st->air_pre_emp_mem = air_in[FRM_LEN-1];
    /* update send_in_overlap buffer */
	for (i = 0; i < DELAY; i++)
	{
		air_buf[i] = st->air_in_overlap[i];
	}
    for (i = 0; i < DELAY; i++)
	{
		st->air_in_overlap[i] = air_buf[FRM_LEN+i];
	}
    /* Apply window to frame prior to FFT */
    for (i = 0; i < FRM_LEN+DELAY; i++)
	{
		air_buf[i] *= window[i];
	}
	for (i = DELAY + FRM_LEN; i < FFT_LEN; i++)
	{
		air_buf[i] = 0;
	}

    /* Perform FFT on the data buffer */
    r_fft (bone_buf, 1);
	r_fft (receive_buf, 1);
	r_fft (air_buf, 1);

	if (st->timbre_enhance_flag == TIMBRE_ENHANCE_ON)
	{
		bone_buf[0] *= send_mapping[0];
		bone_buf[1] *= send_mapping[1];	
		for (j = 1; j < FFT_LEN/2; j++)
		{
			bone_buf[2*j] *= send_mapping[j+1];
			bone_buf[2*j+1] *= send_mapping[j+1];	
		}
	}

	if(st->perception_enhance_flag==PERCEPTION_ENHANCE_ON)
	{
		receive_buf[0] *= receive_mapping[0];
		receive_buf[1] *= receive_mapping[1];	
		for (j = 1; j < FFT_LEN/2; j++)
		{
			receive_buf[2*j] *= receive_mapping[j+1];
			receive_buf[2*j+1] *= receive_mapping[j+1];	
		}		
	}

    /* Estimate the energy in each channel */
    alpha = (st->send_frame_cnt == 0) ? 1.0f : CEE_SM_FAC;
    for (tce = 0, i = LO_CHAN; i <= HI_CHAN; i++)
	{
		for (enrg = 0, j = ch_tbl[i]; j < min(FFT_LEN/2, ch_tbl[i+1]); j++)
		{
			enrg += bone_buf[2*j]*bone_buf[2*j] + bone_buf[2*j+1]*bone_buf[2*j+1]; 
		}
		st->send_ch_enrg[i] = max(MIN_CHAN_ENRG, (1 - alpha) * st->send_ch_enrg[i] + alpha * enrg / (ch_tbl[i+1] - ch_tbl[i]));
		tce += st->send_ch_enrg[i];  // Compute the total channel energy estimate (tce)
		ch_enrg_db[i] = 10.0f*(float)log10(st->send_ch_enrg[i]); //Calculate log spectral deviation
	}

	if (st->send_frame_cnt == 0) {for (i = LO_CHAN; i <= HI_CHAN; i++) st->send_ch_enrg_long_db[i] = ch_enrg_db[i];}
	for (ch_enrg_dev = 0, i = LO_CHAN; i <= HI_CHAN; i++) ch_enrg_dev += (float)fabs(st->send_ch_enrg_long_db[i] - ch_enrg_db[i]);

    /* Calculate long term integration constant as a function of total channel energy (tce) */
    /* (i.e., high tce (-40 dB) -> slow integration (alpha = 0.99), low tce (-60 dB) -> fast integration (alpha = 0.50) */
    alpha = HIGH_ALPHA - ((HIGH_ALPHA - LOW_ALPHA) / (HIGH_TCE_DB - LOW_TCE_DB)) * (HIGH_TCE_DB - 10.0f*(float)log10(tce));
    if(alpha > HIGH_ALPHA)
	{
		alpha = HIGH_ALPHA;
	}
    else if(alpha < LOW_ALPHA)
	{
		alpha = LOW_ALPHA;
	}

    /* Initialize channel noise estimate to channel energy of first four frames */
    if (st->send_frame_cnt < 4)
	{
		for (i = LO_CHAN; i <= HI_CHAN; i++)
		{
			st->send_ch_noise[i] = max(st->send_ch_enrg[i], INE);
		}
	}
	st->send_frame_cnt=min(16, st->send_frame_cnt + 1);

    /* Compute the channel SNR indices */
    for (vm_sum = 0, tne = 0, i = LO_CHAN; i <= HI_CHAN; i++)
	{
		ch_snr[i] = (int)((max(0, 10 * (float)log10(st->send_ch_enrg[i] / st->send_ch_noise[i])) + 0.1875f) / 0.375f);
		vm_sum += vm_tbl[min(ch_snr[i], 89)]; // Compute the sum of voice metrics
        tne += st->send_ch_noise[i]; // Compute the total noise estimate (tne)		
		st->send_ch_enrg_long_db[i] = alpha * st->send_ch_enrg_long_db[i] + (1.0f-alpha) * ch_enrg_db[i]; //Calc long term log spectral energy
	}    
        
    /* Set or reset the update flag */
    update_flag = FALSE;
    if (vm_sum <= UPDATE_THLD)
    {
        update_flag = TRUE;
        st->send_update_cnt = 0;
    }    
    else if((tce > NOISE_FLOOR)&&(ch_enrg_dev < DEV_THLD))
    {
        st->send_update_cnt++;
        if (st->send_update_cnt >= UPDATE_CNT_THLD)
		{
			update_flag = TRUE;
		}
    }
    
    if(st->send_update_cnt == st->send_last_update_cnt)
	{
		st->send_hyster_cnt++;
	}
    else
	{
		st->send_hyster_cnt = 0;
	}

    st->send_last_update_cnt = st->send_update_cnt;
    if(st->send_hyster_cnt > HYSTER_CNT_THLD)
	{
		st->send_update_cnt = 0;
	}
    
    /* Set or reset modify flag */
    for (index_cnt = 0, i = MID_CHAN; i <= HI_CHAN; i++)
	{
		if(ch_snr[i] >= INDEX_THLD)
		{
			index_cnt++;
		}
	}
    
    /* Modify the SNR indices */
    if(index_cnt < INDEX_CNT_THLD)
    {
        for (i = LO_CHAN; i <= HI_CHAN; i++)
		{
			if((vm_sum <= METRIC_THLD) || (ch_snr [i] <= SETBACK_THLD))
			{
				ch_snr [i] = 1;
			}
		}
    }
    
    /* Update the channel noise estimates */
    if (update_flag == TRUE)
	{
		for (i = LO_CHAN; i <= HI_CHAN; i++)
		{
			st->send_ch_noise[i] = max(MIN_CHAN_ENRG, (1.0f - CNE_SM_FAC) * st->send_ch_noise[i] + CNE_SM_FAC * st->send_ch_enrg[i]);
		}
	}

	if(update_flag == TRUE)
	{
		alpha=0.03f;
	}
	else
	{
		alpha=0.00003f;
		
	}

	for(vv=1.0f-alpha, j=1; j<FFT_LEN/2; j++)
	{
		st->cab[2*j] = vv*st->cab[2*j] + alpha*(air_buf[2*j]*bone_buf[2*j] + air_buf[2*j+1]*bone_buf[2*j+1]);
		st->cab[2*j+1] = vv*st->cab[2*j+1] + alpha*(-air_buf[2*j+1]*bone_buf[2*j] + air_buf[2*j]*bone_buf[2*j+1]);
		st->paa[j] = vv*st->paa[j] + alpha*(air_buf[2*j]*air_buf[2*j] + air_buf[2*j+1]*air_buf[2*j+1]);
	}

	if(st->active_noise_suppress_flag = ACTIVE_NOISE_SUPPRESS_ON)
	{
		for(j=1; j<FFT_LEN/2; j++)
		{
			vr = bone_buf[2*j] - (air_buf[2*j]*st->cab[2*j]-air_buf[2*j+1]*st->cab[2*j+1])/st->paa[j];
			vi = bone_buf[2*j+1] - (air_buf[2*j+1]*st->cab[2*j]+air_buf[2*j]*st->cab[2*j+1])/st->paa[j];
			if(vr*vr + vi*vi < bone_buf[2*j]*bone_buf[2*j] + bone_buf[2*j+1]*bone_buf[2*j+1])
			{
				bone_buf[2*j] = vr;
				bone_buf[2*j+1] = vi;
			}
		}
	}

	if(st->statistic_noise_suppress_flag==STATISTIC_NOISE_SUPPRESS_ON)
	{
		//Compute the channel gains
		vv = max(10.0f * (float)log10(NOISE_FLOOR / tne), MIN_GAIN);
		for (i = LO_CHAN; i <= HI_CHAN; i++)
		{
			gain = (float)((max(SNR_THLD, ch_snr[i]) - SNR_THLD) * GAIN_SLOPE) + vv;
			gain = (float)pow(10.0, min(0, gain)/20);
			
			//Filter the input data in the frequency domain
			for (j = ch_tbl[i]; j < min(FFT_LEN/2, ch_tbl[i+1]); j++)
			{
				bone_buf[2*j] *= gain;
				bone_buf[2*j+1] *= gain;
			}
		} 
	}

	

	/* Perform IFFT */
    r_fft (bone_buf, -1);
	r_fft (receive_buf, -1);	

    /* Overlap add the filtered data from previous block. Save data from this block for the next. */ 
    for (i = 0; i < FFT_LEN - FRM_LEN; i++)
	{
		bone_buf[i] += st->send_out_overlap[i];
	}
    for (i = FRM_LEN; i < FFT_LEN; i++)
	{
		st->send_out_overlap[i-FRM_LEN] = bone_buf[i];
	}
    /* Deemphasize the filtered speech and write it out to farray */
    for (vv = st->send_de_emp_mem, i = 0; i < FRM_LEN; i++)
	{
		vv = (float)(bone_buf[i] + EMP_FAC * vv);
		send_out[i] = (short)max(-32768.0f, min(32767.0f, vv));
	}
    st->send_de_emp_mem = vv;
/*
    for (i = 0; i < FRM_LEN; i++)
	{
		send_out[i] = 5000;
		if (update_flag == TRUE) send_out[i] = 0;
	}
*/
    /* Overlap add the filtered data from previous block. Save data from this block for the next. */ 
    for (i = 0; i < FFT_LEN - FRM_LEN; i++)
	{
		receive_buf[i] += st->receive_out_overlap[i];
	}
    for (i = FRM_LEN; i < FFT_LEN; i++)
	{
		st->receive_out_overlap[i-FRM_LEN] = receive_buf[i];
	}
    /* Deemphasize the filtered speech and write it out to farray */
    for (vv = st->receive_de_emp_mem, i = 0; i < FRM_LEN; i++)
	{
		vv = (float)(receive_buf[i] + EMP_FAC * vv);
		receive_out[i] = (short)max(-32768.0f, min(32767.0f, vv));
	}
    st->receive_de_emp_mem = vv;
}
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
void main(void)
{
    short k, bone_send_in[FRM_LEN], send_out[FRM_LEN], out_sp_std[FRM_LEN];
	short receive_in[FRM_LEN], receive_out[FRM_LEN], air_in[FRM_LEN];
    long  frame;
	ENHANCE_ST my_st;
	FILE *f_si, *f_so, *f_ri, *f_ro, *f_air;

	speech_enhance_init(&my_st);
	speech_enhance_config(&my_st, TIMBRE_ENHANCE_OFF, STATISTIC_NOISE_SUPPRESS_ON, ACTIVE_NOISE_SUPPRESS_ON, PERCEPTION_ENHANCE_OFF);

	fopen_s(&f_si,"bone4.pcm", "rb");
	fopen_s(&f_so,"bone4_ns.pcm", "wb");
	fopen_s(&f_ri,"down.pcm", "rb");
	fopen_s(&f_ro,"receive_out2.pcm", "wb");
	fopen_s(&f_air,"air4.pcm", "rb");
	if ((f_si== NULL)||(f_so==NULL)||(f_ri==NULL)||(f_ro==NULL)||(f_air==NULL)) {printf("Unable to open input file.\n"); exit(-1);}

    for(frame=0; ;frame++)
    {
        if(fread(bone_send_in, sizeof(short), FRM_LEN, f_si)!=FRM_LEN) break;
		if(fread(receive_in, sizeof(short), FRM_LEN, f_ri)!=FRM_LEN) break;
		if(fread(air_in, sizeof(short), FRM_LEN, f_air)!=FRM_LEN) break;
		if(frame%16384==0) printf("NS %d \n", frame);
        speech_enhance(&my_st, bone_send_in, send_out, receive_in, receive_out, air_in);

		fwrite(send_out, sizeof(short), FRM_LEN, f_so);
		fwrite(receive_out, sizeof(short), FRM_LEN, f_ro);
/*
		fread(out_sp_std, sizeof(short), FRM_LEN, f_so);
		for (k=0; k<FRM_LEN; k++)
		{
			if(send_out[k]!=out_sp_std[k]) 
			{printf("error!!!!!!!!!!!!!\n"); getchar(); exit(0);}
		}
*/
    }
	fclose(f_si);
	fclose(f_so);
}
