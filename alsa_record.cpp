
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include "signal_processing_library.h"
#include "noise_suppression_x.h"
#include "noise_suppression.h"
#include "gain_control.h"

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
		//������Ҫʹ���˲���������Ƶ���ݷָߵ�Ƶ���Ը�Ƶ�͵�Ƶ�ķ�ʽ���뽵�뺯���ڲ�
		WebRtcSpl_AnalysisQMF(pData,frames,shInL,shInH,filter_state1,filter_state12);

		//����Ҫ����������Ը�Ƶ�͵�Ƶ�����Ӧ�ӿڣ�ͬʱ��Ҫע�ⷵ������Ҳ�Ƿָ�Ƶ�͵�Ƶ
		if (0 == WebRtcNs_Process(pNS_inst ,shInL ,shInH ,shOutL , shOutH))
		{
			short shBufferOut[320];
			//�������ɹ�������ݽ�����Ƶ�͵�Ƶ���ݴ����˲��ӿڣ�Ȼ���ý����ص�����д���ļ�
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

	fprintf(stderr,"AGC pData data: %d ... %d \n",*pData,*(pData+frames-1));
	void *agcHandle = NULL;	
	fprintf(stderr,"AGC pOutDataInit data: %d ... %d\n",*pOutData,*(pOutData+frames-1));
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
	fprintf(stderr,"AGC pOutDataFinal data: %d ... %d\n",*pOutData,*(pOutData+frames-1));
	fprintf(stderr,"AGC pOutDataFinal adress: %d \n",pOutData);
	if (nAgcRet != 0)
	{
		printf("failed in WebRtcAgc_Process %d \n",nAgcRet);
	}
	micLevelIn = outMicLevel;
	fwrite(pOutData, 1, len, outfilename);

	WebRtcAgc_Free(agcHandle);
}


