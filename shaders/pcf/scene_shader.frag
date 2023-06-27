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

layout (binding = 0) uniform sampler2DShadow DepthMap;

uniform mat4 WorldMatrix;
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

   vec4 direction_in_ec = 
      transpose( inverse( ViewMatrix * WorldMatrix ) ) * 
      vec4(Lights[light_index].SpotlightDirection, one);
   vec3 normalized_direction = normalize( direction_in_ec.xyz );
   float cutoff_angle = clamp( Lights[light_index].SpotlightCutoffAngle, zero, 90.0f );
   float factor = dot( -normalized_light_vector, normalized_direction );
   return factor >= cos( radians( cutoff_angle ) ) ? pow( factor, Lights[light_index].SpotlightExponent ) : zero;
}

float getShadowWithPCF()
{
   const float bias_for_shadow_acne = 0.005f;
   vec4 position_in_light_cc = LightViewProjectionMatrix * position_in_wc;
   vec4 depth_map_coord = vec4(
      0.5f * position_in_light_cc.xyz / position_in_light_cc.w + 0.5f,
      position_in_light_cc.w
   );
   depth_map_coord.z -= bias_for_shadow_acne;

   vec2 dx = dFdx( depth_map_coord.xy );
   vec2 dy = dFdy( depth_map_coord.xy );

   const float epsilon = 1e-2f;
   const vec2 min_size = vec2(one);
   const vec2 max_size = vec2(32.0f);
   if (epsilon <= depth_map_coord.x && depth_map_coord.x <= one - epsilon &&
       epsilon <= depth_map_coord.y && depth_map_coord.y <= one - epsilon &&
       zero < depth_map_coord.w) {
      vec2 shadow_size = vec2(textureSize( DepthMap, 0 ));
      vec2 normalizer = one / shadow_size;

      // compute the filter size in pixels to decide how many pixels should be searched.
      // the filter size gets decreased when the camera is closer to shadow because adjacent fragments represent
      // the close pixels in the light view. on the other hand, the filter size gets increased when the camera is
      // farther to shadow for the same reason.
      vec2 filter_size = round( clamp( 2.0f * (abs( dx ) + abs( dy )) * shadow_size, min_size, max_size ) );

      ivec2 window_size = ivec2(filter_size);
      filter_size *= normalizer;
      vec2 lower_left = depth_map_coord.xy - 0.5f * filter_size;
      float shadow = zero;
      for (int y = 0; y < window_size.y; ++y) {
         for (int x = 0; x < window_size.x; ++x) {
            vec3 tex_coord = vec3(lower_left + vec2(x, y) * normalizer, depth_map_coord.z);
            shadow += texture( DepthMap, tex_coord );
         }
      }
      return shadow / float(window_size.x * window_size.y);
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

   color += local_color * getShadowWithPCF();
   return color;
}

void main()
{
   final_color = bool(UseLight) ? calculateLightingEquation() : Material.DiffuseColor;
}