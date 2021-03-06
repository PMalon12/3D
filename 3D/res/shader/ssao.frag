#version 120 // -*- c++ -*-
#extension GL_EXT_gpu_shader4 : require
#extension GL_ARB_gpu_shader5 : enable

in vec2 TexCoord;

// Total number of direct samples to take at each pixel
//#define NUM_SAMPLES (50)
uniform int samples = 50;

/** Used for preventing AO computation on the sky (at infinite depth) and defining the CS Z to bilateral depth key scaling. 
    This need not match the real far plane*/
#define FAR 1000000.f // projection matrix's far plane
#define FAR_PLANE_Z (-1000000.0)

// This is the number of turns around the circle that the spiral pattern makes.  This should be prime to prevent
// taps from lining up.  This particular choice was tuned for NUM_SAMPLES == 9
//#define NUM_SPIRAL_TURNS (30)
uniform int spiralTurns;

//////////////////////////////////////////////////

/** The height in pixels of a 1m object if viewed from 1m away.  
    You can compute it from your projection matrix.  The actual value is just
    a scale factor on radius; you can simply hardcode this to a constant (~500)
    and make your radius value unitless (...but resolution dependent.)  */
uniform float           projScale = 500.f;

/** Negative, "linear" values in world-space units */
uniform sampler2D       depthBuffer;

/** World-space AO radius in scene units (r).  e.g., 1.0m */
uniform float           radius = 1.f;

/** Bias to avoid AO in smooth corners, e.g., 0.01m */
uniform float           bias = 0.01f;

/** intensity / radius^6 */
uniform float           intensityDivR6 = 100.f;

uniform mat4 view;
uniform mat4 proj;

// Compatibility with future versions of GLSL: the shader still works if you change the 
// version line at the top to something like #version 330 compatibility.
#if __VERSION__ == 120
#   define texelFetch   texelFetch2D
#   define textureSize  textureSize2D
#   else
    out vec3            gl_FragColor;
#endif
#define visibility      gl_FragColor.r
#define bilateralKey    gl_FragColor.gb

/**  vec4(-2.0f / (width*P[0][0]), 
          -2.0f / (height*P[1][1]),
          ( 1.0f - P[0][2]) / P[0][0], 
          ( 1.0f + P[1][2]) / P[1][1])
    
    where P is the projection matrix that maps camera space points 
    to [-1, 1] x [-1, 1].  That is, GCamera::getProjectUnit(). */
uniform vec4 projInfo;

/** Reconstruct camera-space P.xyz from screen-space S = (x, y) in
    pixels and camera-space z < 0.  Assumes that the upper-left pixel center
    is at (0.5, 0.5) [but that need not be the location at which the sample tap 
    was placed!]

    Costs 3 MADD.  Error is on the order of 10^3 at the far plane, partly due to z precision.
  */
vec3 reconstructCSPosition(vec2 S, float z) {
    return vec3((S.xy * projInfo.xy + projInfo.zw) * z, z);

}

/** Reconstructs screen-space unit normal from screen-space position */
vec3 reconstructCSFaceNormal(vec3 C) {
    return normalize(cross(dFdx(C), dFdy(C)));
}

const float NEAR = 5.f; // projection matrix's near plane

float LinearizeDepth(float depth)
{
    //float z = depth * 2.0 - 1.0; // Back to NDC 
    //return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));    
    vec3 c = vec3(NEAR * FAR, NEAR - FAR, FAR);
    float r = c.x / ((depth * c.y) + c.z);
    return r;
}

/////////////////////////////////////////////////////////

/** Returns a unit vector and a screen-space radius for the tap on a unit disk (the caller should scale by the actual disk radius) */
vec2 tapLocation(int sampleNumber, float spinAngle, out float ssR){
    // Radius relative to ssR
    float alpha = float(sampleNumber + 0.5) * (1.0 / float(samples));
    float angle = alpha * (float(spiralTurns) * 6.28) + spinAngle;

    ssR = alpha;
    return vec2(cos(angle), sin(angle));
}


/** Used for packing Z into the GB channels */
float CSZToKey(float z) {
    return clamp(z * (1.0 / FAR_PLANE_Z), 0.0, 1.0);
}


/** Used for packing Z into the GB channels */
void packKey(float key, out vec2 p) {
    // Round to the nearest 1/256.0
    float temp = floor(key * 256.0);

    // Integer part
    p.x = temp * (1.0 / 256.0);

    // Fractional part
    p.y = key * 256.0 - temp;
}

 
/** Read the camera-space position of the point at screen-space pixel ssP */
vec3 getPosition(ivec2 ssP) {
    vec3 P;
    P.z = texelFetch(depthBuffer, ssP, 0).r;
    float depth = -(exp2(P.z * log2(FAR + 1.0)) - 1.f);
    //float depth = P.z;
    P = reconstructCSPosition(vec2(ssP) + vec2(0.5), depth);

    return P;
}


