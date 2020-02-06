/**
 * xcb calls
 *
 * Copyright 2020 Jay Sorg <jay.sorg@gmail.com>
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

#ifndef _WTV_XCB_H
#define _WTV_XCB_H

#ifdef __cplusplus
extern "C"
{
#endif

int
wtv_fd_to_drawable(struct wtv_info* wtv, int fd, int fd_width, int fd_height, int fd_stride, int fd_size, int fd_bpp);

#ifdef __cplusplus
}
#endif

#endif

