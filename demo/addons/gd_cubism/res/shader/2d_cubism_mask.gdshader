// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 MizunagiKB <mizukb@live.jp>
// GDCubism shader: Mask
shader_type canvas_item;
render_mode blend_mix, unshaded;

const vec4 channel = vec4(0,0,0,1);
uniform sampler2D tex_main[16] : filter_linear_mipmap, repeat_disable;
instance uniform int tex_idx;

void fragment() {
    COLOR = channel * texture(tex_main[tex_idx], UV).a;
}
