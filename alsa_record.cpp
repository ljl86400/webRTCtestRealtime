
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include "signal_processing_library.h"
#include "noise_suppression_x.h"
#include "noise_suppression.h"
#include "gain_control.h"
#include "echo_cancellation.h"

void webRtcNsProc(short * pData, FILE *outfilenameNs,int frames,short * pOutData,int* filter_state1,int* filter_state12,int* Synthesis_state1,int* Synthesis_state12)
{
	fprintf(stderr,"NS pData data: %d ... %d \n",*pData,*(pData+frames-1));
	NsHandle *pNS_inst = NULL;
	int nMode = 1;
	int len = frames*2;
	int fs = 16000;	


	if (0 != WebRtcNs_Create(&pNS_inst))
	{
		printf("Noise_Suppression WebRtcNs_Create err! \n");
	}

	if (0 !=  WebRtcNs_Init(pNS_inst,fs))
	{
		printf("Noise_Suppression WebRtcNs_Init err! \n");
	}

	if (0 !=  WebRtcNs_set_policy(pNS_inst,nMode))
	{
		printf("Noise_Suppression WebRtcNs_set_policy err! \n");
	}

		short shInL[160],shInH[160];
		short shOutL[160] = {0},shOutH[160] = {0};

		//fprintf(stderr,"NS shuBufferIn[] data: %d ... %d \n",shuBufferIn[0],shuBufferIn[79]);
		//首先需要使用滤波函数将音频数据分高低频，以高频和低频的方式传入降噪函数内部
		WebRtcSpl_AnalysisQMF(pData,frames,shInL,shInH,filter_state1,filter_state12);

		//将需要降噪的数据以高频和低频传入对应接口，同时需要注意返回数据也是分高频和低频
		if (0 == WebRtcNs_Process(pNS_inst ,shInL ,shInH ,shOutL , shOutH))
		{
			short shBufferOut[320];
			//如果降噪成功，则根据降噪后高频和低频数据传入滤波接口，然后用将返回的数据写入文件
			WebRtcSpl_SynthesisQMF(shOutL,shOutH,160,shBufferOut,Synthesis_state1,Synthesis_state12);
			memcpy(pOutData,shBufferOut,frames*sizeof(short));
		}
	
		if (NULL == outfilenameNs)
		{
			printf("open NS out file err! \n");
		}
		fwrite(pOutData, 1, len, outfilenameNs);
		printf("ns outfilenameNs: %d \n",*outfilenameNs);
		WebRtcNs_Free(pNS_inst);
}

void WebRtcAgcProc(short * pData, FILE * outfilename,int frames,short * pOutData)
{

	void *agcHandle = NULL;	
	int fs = 16000;
	WebRtcAgc_Create(&agcHandle);

	int minLevel = 0;
	int maxLevel = 255;
	int agcMode  = 3;	// 3 - Fixed Digital Gain 0dB
	WebRtcAgc_Init(agcHandle, minLevel, maxLevel, agcMode, fs);

	WebRtcAgc_config_t agcConfig;
	agcConfig.compressionGaindB = 20;
	agcConfig.limiterEnable     = 1;
	agcConfig.targetLevelDbfs   = 3;
	WebRtcAgc_set_config(agcHandle, agcConfig);

	int len = frames*sizeof(short);		//  len=2*frames
	int micLevelIn = 0;
	int micLevelOut = 0;

	// memset(pData, 0, len);
	fprintf(stderr,"AGC pDataFinal data: %d ... %d\n",*pData,*(pData+frames-1));

	int inMicLevel  = micLevelOut;
	int outMicLevel = 0;
	uint8_t saturationWarning;
	int nAgcRet = WebRtcAgc_Process(agcHandle, pData, NULL, frames, pOutData,NULL, inMicLevel, &outMicLevel, 0, &saturationWarning);

	if (nAgcRet != 0)
	{
		printf("failed in WebRtcAgc_Process %d \n",nAgcRet);
	}
	micLevelIn = outMicLevel;
	fwrite(pOutData, 1, len, outfilename);

	WebRtcAgc_Free(agcHandle);
}

