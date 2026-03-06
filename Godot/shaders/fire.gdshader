shader_type spatial;
render_mode blend_add, depth_draw_never, cull_disabled, unshaded;

uniform sampler2D fire_texture : source_color, hint_default_black;
uniform float scroll_speed : hint_range(0.0, 2.0) = 0.35;
uniform float distortion_strength : hint_range(0.0, 0.3) = 0.06;
uniform float distortion_speed : hint_range(0.0, 10.0) = 2.5;
uniform float brightness : hint_range(0.1, 5.0) = 2.5;
uniform float top_fade : hint_range(0.0, 1.0) = 0.55;
uniform bool flip_v = false;

void vertex() {
	// Spherical billboard — always faces the camera
	MODELVIEW_MATRIX = VIEW_MATRIX * mat4(
		INV_VIEW_MATRIX[0],
		INV_VIEW_MATRIX[1],
		INV_VIEW_MATRIX[2],
		MODEL_MATRIX[3]
	);
	POSITION = PROJECTION_MATRIX * MODELVIEW_MATRIX * vec4(VERTEX, 1.0);
}

void fragment() {
	float t = TIME;
	vec2 uv = UV;
	if (flip_v) uv.y = 1.0 - uv.y;

	// Multi-frequency horizontal wobble for organic flame motion
	float wobble = sin(uv.y * 5.0  + t * distortion_speed)        * distortion_strength
	             + sin(uv.y * 11.3 + t * distortion_speed * 1.8)  * distortion_strength * 0.4
	             + sin(uv.y * 19.7 + t * distortion_speed * 2.6)  * distortion_strength * 0.2;

	// Two layers scrolling at different speeds for depth / flickering
	vec2 uv1 = vec2(uv.x + wobble,        uv.y - t * scroll_speed);
	vec2 uv2 = vec2(uv.x - wobble * 0.6,  uv.y - t * scroll_speed * 0.65 + 0.45);

	vec3 fire1 = texture(fire_texture, uv1).rgb;
	vec3 fire2 = texture(fire_texture, uv2).rgb;
	vec3 fire  = fire1 * 0.65 + fire2 * 0.35;

	// Fade toward the tip of the flame
	float fade = 1.0 - smoothstep(top_fade, 1.0, uv.y);

	ALBEDO = fire * fade * brightness;
}
