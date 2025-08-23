//------------------------------------------------------------------------------
//  Shader code for texcube-sapp sample.
//
//  NOTE: This source file also uses the '#pragma sokol' form of the
//  custom tags.
//------------------------------------------------------------------------------
@ctype mat4 mat4
@ctype vec3 vec3

//=== shadow pass
@vs vs_shadow
@glsl_options fixup_clipspace // important: map clipspace z from -1..+1 to 0..+1 on GL

layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;
layout(location=2) in vec2 a_uv;
layout(location=3) in float a_vertex_index;

layout(binding=0) uniform vs_shadow_params {
	mat4 light_view_mvp;
};

void main() {
    gl_Position = light_view_mvp * vec4(a_position.xyz, 1.0);
}
@end

@fs fs_shadow
void main() { }
@end

@program shadow vs_shadow fs_shadow

// == texture
#pragma sokol @vs texture_static_mesh_vs

layout(binding=0) uniform texture_static_mesh_vs_params {
	mat4 model;
	mat4 mvp;
};

layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;
layout(location=2) in vec2 a_uv;
layout(location=3) in float a_vertex_index;

/**
#if SOKOL_GLSL
    layout(location=4) in vec4 a_bone_indices;
#else
    layout(location=4) in ivec4 a_bone_indices;
#endif
layout(location=5) in vec4 a_bone_weights;
*/

out vec3 color;
out vec4 world_pos;
out vec3 world_norm; 
out vec2 uv;

void main() {
    gl_Position = mvp * vec4(a_position.xyz, 1.0);
	world_pos = model * vec4(a_position.xyz, 1.f);
    world_norm = normalize((model * vec4(a_normal, 0.0)).xyz);
    color = vec3(1.f, 1.f, 1.f);
	uv = a_uv;
}

#pragma sokol @end

#pragma sokol @fs texture_static_mesh_fs

const int NUM_CASCADES = 4;

layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;
layout(binding=1) uniform texture2DArray shadow_tex;
layout(binding=1) uniform sampler shadow_smp;

layout(binding=2) uniform texture_static_mesh_fs_params {
	mat4 view;
	mat4 light_view_proj[NUM_CASCADES];
	vec4 cascade_splits_shadow_map_size[NUM_CASCADES];
	// xy: shadow map size
	// zw: cascade splits
	vec3 light_ambient;
	vec3 light_color;
	vec3 light_dir;

    vec3 eye_pos;
};

in vec3 color;
in vec4 world_pos;
in vec3 world_norm;
in vec2 uv;

out vec4 frag_color;

// Returns cascade index given view-space depth (positive)
int choose_cascade(float view_depth) {
    for (int i = 0; i < NUM_CASCADES; ++i) {
		// using cascade part of cascade_splits_shadow_map
        if (view_depth <= cascade_splits_shadow_map_size[i].z / cascade_splits_shadow_map_size[i].w) {
			return i;
		}
    }
    return NUM_CASCADES;
}

const float depth_bias = 0.0008;
const float slope_bias = 0.0025;
const float pcf_radius = 1.0;

// 3x3 PCF over a texture array layer
float pcf_shadow_array(vec3 uvz, int cascadeIndex, vec2 texelSize, float depthBias) {
    // uvz.xy in [0,1], uvz.z = receiver depth in light clip [0,1]
    // Early reject outside light frustum
    if (uvz.x < 0.0 || uvz.x > 1.0 || uvz.y < 0.0 || uvz.y > 1.0) return 0.0;

    float shadow = 0.0;
    int radius = int(pcf_radius); // e.g. 1 -> 3x3 taps
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            vec2 offset = vec2(dx, dy) * texelSize;
            float closest = texture(sampler2DArray(shadow_tex, shadow_smp), vec3(uvz.xy + offset, float(cascadeIndex))).r;
            shadow += (uvz.z - depthBias > closest) ? 1.0 : 0.0;            
        }
    }
    float taps = float((2*radius + 1)*(2*radius + 1));
    return shadow / taps;
}

// Main: compute shadow factor for this fragment (0..1), also return cascade idx
float shadow_csm(vec3 world_position, vec3 world_normal, out int casecade_idx) {
    // View-space depth (positive distance forward). If your view convention differs,
    // use: float view_depth = abs((view * vec4(world_position,1)).z);

    float view_depth = -(view * vec4(world_position, 1.0)).z;

    casecade_idx = choose_cascade(view_depth);

    // Project to this cascade's light NDC
    vec4 lpos = light_view_proj[casecade_idx] * vec4(world_position, 1.0);
    // Perspective divide
    lpos.xyz /= lpos.w;

    // NDC -> UV, depth in [0,1]
    vec2 uv     = lpos.xy * 0.5 + 0.5;
    float rz    = lpos.z * 0.5 + 0.5;

    // Receiver + slope bias
    float ndotl = max(dot(normalize(world_normal), -normalize(light_dir)), 0.0);
    float slope = (1.0 - ndotl); // higher when grazing
    float bias  = depth_bias + slope_bias * slope;

    // Texel size for this cascade
	vec2 css = vec2(cascade_splits_shadow_map_size[casecade_idx].x, cascade_splits_shadow_map_size[casecade_idx].y);
    vec2 texel = 1.0 / max(css, vec2(1.0));

    // Percentage-Closer Filtering
    float blocked = pcf_shadow_array(vec3(uv, rz), casecade_idx, texel, bias);

    // Soft fade at cascade edges to hide seams (optional)
    // You can compute blend with neighbor cascade if desired. Here we keep it simple.

    // Return visibility
    return 1.0 - blocked;
}

vec4 gamma(vec4 c) {
    float p = 1.0 / 2.2;
    return vec4(pow(c.xyz, vec3(p)), c.w);
}

void main() {
	
	vec3 diff = texture(sampler2D(tex, smp), uv).xyz;
    int cidx = 0;
    float visibility = shadow_csm(world_pos.xyz, normalize(world_norm), cidx);

    float ndotl = max(dot(normalize(world_norm), -normalize(light_dir)), 0.0);
    vec3  direct = light_color * diff  * ndotl * visibility;

    frag_color = vec4(light_ambient * diff + direct, 1.0);
	frag_color = gamma(frag_color);
}

#pragma sokol @end
#pragma sokol @program texture_static_mesh texture_static_mesh_vs texture_static_mesh_fs
