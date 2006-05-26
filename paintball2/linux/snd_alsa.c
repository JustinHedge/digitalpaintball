/*
	snd_alsa.c

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

	$Id: snd_alsa.c,v 1.3 2006-05-26 05:53:40 jitspoe Exp $
*/

#include <alsa/asoundlib.h>

#include "../client/client.h"
#include "../client/snd_loc.h"

static int  snd_inited;

static snd_pcm_t *playback_handle;
static snd_pcm_hw_params_t *hw_params;

/*
* These are reasonable default values for good latency.  If ALSA
* playback stutters or plain does not work, try adjusting these.
* Period must always be a multiple of 2.  Buffer must always be
* a multiple of period.  See http://alsa-project.org.
*/
static snd_pcm_uframes_t period_size = 1024;
static snd_pcm_uframes_t buffer_size = 4096;

static int sample_bytes;
static int buffer_bytes;

cvar_t *sndbits;
cvar_t *sndspeed;
cvar_t *sndchannels;
cvar_t *snddevice;

static int tryrates[] = { 44100, 22050, 11025, 8000 };

qboolean SNDDMA_Init (void)
{
  int i;
  int err;
  int format;

  if (snd_inited){ // jitlinux
	return 1;
  }

  Com_Printf("Initializing ALSA.\n");
  
  sndbits = Cvar_Get("sndbits", "16", CVAR_ARCHIVE);
  sndspeed = Cvar_Get("sndspeed", "0", CVAR_ARCHIVE);
  sndchannels = Cvar_Get("sndchannels", "2", CVAR_ARCHIVE);
  snddevice = Cvar_Get("snddevice", "default", CVAR_ARCHIVE);
  
  err = snd_pcm_open(&playback_handle, snddevice->string, 
		     SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    Com_Printf("ALSA snd error, cannot open device %s (%s)\n", 
	       snddevice->string,
	       snd_strerror(err));
    return 0;
  }
  
  err = snd_pcm_hw_params_malloc(&hw_params);

  if (err < 0) {
    Com_Printf("ALSA snd error, cannot allocate hw params (%s)\n",
	       snd_strerror(err));
    return 0;
  }
  
  err = snd_pcm_hw_params_any (playback_handle, hw_params);
  if (err < 0) {
    Com_Printf("ALSA snd error, cannot init hw params (%s)\n",
	       snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }
  
  err = snd_pcm_hw_params_set_access(playback_handle, hw_params, 
				     SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) {
    Com_Printf("ALSA snd error, cannot set access (%s)\n",
	       snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }
  
  dma.samplebits = sndbits->value;
  if (dma.samplebits == 16 || dma.samplebits != 8) {
    err = snd_pcm_hw_params_set_format(playback_handle, hw_params,
				       SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
      Com_Printf("ALSA snd error, 16 bit sound not supported, trying 8\n");
      dma.samplebits = 8;
    }
    else {
      format = SND_PCM_FORMAT_S16_LE;
    }
  }
  if (dma.samplebits == 8) {
    err = snd_pcm_hw_params_set_format(playback_handle, hw_params,
				       SND_PCM_FORMAT_U8);
  }
  if (err < 0) {
    Com_Printf("ALSA snd error, cannot set sample format (%s)\n",
	       snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }
  else {
    format = SND_PCM_FORMAT_U8;
  }
  
  dma.speed = (int)sndspeed->value;
  if (!dma.speed) {
    for (i=0 ; i<sizeof(tryrates); i++) {
      int test = tryrates[i];
      err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params,
					    &test, 0);
      if (err < 0) {
	Com_Printf("ALSA snd error, cannot set sample rate %d (%s)\n",
		   tryrates[i], snd_strerror(err));
      }
      else {
	dma.speed = test;
	break;
      }
    }
  }
  if (!dma.speed) {
    Com_Printf("ALSA snd error couldn't set rate.\n");
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }

  dma.channels = (int)sndchannels->value;
  if (dma.channels < 1 || dma.channels > 2)
    dma.channels = 2;
  err = snd_pcm_hw_params_set_channels(playback_handle,hw_params,dma.channels);
  if (err < 0) {
    Com_Printf("ALSA snd error couldn't set channels %d (%s).\n",
	       dma.channels, snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }
  
  if((err = snd_pcm_hw_params_set_period_size_near(playback_handle, 
			hw_params, &period_size, 0)) < 0){
		Com_Printf("ALSA: cannot set period size near(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return 0;
	}
	
	if((err = snd_pcm_hw_params_set_buffer_size_near(playback_handle, 
			hw_params, &buffer_size)) < 0){
		Com_Printf("ALSA: cannot set buffer size near(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return 0;
	}
  
  err = snd_pcm_hw_params(playback_handle, hw_params);
  if (err < 0) {
    Com_Printf("ALSA snd error couldn't set params (%s).\n",snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }

  	sample_bytes = dma.samplebits / 8;
	buffer_bytes = buffer_size * dma.channels * sample_bytes;
	
	dma.buffer = malloc(buffer_bytes);  //allocate pcm frame buffer
	memset(dma.buffer, 0, buffer_bytes);
	
	dma.samples = buffer_size * dma.channels;
	dma.submission_chunk = period_size * dma.channels;
	
	dma.samplepos = 0;
	
	snd_pcm_prepare(playback_handle);
	snd_inited = 1;
	return 1;
}

int
SNDDMA_GetDMAPos (void)
{
  if(snd_inited)
    return dma.samplepos;
  else
    Com_Printf ("Sound not inizialized\n");
  return 0;
}

void
SNDDMA_Shutdown (void)
{
  if (snd_inited) {
    snd_pcm_drop(playback_handle);
    snd_pcm_close(playback_handle);
    snd_inited = 0;
  }
  free(dma.buffer);
  dma.buffer = NULL;
}

/*
  SNDDMA_Submit
Send sound to device if buffer isn't really the dma buffer
*/
void
SNDDMA_Submit (void)
{
  int s, w, frames;
	void *start;
	
	if(!snd_inited)
		return;
		
	s = dma.samplepos * sample_bytes;
	start = (void *) &dma.buffer[s];
	
	frames = dma.submission_chunk / dma.channels;
	
	if((w = snd_pcm_writei(playback_handle, start, frames)) < 0){  //write to card
		snd_pcm_prepare(playback_handle);  //xrun occured
		return;
	}
	
	dma.samplepos += w * dma.channels;  //mark progress
	
	if(dma.samplepos >= dma.samples)
		dma.samplepos = 0;  //wrap buffer
}


void SNDDMA_BeginPainting(void)
{    
}
