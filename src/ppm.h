//
// Copyright (C) Wojciech Jarosz <wjarosz@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE.txt file.
//

#pragma once

bool is_ppm(const char * filename);
bool write_ppm(const char * filename, int width, int height, int numChannels, const unsigned char * data);
float * load_ppm(const char * filename, int * width, int * height, int * numChannels);
