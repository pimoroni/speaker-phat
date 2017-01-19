/*
 *   phatmeter :  level meter ALSA plugin for Raspberry Pi HATs and pHATs
 *   Copyright (c) 2017 by Phil Howard <phil@pimoroni.com>
 *
 *   Derived from:
 *   ameter :  level meter ALSA plugin with SDL display
 *   Copyright (c) 2005 by Laurent Georget <laugeo@free.fr>
 *   Copyright (c) 2001 by Abramo Bagnara <abramo@alsa-project.org>
 *   Copyright (c) 2002 by Steve Harris <steve@plugin.org.uk>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <alsa/asoundlib.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>


#define BAR_WIDTH 70
/* milliseconds to go from 32767 to 0 */
#define DECAY_MS 1000
/* milliseconds for peak to disappear */
#define PEAK_MS 800

#define LED_BRIGHTNESS 255

#define MAX_METERS 2

int stupid_led_mappings[10] = {0, 1, 2, 4, 6, 8, 10, 12, 14, 16};

typedef struct _snd_pcm_scope_ameter_channel {
  int16_t levelchan;
  int16_t peak;
  unsigned int peak_age;
  int16_t previous;
} snd_pcm_scope_ameter_channel_t;

typedef struct _snd_pcm_scope_ameter {
  snd_pcm_t *pcm;
  snd_pcm_scope_t *s16;
  snd_pcm_scope_ameter_channel_t *channels;
  snd_pcm_uframes_t old;
  int top;
  unsigned int bar_width;
  unsigned int decay_ms;
  unsigned int peak_ms;
  unsigned int led_brightness;
} snd_pcm_scope_ameter_t;

int i2c = 0;
int num_meters, num_scopes;
//unsigned int led_brightness = 255;

static int level_enable(snd_pcm_scope_t * scope)
{

  snd_pcm_scope_ameter_t *level =
    snd_pcm_scope_get_callback_private(scope);
  level->channels =
    calloc(snd_pcm_meter_get_channels(level->pcm),
	   sizeof(*level->channels));
  if (!level->channels) {
    if (level) free(level); 
    return -ENOMEM;
  }

  snd_pcm_scope_set_callback_private(scope, level);
  return (0);

}

static void level_disable(snd_pcm_scope_t * scope)
{
  snd_pcm_scope_ameter_t *level =
    snd_pcm_scope_get_callback_private(scope);
 
  if(level->channels) free(level->channels);
}

static void level_close(snd_pcm_scope_t * scope)
{

  snd_pcm_scope_ameter_t *level =
    snd_pcm_scope_get_callback_private(scope);
  if (level) free(level); 
}

static void level_start(snd_pcm_scope_t * scope ATTRIBUTE_UNUSED)
{
  sigset_t s;
  sigemptyset(&s);
  sigaddset(&s, SIGINT);
  pthread_sigmask(SIG_BLOCK, &s, NULL); 
}

static void level_stop(snd_pcm_scope_t * scope)
{
}

static void level_update(snd_pcm_scope_t * scope)
{
  snd_pcm_scope_ameter_t *level = snd_pcm_scope_get_callback_private(scope);
  snd_pcm_t *pcm = level->pcm;
  snd_pcm_sframes_t size;
  snd_pcm_uframes_t size1, size2;
  snd_pcm_uframes_t offset, cont;
  unsigned int c, channels;
  unsigned int ms;
  int max_decay, max_decay_temp;

  size = snd_pcm_meter_get_now(pcm) - level->old;
  if (size < 0){
    size += snd_pcm_meter_get_boundary(pcm);
  }

  offset = level->old % snd_pcm_meter_get_bufsize(pcm);
  cont = snd_pcm_meter_get_bufsize(pcm) - offset;
  size1 = size;
  if (size1 > cont){
    size1 = cont;
  }

  size2 = size - size1;
  ms = size * 1000 / snd_pcm_meter_get_rate(pcm);
  max_decay = 32768 * ms / level->decay_ms;

  /* max_decay_temp=max_decay; */
  channels = snd_pcm_meter_get_channels(pcm);

  int meter_level = 0; 
  int brightness = level->led_brightness;

  for (c = 0; c < channels; c++) {
    int16_t *ptr;
    int s, lev = 0;
    snd_pcm_uframes_t n;
    snd_pcm_scope_ameter_channel_t *l;
    l = &level->channels[c];
    ptr = snd_pcm_scope_s16_get_channel_buffer(level->s16, c) + offset;

    for (n = size1; n > 0; n--) {
      s = *ptr;
      if (s < 0)
	s = -s;
      if (s > lev)
	lev = s;
      ptr++;
    }

    ptr = snd_pcm_scope_s16_get_channel_buffer(level->s16, c);
    for (n = size2; n > 0; n--) {
      s = *ptr;
      if (s < 0)
	s = -s;
      if (s > lev)
	lev = s;
      ptr++;
    }

    /* limit the decay */
    if (lev < l->levelchan) {	  
      /* make max_decay go lower with level */
      max_decay_temp =
	max_decay / (32767 / (l->levelchan));
      lev = l->levelchan - max_decay_temp;
      max_decay_temp = max_decay;
    }

    l->levelchan = lev;

    if(lev > meter_level){
        meter_level = lev;
    }
    
    l->previous= lev;  
  }
    
    //printf("Level: %d\n", meter_level);

    //int bar = 1275.0f * log10(32768.0f / (lev + 320));
    int bar = (meter_level / 10000.0f) * (brightness * 10.0f);

    if(bar < 0) bar = 0;
    if(bar > (brightness*10)) bar = (brightness*10);
    //bar = 2550 - bar;


    int led;
    for(led = 0; led < 10; led++){
       int val = 0;

       if(bar > brightness){
           val = brightness;
           bar -= brightness;
       }
       else if(bar > 0){
       	   val = bar;
           bar = 0;
       }
       
       wiringPiI2CWriteReg8(i2c, 0x01 + stupid_led_mappings[led], val);
    }
    wiringPiI2CWriteReg8(i2c, 0x16, 0x01);


  


  level->old = snd_pcm_meter_get_now(pcm);

}

