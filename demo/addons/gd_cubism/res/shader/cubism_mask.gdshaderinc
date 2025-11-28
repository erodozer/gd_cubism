// masking
instance uniform bool has_mask = false;
instance uniform bool invert;
uniform sampler2D tex_mask : filter_linear_mipmap, repeat_disable;
instance uniform vec2 canvas_size = vec2(2,2);
instance uniform vec4 mask_rect = vec4(0,0,1,1);
instance uniform vec4 mask_bounds = vec4(0,0,1,1);
instance uniform vec4 layout_rect = vec4(0,0,1,1);
instance uniform vec4 channel: source_color = vec4(0,0,0,1); // used by masking to know which dimension to pull from

varying vec2 MASK_UV;
varying vec4 modulate;

bool discard_mask() {
	if(has_mask) {
		if (MASK_UV.x > 1.0 || MASK_UV.y > 1.0 || MASK_UV.x < 0.0 || MASK_UV.y < 0.0) {
			return true;
		}
	}
	return false;
}

vec2 mask_uv(vec2 vtx) {
	if (!has_mask) {
		return vec2(0,0);
	}

	vec2 tex_size = vec2(textureSize(tex_mask, 0));
    vec2 layout_ratio = mask_bounds.zw;
	vec2 uv_offset = mask_bounds.xy;
	vec2 mask_vtx = (vtx - mask_rect.xy) / mask_rect.zw;
	if (mask_vtx.x < 0.0 || mask_vtx.x > 1.0 || mask_vtx.y < 0.0 || mask_vtx.y > 1.0) {
		return vec2(-1.0, -1.0); // throw out of bounds to dispose pixel
	}
	vec2 mask_ratio = min(mask_rect.zw / (tex_size * layout_ratio), vec2(1.0, 1.0)) * layout_ratio;
	vec2 uv = uv_offset + (mask_vtx * layout_ratio);
    return uv;
}

vec4 sample_mask(vec4 color_tex, vec4 base) {
    vec4 color_for_mask = color_tex * base;
    color_for_mask.rgb = color_for_mask.rgb * color_for_mask.a;

    vec4 clip_mask = texture(tex_mask, MASK_UV);

    float mask_val = clip_mask.r + clip_mask.g + clip_mask.b + clip_mask.a;
    if (invert) {
        mask_val = 1.0 - mask_val;
    }
    return color_for_mask * mask_val;
}

void vertex() {
	MASK_UV = mask_uv(VERTEX.xy);
	modulate = COLOR;
}