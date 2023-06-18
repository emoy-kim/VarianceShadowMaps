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

layout (binding = 0) uniform sampler2DArray MomentsMap;

uniform mat4 WorldMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 LightViewProjectionMatrix[3];
uniform float SplitPositions[3];
uniform int UseLight;
uniform int LightIndex;
uniform int LightNum;
uniform vec4 GlobalAmbient;

in vec3 position_in_wc;
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

float getChebyshevUpperBound(in vec3 depth_map_coord, in int split)
{
   float t = depth_map_coord.z;
   vec2 moments = texture( MomentsMap, vec3(depth_map_coord.xy, float(split)) ).rg;
   if (t <= moments.x) return one;

   const float min_variance = 1e-6f;
   float variance = max( moments.y - moments.x * moments.x, min_variance );
   float d = t - moments.x;
   return variance / (variance + d * d);
}

float reduceLightBleeding(in float shadow)
{
   const float light_bleeding_reduction_amount = 0.18f;
   return clamp( (shadow - light_bleeding_reduction_amount) / (one - light_bleeding_reduction_amount), zero, one );
}

float getShadowWithPSVSM()
{
   vec4 split_positions = vec4(SplitPositions[0], SplitPositions[1], SplitPositions[2], 1E+20f);
   float slice_depth = -position_in_ec.z;
   int split = int(dot( vec4(one), vec4(greaterThan( vec4(slice_depth), split_positions )) ));

   const int split_lookup[8] = { 0, 1, 1, 2, 2, 2, 2, 3 };
   int power_of_two = 1 << split;
   int split_x = int(abs( dFdx( power_of_two ) ));
   int split_y = int(abs( dFdy( power_of_two ) ));
   int split_xy = int(abs( dFdx( split_y ) ));
   int split_max = max( split_xy, max( split_x, split_y ) );
   // split_max = min( split_max, 8 );
   split = split_max > 0 ? split_lookup[split_max - 1] : split;

   const float bias_for_shadow_acne = 0.005f;
   vec4 position_in_light_cc = LightViewProjectionMatrix[split] * vec4(position_in_wc, 1.0f);
   vec4 depth_map_coord = vec4(0.5f * position_in_light_cc.xyz / position_in_light_cc.w + 0.5f, position_in_light_cc.w);
   depth_map_coord.z -= bias_for_shadow_acne;

   const float epsilon = 1e-2f;
   if (epsilon <= depth_map_coord.x && depth_map_coord.x <= one - epsilon &&
       epsilon <= depth_map_coord.y && depth_map_coord.y <= one - epsilon &&
       zero < depth_map_coord.w) {
      float shadow = getChebyshevUpperBound( depth_map_coord.xyz, split );
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

   color += local_color * getShadowWithPSVSM();
   return color;
}

void main()
{
   final_color = bool(UseLight) ? calculateLightingEquation() : Material.DiffuseColor;
}