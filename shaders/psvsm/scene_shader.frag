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
   float SpotlightCutoffAngle;
   float SpotlightFeather;
   float FallOffRadius;
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

layout (location = 0) out vec4 final_color;

const float zero = 0.0f;
const float one = 1.0f;
const float half_pi = 1.57079632679489661923132169163975144f;

bool IsPointLight(in vec4 light_position)
{
   return light_position.w != zero;
}

float getAttenuation(in vec3 light_vector, in int light_index)
{
   float squared_distance = dot( light_vector, light_vector );
   float distance = sqrt( squared_distance );
   float radius = Lights[light_index].FallOffRadius;
   if (distance <= radius) return one;

   return clamp( radius * radius / squared_distance, zero, one );
}

float getSpotlightFactor(in vec3 normalized_light_vector, in int light_index)
{
   if (Lights[light_index].SpotlightCutoffAngle >= 180.0f) return one;

   vec4 direction_in_ec = transpose( inverse( ViewMatrix ) ) * vec4(Lights[light_index].SpotlightDirection, zero);
   vec3 normalized_direction = normalize( direction_in_ec.xyz );
   float factor = dot( -normalized_light_vector, normalized_direction );
   float cutoff_angle = radians( clamp( Lights[light_index].SpotlightCutoffAngle, zero, 90.0f ) );
   if (factor >= cos( cutoff_angle )) {
      float normalized_angle = acos( factor ) * half_pi / cutoff_angle;
      float threshold = half_pi * (one - Lights[light_index].SpotlightFeather);
      return normalized_angle <= threshold ? one :
         cos( half_pi * (normalized_angle - threshold) / (half_pi - threshold) );
   }
   return zero;
}

float getChebyshevUpperBound(in vec3 moments_map_coord, in int split)
{
   float t = moments_map_coord.z;
   vec2 moments = texture( MomentsMap, vec3(moments_map_coord.xy, float(split)) ).rg;
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
   float depth = -position_in_ec.z;
   vec3 split_positions = vec3(SplitPositions[0], SplitPositions[1], SplitPositions[2]);
   int split = int(dot( vec3(one), vec3(greaterThan( vec3(depth), split_positions )) )) - 1;

   // https://forum.beyond3d.com/threads/variance-shadow-maps-demo-d3d10.37062/
   const int split_lookup[8] = { 0, 1, 1, 2, 2, 2, 2, 3 };
   int power_of_two = 1 << split;
   int split_x = int(abs( dFdx( power_of_two ) ));
   int split_y = int(abs( dFdy( power_of_two ) ));
   int split_xy = int(abs( dFdx( split_y ) ));
   int split_max = max( split_xy, max( split_x, split_y ) );
   // split_max = min( split_max, 8 );
   split = split_max > 0 ? split_lookup[split_max - 1] : split;

   vec4 position_in_light = LightViewProjectionMatrix[split] * vec4(position_in_wc, 1.0f);
   vec4 moments_map_coord = vec4(0.5f * position_in_light.xyz / position_in_light.w + 0.5f, position_in_light.w);

   const float epsilon = 1e-2f;
   if (epsilon <= moments_map_coord.x && moments_map_coord.x <= one - epsilon &&
       epsilon <= moments_map_coord.y && moments_map_coord.y <= one - epsilon &&
       zero < moments_map_coord.w) {
      float shadow = getChebyshevUpperBound( moments_map_coord.xyz, split );
      //return reduceLightBleeding( shadow );
      return shadow;
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

   color += local_color * final_effect_factor * getShadowWithPSVSM();
   return color;
}

void main()
{
   final_color = bool(UseLight) ? calculateLightingEquation() : Material.DiffuseColor;
}