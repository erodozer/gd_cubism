// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 MizunagiKB <mizukb@live.jp>
// GDCubism shader: Mask
shader_type canvas_item;
render_mode blend_mix, unshaded;

#include "cubism_sampler.gdshaderinc"

void fragment() {
    vec4 color_tex = SAMPLE_TEX;
    COLOR = vec4(0,0,0,color_tex.a);
}