void WebRtcAecProc(short *near_frame,short* far_frame,FILE * fp_out,int frames,short * out_frame)
{

	int fs = 16000;
	int len = frames*sizeof(short);
	void *aecmInst = NULL;

	WebRtcAec_Create(&aecmInst);
	WebRtcAec_Init(aecmInst, fs, fs);

	AecConfig config;
	config.nlpMode = 0;
	WebRtcAec_set_config(aecmInst, config);

	WebRtcAec_BufferFarend(aecmInst, far_frame, frames);//对参考声音(回声)的处理
	WebRtcAec_Process(aecmInst, near_frame, NULL, out_frame, NULL, frames,40,0);//回声消除

	fwrite(out_frame, 1, len, fp_out);

	WebRtcAec_Free(aecmInst);

}

int main()
{
   long loops;		//一个长整型变量， 
   int rc,rc1,rc2,rc3,rc4,rc5,rc6;		//一个int变量 ,用来存放 snd_pcm_open（访问硬件）的返回值 
   int size;		//一个int变量 
   snd_pcm_t * handle;		// 一个指向snd_pcm_t的指针 
   snd_pcm_hw_params_t * params;	// 一个指向 snd_pcm_hw_params_t的指针 
   unsigned int val;		// 无符号整型变量 ，用来存放录音时候的采样率 
   int dir;			// 整型变量 
   snd_pcm_uframes_t frames;		// snd_pcm_uframes_t 型变量 
   short * buffer = NULL;		// 一个字符型指针 
   short * buffertemp1 = NULL;		// 一个临时字符型指针
   short * buffertemp2 = NULL;		// 一个临时字符型指针
   short * buffertemp3 = NULL;		// 一个临时字符型指针
   short * buffertemp4 = NULL;		// 一个临时字符型指针
   short * buffertemp5 = NULL;		// 一个临时字符型指针
   short * buffertempmicin = NULL;		// 一个临时字符型指针
   short * buffertempspeaker = NULL;		// 一个临时字符型指针
   short * bufferAgcOutData = NULL;	// 指向Agc输出数据地址的指针
   short * bufferNsOutData = NULL;	// 指向NS输出数据地址的指针
   short * bufferAecOutData = NULL;	// 指向NS输出数据地址的指针
   short * bufferAecMicinData = NULL;	// 指向NS输出数据地址的指针
   short * bufferAecSpeakerData = NULL;	// 指向NS输出数据地址的指针

   int  filter_state1[6],filter_state12[6];
   int  Synthesis_state1[6],Synthesis_state12[6];
   memset(filter_state1,0,sizeof(filter_state1));
   memset(filter_state12,0,sizeof(filter_state12));
   memset(Synthesis_state1,0,sizeof(Synthesis_state1));
   memset(Synthesis_state12,0,sizeof(Synthesis_state12));
   FILE * out_fd1,*out_fd2,*out_fd3,*out_fd4,*out_fd5,*out_fd6,*out_fdAgc,*out_fdNs,*out_fdAec;		// 一个指向文件的指针 
   out_fd1 = fopen("out_pcm1.raw","wb+");
   out_fd2 = fopen("out_pcm2.raw","wb+");
   out_fd3 = fopen("out_pcm3.raw","wb+");
   out_fd4 = fopen("out_pcm4.raw","wb+");
   out_fd5 = fopen("out_pcm5.raw","wb+");
   out_fd6 = fopen("out_pcm6.raw","wb+");
   out_fdAgc = fopen("out_pcmAgc.raw","wb+");
   out_fdNs = fopen("out_pcmNs.raw","wb+");
   out_fdAec = fopen("out_pcmAec.raw","wb+");		
/* 将流与文件之间的关系建立起来，文件名为 out_pcm.raw，w是以文本方式打开文件，wb是二进制方式打开文件wb+ 读写打开或建立一个二进制文件，允许读和写。*/ 
   /* open PCM device for recording (capture). */
   // 访问硬件，并判断硬件是否访问成功 
   rc = snd_pcm_open(&handle, "default",SND_PCM_STREAM_CAPTURE,0);
   if( rc < 0 )
   {
      fprintf(stderr,"unable to open pcm device: %s\n",
              snd_strerror(rc));
      exit(1);
   }
   /* allocate a hardware parameters object */
   // 分配一个硬件变量对象 
   snd_pcm_hw_params_alloca(&params);
   /* fill it with default values. */
   // 按照默认设置对硬件对象进行设置 
   snd_pcm_hw_params_any(handle,params);
   /* set the desired hardware parameters */
   /* interleaved mode 设置数据为交叉模式*/
   snd_pcm_hw_params_set_access(handle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
   /* signed 16-bit little-endian format */
   // 设置数据编码格式为PCM、有符号、16bit、LE格式 
   snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_LE);

   // 设置声道数量 
   snd_pcm_hw_params_set_channels(handle,params,6);
   /* sampling rate */
   // 设置采样率 
   val = 16000;
   snd_pcm_hw_params_set_rate_near(handle,params,&val,&dir);

   /* set period size */
   // 周期长度（帧数） 
   frames = 320;
   snd_pcm_hw_params_set_period_size_near(handle,params,&frames,&dir);
   /* write parameters to the driver */
   // 将配置写入驱动程序中
   // 判断是否已经配置正确 
   rc = snd_pcm_hw_params(handle,params);
   if ( rc < 0 )
   {
       fprintf(stderr,"unable to set hw parameters: %s\n",snd_strerror(rc));
       exit(1);
   }
   /* use a buffer large enough to hold one period */
   // 配置一个缓冲区用来缓冲数据，缓冲区要足够大，此处看意思应该是只配置了
   // 够两个声道用的缓冲内存 
   snd_pcm_hw_params_get_period_size(params,&frames,&dir);
   size = frames * 12; /* 2 bytes/sample, 2channels */
   buffer = ( short * ) malloc(size);
   buffertemp4 = ( short * )malloc(frames*sizeof(short));
   bufferAgcOutData = ( short * )malloc(frames*sizeof(short));
   bufferNsOutData = ( short * )malloc(frames*sizeof(short));
   bufferAecOutData = ( short * )malloc(frames*sizeof(short));
   bufferAecMicinData = ( short * )malloc(frames*sizeof(short));
   bufferAecSpeakerData = ( short * )malloc(frames*sizeof(short));

   // 记录声音的长度，单位uS 
   snd_pcm_hw_params_get_period_time(params, &val, &dir);
   loops = 3000000 / val;
   while( loops > 0 )
   {
       loops--;
       rc = snd_pcm_readi(handle,buffer,frames);		// 读取录音数据
       if ( rc == -EPIPE )
       {
          /* EPIPE means overrun */
          fprintf(stderr,"overrun occured\n");
          snd_pcm_prepare(handle);
       }
       else if ( rc < 0 )
       {
          fprintf(stderr,"error from read: %s\n",snd_strerror(rc));
       }
       else if ( rc != (int)frames)
       {
          fprintf(stderr,"short read, read %d frames\n",rc);
       }

       // 将音频数据写入文件，把buffer中的数据写入到out-fd中
	buffertemp1 = buffer;
	buffertemp2 = buffer;
	buffertemp5 = buffertemp4;
	buffertempmicin = bufferAecMicinData;
	buffertempspeaker = bufferAecSpeakerData;

	fprintf(stderr,"buffertemp4 adress: %d \n",buffertemp4);
	fprintf(stderr,"buffertemp5pre adress: %d \n",buffertemp5);

	int loopfor;
	for(loopfor = 1;loopfor <= frames;loopfor++)
	    {
		fprintf(stderr,"loopfor data:  %d \n",loopfor);
		int loopwhile = 6;
		fprintf(stderr,"buffertemp2 data: %d %d %d %d %d %d\n",*buffertemp2,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++);
		
		buffertemp2++;

		buffertemp3 = buffertemp1;
		rc1 = fwrite(buffertemp3, 1, 2, out_fd1);

		buffertemp3 = buffertemp3 + 1;
		rc2 = fwrite(buffertemp3, 1, 2, out_fd2);
		*buffertemp5 = *buffertemp3;
		buffertemp5++;

		buffertemp3 = buffertemp3 + 1;
		rc3 = fwrite(buffertemp3, 1, 2, out_fd3);
		*buffertempmicin = *buffertemp3;
		fprintf(stderr,"buffertempmicin data: %d %d %d \n",*(buffertempmicin-2),*(buffertempmicin-1),*buffertempmicin);
	 	fprintf(stderr,"buffertempmicinframesin adress: %d \n",buffertempmicin);
		buffertempmicin++;

		buffertemp3 = buffertemp3 + 1;
		rc4 = fwrite(buffertemp3, 1, 2, out_fd4);

		buffertemp3 = buffertemp3 + 1;
		rc5 = fwrite(buffertemp3, 1, 2, out_fd5);

		buffertemp3 = buffertemp3 + 1;
		rc6 = fwrite(buffertemp3, 1, 2, out_fd6);
		*buffertempspeaker = *buffertemp3;
		fprintf(stderr,"buffertempspeaker data: %d %d %d \n",*(buffertempspeaker-2),*(buffertempspeaker-1),*buffertempspeaker);
	 	fprintf(stderr,"buffertempspeakerframesin adress: %d \n",buffertempspeaker);
		buffertempspeaker++;

		buffertemp1 = buffertemp1 + 6;
	    }
	    fprintf(stderr,"buffertemp4 data: %d %d %d ... %d %d %d \n",*buffertemp4,*(buffertemp4+1),*(buffertemp4+2),*(buffertemp4+(frames-3)),*(buffertemp4+frames-2),*(buffertemp4+frames-1));
	    printf("agc outfilename: %d \n",*out_fdAgc);

	    WebRtcAgcProc(buffertemp4,out_fdAgc,frames,bufferAgcOutData);
	    fprintf(stderr,"bufferAgcOutData adress: %d \n",bufferAgcOutData);
	    fprintf(stderr,"buffertemp4 adress: %d \n",buffertemp4);
	    fprintf(stderr,"buffertemp5framesout adress: %d \n",buffertemp5);
	    printf("ns outfilename: %d \n",*out_fdNs);

	    WebRtcAecProc(buffertempmicin,buffertempspeaker,out_fdAec,frames,bufferAecOutData);
	    fprintf(stderr,"bufferAecOutData adress: %d \n",bufferAecOutData);
	    printf("ns outfilename: %d \n",*out_fdAec);

	    webRtcNsProc(bufferAgcOutData,out_fdNs,frames,bufferNsOutData,filter_state1,filter_state12,Synthesis_state1,Synthesis_state12);
	    printf("int:%d\n",sizeof(int));
	    printf("int16_t:%d\n",sizeof(int16_t));
	    printf("int:%\n");
	    fprintf(stderr,"bufferNsOutData adress: %d \n \n",bufferNsOutData);	
       // rc = write(1, buffer, size);
       // if ( rc != size )
       if ( rc != frames )
        {
            fprintf(stderr,"short write: wrote %d bytes\n \n",rc);
        }
   }
   snd_pcm_drain(handle);
   snd_pcm_close(handle);
   free(buffer);
   free(buffertemp1);
   free(buffertemp2);
   free(buffertemp3);
   free(buffertempmicin);
   free(buffertempspeaker);
   free(bufferAgcOutData);
   free(bufferNsOutData);
   free(bufferAecOutData);
   free(bufferAecMicinData);
   free(bufferAecSpeakerData);
   fclose(out_fd1);
   fclose(out_fd2);
   fclose(out_fd3);
   fclose(out_fd4);
   fclose(out_fd5);
   fclose(out_fd6);
   fclose(out_fdAgc);
   fclose(out_fdNs);
   fclose(out_fdAec);
}
