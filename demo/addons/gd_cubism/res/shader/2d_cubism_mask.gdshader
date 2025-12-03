shader_type canvas_item;
render_mode blend_add, unshaded;
// TODO switch to blend_max once implemented to fix alpha https://github.com/godotengine/godot/pull/48654
instance uniform vec4 channel: source_color;

void fragment() {
	vec4 t = texture(TEXTURE, UV);
    // BUG: godot premultiplies alpha in blend_add, so writing to individual channels doesn't work
	COLOR = channel * t.a;
}
