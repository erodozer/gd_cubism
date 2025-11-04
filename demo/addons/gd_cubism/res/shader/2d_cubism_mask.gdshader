// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 MizunagiKB <mizukb@live.jp>
// GDCubism shader: Mask
shader_type canvas_item;
render_mode blend_mix, unshaded;

const vec4 channel = vec4(0,0,0,1);
uniform sampler2D tex_main[16] : filter_linear_mipmap, repeat_disable;
uniform sampler2D tex_main_nofilter[16] : filter_nearest_mipmap, repeat_disable;
instance uniform int tex_idx;
instance uniform bool filter = true;

void fragment() {
    vec4 tex;
    if (filter) {
        tex = texture(tex_main[tex_idx], UV);
    } else {
        tex = texture(tex_main_nofilter[tex_idx], UV);
    }
    COLOR = channel * tex.a;
}
