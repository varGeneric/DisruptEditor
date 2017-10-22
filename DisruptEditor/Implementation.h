#pragma once

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

#include "debug_draw.hpp"
#include <stdio.h>
#include <string>
#include <algorithm>
#include <cctype>

#include "wluFile.h"
#include "xbgFile.h"
#include "DatFat.h"
#include "DominoBox.h"
#include "tinyfiles.h"
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include "Camera.h"
#include "noc_file_dialog.h"
#include "GLHelper.h"
#include <Shlwapi.h>

#include "ini.h"
