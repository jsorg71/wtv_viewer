/**
 * pulseaudio calls
 *
 * Copyright 2015-2021 Jay Sorg <jay.sorg@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WTV_PA_H
#define _WTV_PA_H

#ifdef __cplusplus
extern "C"
{
#endif

#define CAP_PA_FORMAT_48000_1CH_16LE 1
#define CAP_PA_FORMAT_48000_2CH_16LE 2
#define CAP_PA_FORMAT_48000_6CH_16LE 6

int
wtv_pa_create(const char* name, void** handle);
int
wtv_pa_delete(void* handle);
int
wtv_pa_start(void* handle, const char* name, int ms_latency,
             int format);
int
wtv_pa_stop(void* handle);
int
wtv_pa_play(void* handle, void* data, int data_bytes);
int
wtv_pa_play_non_blocking(void* handle, void* data, int data_bytes,
                         int* data_bytes_processed);
int
wtv_pa_get_latency(void* handle, int* latency);
int
wtv_pa_set_volume(void* handle, int volume);

#ifdef __cplusplus
}
#endif

#endif

