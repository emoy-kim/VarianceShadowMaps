#version 460

#define MAX_LIGHTS 32

struct LightInfo
{
   int LightSwitch;
   vec4 Position;
   vec4 AmbientColor;
   vec4 DiffuseColor;
   vec4 SpecularColor;
   vec3 SpotlightDirection;
   float SpotlightExponent;
   float SpotlightCutoffAngle;
   vec3 AttenuationFactors;
};
uniform LightInfo Lights[MAX_LIGHTS];

struct MateralInfo {
   vec4 EmissionColor;
   vec4 AmbientColor;
   vec4 DiffuseColor;
   vec4 SpecularColor;
   float SpecularExponent;
};
uniform MateralInfo Material;

layout (binding = 0) uniform sampler2D MomentsMap;

uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 LightViewProjectionMatrix;
uniform int UseLight;
uniform int LightIndex;
uniform int LightNum;
uniform vec4 GlobalAmbient;

in vec4 position_in_wc;
in vec3 position_in_ec;
in vec3 normal_in_ec;
in vec2 tex_coord;

layout (location = 0) out vec4 final_color;

const float zero = 0.0f;
const float one = 1.0f;

bool IsPointLight(in vec4 light_position)
{
   return light_position.w != zero;
}

float getAttenuation(in vec3 light_vector, in int light_index)
{
   vec3 distance_scale;
   distance_scale.x = one;
   distance_scale.z = dot( light_vector, light_vector );
   distance_scale.y = sqrt( distance_scale.z );
   return one / dot( distance_scale, Lights[light_index].AttenuationFactors );
}

float getSpotlightFactor(in vec3 normalized_light_vector, in int light_index)
{
   if (Lights[light_index].SpotlightCutoffAngle >= 180.0f) return one;

   vec4 direction_in_ec = transpose( inverse( ViewMatrix ) ) * vec4(Lights[light_index].SpotlightDirection, one);
   vec3 normalized_direction = normalize( direction_in_ec.xyz );
   float cutoff_angle = clamp( Lights[light_index].SpotlightCutoffAngle, zero, 90.0f );
   float factor = dot( -normalized_light_vector, normalized_direction );
   return factor >= cos( radians( cutoff_angle ) ) ? pow( factor, Lights[light_index].SpotlightExponent ) : zero;
}

vec2 getMomentsFromSAT(in ivec4 coord, in ivec2 offset)
{
   vec2 s00 = texelFetchOffset( MomentsMap, coord.xy, 0, offset ).rg;
   vec2 s10 = texelFetchOffset( MomentsMap, coord.zy, 0, offset ).rg;
   vec2 s01 = texelFetchOffset( MomentsMap, coord.xw, 0, offset ).rg;
   vec2 s11 = texelFetchOffset( MomentsMap, coord.zw, 0, offset ).rg;
   return s11 - s01 - s10 + s00;
}

float getChebyshevUpperBound(in ivec4 tile, in vec4 weights, in float t)
{
   vec2 filter_size = vec2(tile.zw - tile.xy);
   float normalizer = one / (filter_size.x * filter_size.y);
   vec2 m00 = getMomentsFromSAT( tile, ivec2(0) ) * normalizer;
   vec2 m10 = getMomentsFromSAT( tile, ivec2(1, 0) ) * normalizer;
   vec2 m01 = getMomentsFromSAT( tile, ivec2(0, 1) ) * normalizer;
   vec2 m11 = getMomentsFromSAT( tile, ivec2(1) ) * normalizer;
   vec2 moments = vec2(
      dot( weights, vec4(m10.x, m01.x, m11.x, m00.x) ),
      dot( weights, vec4(m10.y, m01.y, m11.y, m00.y) )
   );
   if (t <= moments.x) return one;

   const float min_variance = 3e-4f;
   float variance = max( moments.y - moments.x * moments.x, min_variance );
   float d = t - moments.x;
   return variance / (variance + d * d);
}

float reduceLightBleeding(in float shadow)
{
   const float light_bleeding_reduction_amount = 0.18f;
   return clamp( (shadow - light_bleeding_reduction_amount) / (one - light_bleeding_reduction_amount), zero, one );
}

float getShadowWithSATVSM()
{
   vec4 position_in_light_cc = LightViewProjectionMatrix * position_in_wc;
   vec4 moments_map_coord = vec4(
      0.5f * position_in_light_cc.xyz / position_in_light_cc.w + 0.5f,
      position_in_light_cc.w
   );

   vec2 dx = dFdx( moments_map_coord.xy );
   vec2 dy = dFdy( moments_map_coord.xy );

   const float epsilon = 1e-1f;
   const vec2 min_size = vec2(one);
   if (epsilon <= moments_map_coord.x && moments_map_coord.x <= one - epsilon &&
       epsilon <= moments_map_coord.y && moments_map_coord.y <= one - epsilon &&
       zero < moments_map_coord.w) {
      vec2 shadow_size = vec2(textureSize( MomentsMap, 0 ));

      // this filter is designed to reduce artifacts causing when the filter size from pcf is used.
      // the filter size gets increased when the camera is closer to hide artifacts.
      vec2 filter_size = smoothstep( 1.05f - abs( dx ) - abs( dy ), vec2(zero), vec2(one) );
      filter_size = round( clamp( filter_size * shadow_size, min_size, shadow_size ) );
      //vec2 filter_size = round( clamp( 2.0f * (abs( dx ) + abs( dy )) * shadow_size, min_size, max_size ) );

      vec2 lower_left = moments_map_coord.xy * shadow_size - 0.5f * filter_size;
      vec2 upper_right = lower_left + filter_size;
      ivec4 tile = ivec4(lower_left, upper_right);

      vec4 weights;
      weights.xy = fract( lower_left );
      weights.zw = one - weights.xy;
      weights = weights.xzxz * weights.wyyw;
      float shadow = getChebyshevUpperBound( tile, weights, moments_map_coord.z );
      return reduceLightBleeding( shadow );
   }
   return one;
}

vec4 calculateLightingEquation()
{
   vec4 color = Material.EmissionColor + GlobalAmbient * Material.AmbientColor;

   if (Lights[LightIndex].LightSwitch == 0) return color;
      
   vec4 light_position_in_ec = ViewMatrix * Lights[LightIndex].Position;
      
   float final_effect_factor = one;
   vec3 light_vector = light_position_in_ec.xyz - position_in_ec;
   if (IsPointLight( light_position_in_ec )) {
      float attenuation = getAttenuation( light_vector, LightIndex );

      light_vector = normalize( light_vector );
      float spotlight_factor = getSpotlightFactor( light_vector, LightIndex );
      final_effect_factor = attenuation * spotlight_factor;
   }
   else light_vector = normalize( light_position_in_ec.xyz );
   
   if (final_effect_factor <= zero) return color;

   vec4 local_color = Lights[LightIndex].AmbientColor * Material.AmbientColor;

   float diffuse_intensity = max( dot( normal_in_ec, light_vector ), zero );
   local_color += diffuse_intensity * Lights[LightIndex].DiffuseColor * Material.DiffuseColor;

   vec3 halfway_vector = normalize( light_vector - normalize( position_in_ec ) );
   float specular_intensity = max( dot( normal_in_ec, halfway_vector ), zero );
   local_color += 
      pow( specular_intensity, Material.SpecularExponent ) * 
      Lights[LightIndex].SpecularColor * Material.SpecularColor;

   color += local_color * getShadowWithSATVSM();
   return color;
}

void main()
{
   final_color = bool(UseLight) ? calculateLightingEquation() : Material.DiffuseColor;
}