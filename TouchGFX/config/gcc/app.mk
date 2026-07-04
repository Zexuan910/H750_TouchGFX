
# Location of the TouchGFX framework used by Designer asset tools
touchgfx_path := ../../../../TouchGFX/4.26.1/touchgfx

# Location of the TouchGFX Environment
touchgfx_env := ../../../../TouchGFX/4.26.1/env
PATH := $(touchgfx_env)/MinGW/msys/1.0/Ruby30-x64/bin;$(PATH)
# Optional additional compiler flags
user_cflags := -DUSE_BPP=16
