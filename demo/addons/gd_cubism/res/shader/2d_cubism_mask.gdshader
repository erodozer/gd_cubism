// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 MizunagiKB <mizukb@live.jp>
// GDCubism shader: Mask
shader_type canvas_item;
render_mode blend_mix, unshaded;

#include "cubism_sampler.gdshaderinc"
instance uniform vec4 channel: source_color = vec4(0,0,0,1); // used by masking to know which dimension to pull from

void fragment() {
    vec4 color_tex = SAMPLE_TEX;
    COLOR = channel * color_tex.a;
}