static void level_reset(snd_pcm_scope_t * scope)
{
  snd_pcm_scope_ameter_t *level = snd_pcm_scope_get_callback_private(scope);
  snd_pcm_t *pcm = level->pcm;
  memset(level->channels, 0, snd_pcm_meter_get_channels(pcm) * sizeof(*level->channels));
  level->old = snd_pcm_meter_get_now(pcm);
}

snd_pcm_scope_ops_t level_ops = {
  enable:level_enable,
  disable:level_disable,
  close:level_close,
  start:level_start,
  stop:level_stop,
  update:level_update,
  reset:level_reset,
};

int snd_pcm_scope_ameter_open(snd_pcm_t * pcm, const char *name,
			      unsigned int bar_width,
			      unsigned int decay_ms, unsigned int peak_ms,
                              unsigned int led_brightness,
			      snd_pcm_scope_t ** scopep)
{
  snd_pcm_scope_t *scope, *s16;
  snd_pcm_scope_ameter_t *level;
  int err = snd_pcm_scope_malloc(&scope);
  if (err < 0){
    return err;
  }
  level = calloc(1, sizeof(*level));
  if (!level) {
    if (scope) free(scope);
    return -ENOMEM;
  }
  level->pcm = pcm;
  level->bar_width = bar_width;
  level->decay_ms = decay_ms;
  level->peak_ms = peak_ms;
  level->led_brightness = led_brightness;
  s16 = snd_pcm_meter_search_scope(pcm, "s16");
  if (!s16) {
    err = snd_pcm_scope_s16_open(pcm, "s16", &s16);
    if (err < 0) {
      if (scope) free(scope);
      if (level)free(level);
      return err;
    }
  }
  level->s16 = s16;
  snd_pcm_scope_set_ops(scope, &level_ops);
  snd_pcm_scope_set_callback_private(scope, level);
  if (name){
    snd_pcm_scope_set_name(scope, strdup(name));
  }
  snd_pcm_meter_add_scope(pcm, scope);
  *scopep = scope;
  return 0;
}

void clear_display(void){
  int led;

  for(led = 0; led < 18; led++){
      wiringPiI2CWriteReg8(i2c, 0x01 + led, 0x0);
  }

  wiringPiI2CWriteReg8(i2c, 0x16, 0x01);
}

int _snd_pcm_scope_ameter_open(snd_pcm_t * pcm, const char *name,
			       snd_config_t * root, snd_config_t * conf)
{
  snd_config_iterator_t i, next;
  snd_pcm_scope_t *scope;
  long bar_width = -1, decay_ms = -1, peak_ms = -1, led_brightness = -1;
  int err;

  num_meters = MAX_METERS;
  num_scopes = MAX_METERS;

  //printf("Setting up i2c\n");

  i2c = wiringPiI2CSetup(0x54);
  if(i2c == -1){
    fprintf(stderr, "Unable to connect to Speaker pHAT");
    exit(1);
  }

  wiringPiI2CWriteReg8(i2c, 0x00, 0x01);
  wiringPiI2CWriteReg8(i2c, 0x13, 0xff);
  wiringPiI2CWriteReg8(i2c, 0x14, 0xff);
  wiringPiI2CWriteReg8(i2c, 0x15, 0xff);

  clear_display();

  atexit(clear_display);
 
  snd_config_for_each(i, next, conf) {
    snd_config_t *n = snd_config_iterator_entry(i);
    const char *id;
    if (snd_config_get_id(n, &id) < 0)
      continue;
    if (strcmp(id, "comment") == 0)
      continue;
    if (strcmp(id, "type") == 0)
      continue;
    /*if (strcmp(id, "bar_width") == 0) {
      err = snd_config_get_integer(n, &bar_width);
      if (err < 0) {
	SNDERR("Invalid type for %s", id);
	return -EINVAL;
      }
      continue;
    }*/
    if (strcmp(id, "brightness") == 0) {
      err = snd_config_get_integer(n, &led_brightness);
      if (err < 0) {
        SNDERR("Invalid type for %", id);
        return -EINVAL;
      }
      continue;
    }
    if (strcmp(id, "decay_ms") == 0) {
      err = snd_config_get_integer(n, &decay_ms);
      if (err < 0) {
	SNDERR("Invalid type for %s", id);
	return -EINVAL;
      }
      continue;
    }
    if (strcmp(id, "peak_ms") == 0) {
      err = snd_config_get_integer(n, &peak_ms);
      if (err < 0) {
	SNDERR("Invalid type for %s", id);
	return -EINVAL;
      }
      continue;
    }
    SNDERR("Unknown field %s", id);
    return -EINVAL;
  }

  if (decay_ms < 0)
    decay_ms = DECAY_MS;
  if (peak_ms < 0)
    peak_ms = PEAK_MS;
  if (led_brightness < 0)
    led_brightness = LED_BRIGHTNESS;

  return snd_pcm_scope_ameter_open(pcm, name, bar_width, decay_ms,
				   peak_ms, led_brightness, &scope);

}
