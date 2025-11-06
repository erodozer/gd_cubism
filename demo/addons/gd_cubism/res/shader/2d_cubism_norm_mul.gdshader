// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 MizunagiKB <mizukb@live.jp>
// GDCubism shader: Mix
shader_type canvas_item;
render_mode blend_mul, unshaded;

const vec4 channel = vec4(0,0,0,1);

instance uniform vec4 color_base: source_color;
instance uniform vec4 color_screen: source_color;
instance uniform vec4 color_multiply: source_color;
instance uniform int tex_idx = 0;
uniform sampler2D tex_main[16] : filter_linear_mipmap, repeat_disable;
uniform sampler2D tex_main_nofilter[16] : filter_nearest_mipmap, repeat_disable;
varying vec4 modulate;
instance uniform bool filter = true;

// masking
instance uniform bool has_mask = false;
instance uniform bool invert;
uniform sampler2D tex_mask : filter_linear_mipmap, repeat_disable;
uniform float mask_scale = 1.0;
uniform vec2 canvas_size = vec2(2,2);
uniform vec2 mesh_offset = vec2(0,0);

varying vec2 MASK_UV;

void vertex() {
	if (has_mask) {
	    vec2 mask_size = vec2(textureSize(tex_mask, 0)) / mask_scale;
	    vec2 mask_vtx = VERTEX - mesh_offset;
	    MASK_UV = mask_vtx / mask_size;
	}
	modulate = COLOR;
}

void fragment() {
	if (has_mask) {
		if (MASK_UV.x > 1.0 || MASK_UV.y > 1.0 || MASK_UV.x < 0.0 || MASK_UV.y < 0.0) {
			discard;
		}
	}
	
    vec4 color_tex;
    if (filter) {
        color_tex = texture(tex_main[tex_idx], UV);
    } else {
        color_tex = texture(tex_main_nofilter[tex_idx], UV);
    }
    color_tex.rgb = color_tex.rgb * color_multiply.rgb;

    // premul alpha
    color_tex.rgb = color_tex.rgb + color_screen.rgb - (color_tex.rgb * color_screen.rgb);

	if (has_mask) {
	    vec4 color_for_mask = color_tex * color_base;
	    color_for_mask.rgb = color_for_mask.rgb * color_for_mask.a;

	    vec4 clip_mask = texture(tex_mask, MASK_UV) * channel;

	    float mask_val = clip_mask.r + clip_mask.g + clip_mask.b + clip_mask.a;
        if (invert) {
            mask_val = 1.0 - mask_val;
        }
	    color_for_mask = color_for_mask * mask_val;
        COLOR = vec4(
            color_for_mask.r + (1.0 - color_for_mask.a),
            color_for_mask.g + (1.0 - color_for_mask.a),
            color_for_mask.b + (1.0 - color_for_mask.a),
            1.0
        ) * modulate;
	} else {
		// premul alpha
	    vec4 color = color_tex * color_base * modulate;
        COLOR = vec4(color.rgb * color.a, color.a);
	}
}
