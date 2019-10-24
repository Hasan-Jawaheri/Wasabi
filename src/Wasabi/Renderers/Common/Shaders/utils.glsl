
vec2 packNormalSpheremapTransform(vec3 norm) {
    vec2 packed = normalize(norm.xy) * (sqrt(-norm.z*0.5+0.5));
    packed = packed * 0.5 + 0.5;
    return packed;
}

vec3 unpackNormalSpheremapTransform(vec2 packed2) {
	vec4 packed = vec4(packed2, 0, 0);
    vec4 nn = packed * vec4(2,2,0,0) + vec4(-1,-1,1,-1);
    float l = dot(nn.xyz, -nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    vec3 unpacked = nn.xyz * 2 + vec3(0,0,-1);
    return unpacked;
}
