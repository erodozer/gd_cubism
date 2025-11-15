instance uniform int tex_idx = 0;
instance uniform bool filter = true;

#define PICK_TEX(i, uv) filter ? texture(tex_main[i], uv) : texture(tex_main_nofilter[i], uv)

#if CURRENT_RENDERER == RENDERER_COMPATIBILITY
// GLES2 has a limit of 8 samplers in a shader
uniform sampler2D tex_main[8] : filter_linear_mipmap, repeat_disable;
uniform sampler2D tex_main_nofilter[8] : filter_nearest_mipmap, repeat_disable;

#define SAMPLE_TEX sample_tex(UV)

vec4 sample_tex(vec2 uv) {
	switch (tex_idx) {
		case 1: return PICK_TEX(1, uv);
		case 2: return PICK_TEX(2, uv);
		case 3: return PICK_TEX(3, uv);
		case 4: return PICK_TEX(4, uv);
		case 5: return PICK_TEX(5, uv);
		case 6: return PICK_TEX(6, uv);
		case 7: return PICK_TEX(7, uv);
		default:
			return PICK_TEX(0, uv);
	}
}

#else
uniform sampler2D tex_main[16] : filter_linear_mipmap, repeat_disable;
uniform sampler2D tex_main_nofilter[16] : filter_nearest_mipmap, repeat_disable;

#define SAMPLE_TEX PICK_TEX(tex_idx, UV)

#endif