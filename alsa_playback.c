/*
read standard from input and writes to the default PCM device for 5
seconds of data
*/

/* use the newer ALSA API */

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

int main()
{
    long loops;
    int rc;
    int size;
    snd_pcm_t * handle;
    snd_pcm_hw_params_t * params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;
    char * buffer;
    int fd;
    fd=open("out_pcm.raw",O_RDONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);  
    /* open PCM device for playback */
    rc = snd_pcm_open(&handle, "default",
                      SND_PCM_STREAM_PLAYBACK,0);
    if( rc < 0 )
    {
        fprintf(stderr,"unable to open pcm device: %s\n", snd_strerror(rc));
        exit(0);
    }
    
    /* allocate a hardware parameters object */
    snd_pcm_hw_params_alloca(&params);
    /* fill it in with default parameters */
    snd_pcm_hw_params_any(handle,params);
    /* set the desired hardware parameters */
    /* interleaved mode */
    snd_pcm_hw_params_set_access(handle,params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);
    /* signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle,params,
                                 SND_PCM_FORMAT_S16_LE);
    /* two channels (stereo) */
    snd_pcm_hw_params_set_channels(handle,params,2);
    /* 44100Hz sampling rate */
    val = 44100;
    snd_pcm_hw_params_set_rate_near(handle,params,&val,&dir);
    /* set period size */
    frames = 32;
    snd_pcm_hw_params_set_period_size_near(handle,params,&frames,&dir);
    /* write the parameters to the driver */
    rc = snd_pcm_hw_params(handle,params);
    if( rc < 0 )
    {
        fprintf(stderr,
                "unable to set hw parameters: %s\n",
                snd_strerror(rc));
        exit(1);
    }
    /* use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params,&frames,&dir);
    size = frames * 4; /* 2 bytes * 2 channels */
    buffer = (char*)malloc(size);
    /* lop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params,&val,&dir);
    /* period time */
    loops = 5000000 / val;
    printf("loops:%ld\n",loops);
    printf("fd:%d\n",fd);
    while( loops > 0 ) // 循环录音
    {
        loops--;
        rc = read(0,buffer,size);
        //printf("%d = read(fd,buffer,size)\n",rc);
        if( rc == 0 ) // 未读取到数据
        {
            fprintf(stderr,"end of file on input\n");
            break;
        }
        else if ( rc != size )  // 实际读取数据偏少
        {
            fprintf(stderr,"short read: read %d bytes \n",rc);
        }   
        rc = snd_pcm_writei(handle,buffer,frames); // 写入声卡
        //printf("snd_pcm_writei(handle,buffer,frames) = %d\n",rc);
        if( rc == -EPIPE )
        {
             /* EPIPE means underrun */
             fprintf(stderr,"underrun occurred\n");
             snd_pcm_prepare(handle);
        }
        else if ( rc < 0 )
        {
             fprintf(stderr,"error from writei: %s\n",snd_strerror(rc));
        }
        else if ( rc != (int)frames )
        {
             fprintf(stderr,"short write, write %d frames\n",rc);
        }
    }
    printf("loops:%ld\n",loops);
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    printf("alsa_playback test\n");
    return 0;
}
