precision highp float;

uniform sampler2D fonts_texture;

uniform vec3 rgb;

varying vec2 st;

void main() {
	float alpha = texture2D(fonts_texture, st).a;
	gl_FragColor = vec4(rgb*alpha, alpha);
}