int main()
{
   long loops;		//һ�������ͱ����� 
   int rc,rc1,rc2,rc3,rc4,rc5,rc6;		//һ��int���� ,������� snd_pcm_open������Ӳ�����ķ���ֵ 
   int size;		//һ��int���� 
   snd_pcm_t * handle;		// һ��ָ��snd_pcm_t��ָ�� 
   snd_pcm_hw_params_t * params;	// һ��ָ�� snd_pcm_hw_params_t��ָ�� 
   unsigned int val;		// �޷������ͱ��� ���������¼��ʱ��Ĳ����� 
   int dir;			// ���ͱ��� 
   snd_pcm_uframes_t frames;		// snd_pcm_uframes_t �ͱ��� 
   short * buffer = NULL;		// һ���ַ���ָ�� 
   short * buffertemp1 = NULL;		// һ����ʱ�ַ���ָ��
   short * buffertemp2 = NULL;		// һ����ʱ�ַ���ָ��
   short * buffertemp3 = NULL;		// һ����ʱ�ַ���ָ��
   short * buffertemp4 = NULL;		// һ����ʱ�ַ���ָ��
   short * buffertemp5 = NULL;		// һ����ʱ�ַ���ָ��
   short * bufferAgcOutData = NULL;	// ָ��Agc������ݵ�ַ��ָ��
   short * bufferNsOutData = NULL;	// ָ��NS������ݵ�ַ��ָ��

   int  filter_state1[6],filter_state12[6];
   int  Synthesis_state1[6],Synthesis_state12[6];
   memset(filter_state1,0,sizeof(filter_state1));
   memset(filter_state12,0,sizeof(filter_state12));
   memset(Synthesis_state1,0,sizeof(Synthesis_state1));
   memset(Synthesis_state12,0,sizeof(Synthesis_state12));
   FILE * out_fd1,*out_fd2,*out_fd3,*out_fd4,*out_fd5,*out_fd6,*out_fdAgc,*out_fdNs;		// һ��ָ���ļ���ָ�� 
   out_fd1 = fopen("out_pcm1.raw","wb+");
   out_fd2 = fopen("out_pcm2.raw","wb+");
   out_fd3 = fopen("out_pcm3.raw","wb+");
   out_fd4 = fopen("out_pcm4.raw","wb+");
   out_fd5 = fopen("out_pcm5.raw","wb+");
   out_fd6 = fopen("out_pcm6.raw","wb+");
   out_fdAgc = fopen("out_pcmAgc.raw","wb+");
   out_fdNs = fopen("out_pcmNs.raw","wb+");
   // out_fdAec = fopen("out_pcmAec.raw","wb+");		
/* �������ļ�֮��Ĺ�ϵ�����������ļ���Ϊ out_pcm.raw��w�����ı���ʽ���ļ���wb�Ƕ����Ʒ�ʽ���ļ�wb+ ��д�򿪻���һ���������ļ����������д��*/ 
   /* open PCM device for recording (capture). */
   // ����Ӳ�������ж�Ӳ���Ƿ���ʳɹ� 
   rc = snd_pcm_open(&handle, "default",SND_PCM_STREAM_CAPTURE,0);
   if( rc < 0 )
   {
      fprintf(stderr,"unable to open pcm device: %s\n",
              snd_strerror(rc));
      exit(1);
   }
   /* allocate a hardware parameters object */
   // ����һ��Ӳ���������� 
   snd_pcm_hw_params_alloca(&params);
   /* fill it with default values. */
   // ����Ĭ�����ö�Ӳ������������� 
   snd_pcm_hw_params_any(handle,params);
   /* set the desired hardware parameters */
   /* interleaved mode ��������Ϊ����ģʽ*/
   snd_pcm_hw_params_set_access(handle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
   /* signed 16-bit little-endian format */
   // �������ݱ����ʽΪPCM���з��š�16bit��LE��ʽ 
   snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_LE);

   // ������������ 
   snd_pcm_hw_params_set_channels(handle,params,6);
   /* sampling rate */
   // ���ò����� 
   val = 16000;
   snd_pcm_hw_params_set_rate_near(handle,params,&val,&dir);

   /* set period size */
   // ���ڳ��ȣ�֡���� 
   frames = 320;
   snd_pcm_hw_params_set_period_size_near(handle,params,&frames,&dir);
   /* write parameters to the driver */
   // ������д������������
   // �ж��Ƿ��Ѿ�������ȷ 
   rc = snd_pcm_hw_params(handle,params);
   if ( rc < 0 )
   {
       fprintf(stderr,"unable to set hw parameters: %s\n",snd_strerror(rc));
       exit(1);
   }
   /* use a buffer large enough to hold one period */
   // ����һ�������������������ݣ�������Ҫ�㹻�󣬴˴�����˼Ӧ����ֻ������
   // �����������õĻ����ڴ� 
   snd_pcm_hw_params_get_period_size(params,&frames,&dir);
   size = frames * 12; /* 2 bytes/sample, 2channels */
   buffer = ( short * ) malloc(size);
   buffertemp4 = ( short * )malloc(frames*sizeof(short));
   bufferAgcOutData = ( short * )malloc(frames*sizeof(short));
   bufferNsOutData = ( short * )malloc(frames*sizeof(short));

   // ��¼�����ĳ��ȣ���λuS 
   snd_pcm_hw_params_get_period_time(params, &val, &dir);
   loops = 3000000 / val;
   while( loops > 0 )
   {
       loops--;
       rc = snd_pcm_readi(handle,buffer,frames);		// ��ȡ¼������
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
       // ����Ƶ����д���ļ�����buffer�е�����д�뵽out-fd��
	buffertemp1 = buffer;
	buffertemp2 = buffer;
	buffertemp5 = buffertemp4;
	fprintf(stderr,"buffertemp4 adress: %d \n",buffertemp4);
	fprintf(stderr,"buffertemp5pre adress: %d \n",buffertemp5);
	int loopfor;
	for(loopfor = 1;loopfor <= frames;loopfor++)
	    {
		fprintf(stderr,"loopfor data:  %d \n",loopfor);
		int loopwhile = 6;
		fprintf(stderr,"buffertemp2 data: %d %d %d %d %d %d\n",*buffertemp2,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++);
		// fprintf(stderr,"buffertemp2 data: %d %d %d %d %d %d %d %d %d %d %d %d\n",*buffertemp2,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++,*buffertemp2++);
		
		buffertemp2++;

		fprintf(stderr,"buffertemp1 data: %d \n",*buffertemp1);
		buffertemp3 = buffertemp1;
		rc1 = fwrite(buffertemp3, 1, 2, out_fd1);
		buffertemp3 = buffertemp3 + 1;
		rc2 = fwrite(buffertemp3, 1, 2, out_fd2);
		*buffertemp5 = *buffertemp3;
		fprintf(stderr,"buffertemp5 data: %d %d %d \n",*(buffertemp5-2),*(buffertemp5-1),*buffertemp5);
	 	fprintf(stderr,"buffertemp5framesin adress: %d \n",buffertemp5);
		buffertemp5++;
		buffertemp3 = buffertemp3 + 1;
		rc3 = fwrite(buffertemp3, 1, 2, out_fd3);
		buffertemp3 = buffertemp3 + 1;
		rc4 = fwrite(buffertemp3, 1, 2, out_fd4);
		buffertemp3 = buffertemp3 + 1;
		rc5 = fwrite(buffertemp3, 1, 2, out_fd5);
		buffertemp3 = buffertemp3 + 1;
		rc6 = fwrite(buffertemp3, 1, 2, out_fd6);
		buffertemp1 = buffertemp1 + 6;
       		// rc = fwrite(buffer, 1, size, out_fd);
	    }
	    fprintf(stderr,"buffertemp4 data: %d %d %d ... %d %d %d \n",*buffertemp4,*(buffertemp4+1),*(buffertemp4+2),*(buffertemp4+(frames-3)),*(buffertemp4+frames-2),*(buffertemp4+frames-1));
	    printf("agc outfilename: %d \n",*out_fdAgc);
	    WebRtcAgcProc(buffertemp4,out_fdAgc,frames,bufferAgcOutData);
	    fprintf(stderr,"bufferAgcOutData adress: %d \n",bufferAgcOutData);
	    fprintf(stderr,"buffertemp4 adress: %d \n",buffertemp4);
	    fprintf(stderr,"buffertemp5framesout adress: %d \n",buffertemp5);
	    printf("ns outfilename: %d \n",*out_fdNs);
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
   free(bufferAgcOutData);
   fclose(out_fd1);
   fclose(out_fd2);
   fclose(out_fd3);
   fclose(out_fd4);
   fclose(out_fd5);
   fclose(out_fd6);
   fclose(out_fdAgc);
}
