// masking
instance uniform bool has_mask = false;
instance uniform bool invert;
uniform sampler2D tex_mask : filter_linear_mipmap, repeat_disable;
instance uniform vec4 mask_rect = vec4(0,0,1,1); // 1x pixel size rect of the target mask
instance uniform vec4 mask_bounds = vec4(0,0,1,1); // cell position and scale
instance uniform vec2 mask_scale = vec2(1,1); // size of the mask within its cell
instance uniform vec4 channel: source_color = vec4(0,0,0,1); // used by masking to know which dimension to pull from

varying float mask;

vec2 mask_uv(vec2 vtx) {
	if (!has_mask) {
		return vec2(0,0);
	}

	vec2 tex_size = vec2(textureSize(tex_mask, 0));
	// get cell dimensions
	vec2 layout_ratio = mask_bounds.zw;
	// get cell offset
	vec2 layout_offset = mask_bounds.xy;
	
	// reposition the vtx to the top-left pixel of the mask rect
	vec2 mask_vtx = vtx - mask_rect.xy;
	
	// scale the vtx to match to the dimensions within the mask texture
	vec2 mask_uv = (mask_vtx * mask_scale) / tex_size;
	
	// shift the vtx to the cell's offset
	return mask_uv + layout_offset;
}

float sample_mask(vec2 vtx) {
	vec2 uv = mask_uv(vtx);
	vec2 cell_tl = mask_bounds.xy;
	vec2 cell_br = mask_bounds.zw + cell_tl;
	// dispose pixel with UVs are outside of the mask bounds
	if (uv.x > cell_br.x || uv.y > cell_br.y || uv.x < cell_tl.x || uv.y < cell_tl.y) {
		return -1.0;
	}
    vec4 clip_mask = texture(tex_mask, uv) * channel;

	// godot annoyingly does premult alpha when add blending
	// so to write to each channel we have to have some alpha value > 0
    float mask_val = ceil(clip_mask.r) + ciel(clip_mask.g) + ciel(clip_mask.b) + floor(clip_mask.a);
    if (invert) {
        mask_val = 1.0 - mask_val;
    }
    return mask_val;
}

void vertex() {
	mask = sample_mask(VERTEX.xy);
}