/** Read the camera-space position of the point at screen-space pixel ssP + unitOffset * ssR.  Assumes length(unitOffset) == 1 */
vec3 getOffsetPosition(ivec2 ssC, vec2 unitOffset, float ssR) {

    ivec2 ssP = ivec2(ssR * unitOffset) + ssC;
    
    vec3 P;

    ivec2 mipP = clamp(ssP, ivec2(0), textureSize(depthBuffer, 0) - ivec2(1));
    P.z = texelFetch(depthBuffer, mipP, 0).r;
    

    float depth = -(exp2(P.z * log2(FAR + 1.0)) - 1.f);

    P = reconstructCSPosition(vec2(ssP) + vec2(0.5), depth);

    return P;
}


float radius2 = radius * radius;
float invRadius2 = 1.f / radius2;

/** Compute the occlusion due to sample with index \a i about the pixel at \a ssC that corresponds
    to camera-space point \a C with unit normal \a n_C, using maximum screen-space sampling radius \a ssDiskRadius 

    Note that units of H() in the HPG12 paper are meters, not
    unitless.  The whole falloff/sampling function is therefore
    unitless.  In this implementation, we factor out (9 / radius).

    Four versions of the falloff function are implemented below
*/
float sampleAO(in ivec2 ssC, in vec3 C, in vec3 n_C, in float ssDiskRadius, in int tapIndex, in float randomPatternRotationAngle) {
    // Offset on the unit disk, spun for this pixel
    float ssR;
    vec2 unitOffset = tapLocation(tapIndex, randomPatternRotationAngle, ssR);
    ssR *= ssDiskRadius;
    
    // The occluding point in camera space
    vec3 Q = getOffsetPosition(ssC, unitOffset, ssR);

    vec3 v = Q - C;

    float vv = dot(v, v);
    float vn = dot(v, n_C);

    const float epsilon = 0.01;
    
    // A: From the HPG12 paper
    // Note large epsilon to avoid overdarkening within cracks
    //return float(vv < radius2) * max((vn - bias) / (epsilon + vv), 0.0) * radius2 * 0.6;

    // B: Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
    float f = max(radius2 - vv, 0.0); return f * f * f * max((vn - bias) / (epsilon + vv), 0.0);

    // C: Medium contrast (which looks better at high radii), no division.  Note that the 
    // contribution still falls off with radius^2, but we've adjusted the rate in a way that is
    // more computationally efficient and happens to be aesthetically pleasing.
    //return 4.0 * max(1.0 - vv * invRadius2, 0.0) * max(vn - bias, 0.0);

    // D: Low contrast, no division operation
    //return 2.0 * float(vv < radius * radius) * max(vn - bias, 0.0);
}




void main() {

    // Pixel being shaded 
    ivec2 ssC = ivec2(gl_FragCoord.xy);

    // World space point being shaded
    vec3 C = getPosition(ssC);

    packKey(CSZToKey(C.z), bilateralKey);

    // Hash function used in the HPG12 AlchemyAO paper
    //float randomPatternRotationAngle = (3 * ssC.x ^ ssC.y + ssC.x * ssC.y) * 10;
    float randomPatternRotationAngle = 0.5;

    // Reconstruct normals from positions. These will lead to 1-pixel black lines
    // at depth discontinuities, however the blur will wipe those out so they are not visible
    // in the final image.
    vec3 n_C = reconstructCSFaceNormal(C);

    // Choose the screen-space sample radius
    // proportional to the projected area of the sphere
    float ssDiskRadius = projScale * radius / C.z;
    //float ssDiskRadius =  projScale / C.z;
    
    float sum = 0.0;
    for (int i = 0; i < samples; ++i) {
        sum += sampleAO(ssC, C, n_C, ssDiskRadius, i, randomPatternRotationAngle);
    }

    float A = max(0.0, 1.0 - sum * intensityDivR6 * (5.0 / float(samples)));

    // Bilateral box-filter over a quad for free, respecting depth edges
    // (the difference that this makes is subtle)
    if (abs(dFdx(C.z)) < 0.02) {
        A -= dFdx(A) * ((ssC.x & 1) - 0.5);
    }
    if (abs(dFdy(C.z)) < 0.02) {
        A -= dFdy(A) * ((ssC.y & 1) - 0.5);
    }
    
    visibility = A;

    //gl_FragColor.rgb = vec3();

    //ivec2 ssP = ssC;

    //float d = texelFetch(depthBuffer, ssP, 0).r;
    //float depth = (exp2(d * log2(FAR + 1.0)) - 1.f);

    //gl_FragColor.rgb = vec3(d);
    
}