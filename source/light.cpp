#include "light.h"

LightGL::LightGL() :
   TurnLightOn( true ), GlobalAmbientColor( 0.2f, 0.2f, 0.2f, 1.0f ), TotalLightNum( 0 )
{
}

bool LightGL::isLightOn() const
{
   return TurnLightOn;
}

void LightGL::toggleLightSwitch()
{
   TurnLightOn = !TurnLightOn;
}

void LightGL::addLight(
   const glm::vec4& light_position,
   const glm::vec4& ambient_color,
   const glm::vec4& diffuse_color,
   const glm::vec4& specular_color,
   const glm::vec3& spotlight_direction,
   float spotlight_exponent,
   float spotlight_cutoff_angle_in_degree,
   const glm::vec3& attenuation_factor
)
{
   Positions.emplace_back( light_position );

   AmbientColors.emplace_back( ambient_color );
   DiffuseColors.emplace_back( diffuse_color );
   SpecularColors.emplace_back( specular_color );

   SpotlightDirections.emplace_back( spotlight_direction );
   SpotlightExponents.emplace_back( spotlight_exponent );
   SpotlightCutoffAngles.emplace_back( spotlight_cutoff_angle_in_degree );

   AttenuationFactors.emplace_back( attenuation_factor );

   IsActivated.emplace_back( true );

   TotalLightNum = static_cast<int>(Positions.size());
}

void LightGL::setGlobalAmbientColor(const glm::vec4& global_ambient_color)
{
   GlobalAmbientColor = global_ambient_color;
}

void LightGL::setAmbientColor(const glm::vec4& ambient_color, const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   AmbientColors[light_index] = ambient_color;
}

void LightGL::setDiffuseColor(const glm::vec4& diffuse_color, const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   DiffuseColors[light_index] = diffuse_color;
}

void LightGL::setSpecularColor(const glm::vec4& specular_color, const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   SpecularColors[light_index] = specular_color;
}

void LightGL::setSpotlightDirection(const glm::vec3& spotlight_direction, const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   SpotlightDirections[light_index] = spotlight_direction;
}

void LightGL::setSpotlightExponent(const float& spotlight_exponent, const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   SpotlightExponents[light_index] = spotlight_exponent;
}

void LightGL::setSpotlightCutoffAngle(const float& spotlight_cutoff_angle_in_degree, const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   SpotlightCutoffAngles[light_index] = spotlight_cutoff_angle_in_degree;
}

void LightGL::setAttenuationFactor(const glm::vec3& attenuation_factor, const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   AttenuationFactors[light_index] = attenuation_factor;
}

void LightGL::setLightPosition(const glm::vec4& light_position, const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   Positions[light_index] = light_position;
}

void LightGL::activateLight(const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   IsActivated[light_index] = true;
}

void LightGL::deactivateLight(const int& light_index)
{
   if (light_index >= TotalLightNum) return;
   IsActivated[light_index] = false;
}

void LightGL::transferUniformsToShader(const ShaderGL* shader)
{
   glUniform1i( shader->getLightAvailabilityLocation(), TurnLightOn ? 1 : 0 );
   glUniform1i( shader->getLightNumLocation(), static_cast<GLint>(TotalLightNum) );
   glUniform4fv( shader->getGlobalAmbientLocation(), 1, &GlobalAmbientColor[0] );

   for (int i = 0; i < TotalLightNum; ++i) {
      glUniform1i( shader->getLightSwitchLocation( i ), IsActivated[0] ? 1 : 0 );
      glUniform4fv( shader->getLightPositionLocation( i ), 1, &Positions[i][0] );
      glUniform4fv( shader->getLightAmbientLocation( i ), 1, &AmbientColors[i][0] );
      glUniform4fv( shader->getLightDiffuseLocation( i ), 1, &DiffuseColors[i][0] );
      glUniform4fv( shader->getLightSpecularLocation( i ), 1, &SpecularColors[i][0] );
      glUniform3fv( shader->getLightSpotlightDirectionLocation( i ), 1, &SpotlightDirections[i][0] );
      glUniform1f( shader->getLightSpotlightExponentLocation( i ), SpotlightExponents[i] );
      glUniform1f( shader->getLightSpotlightCutoffAngleLocation( i ), SpotlightCutoffAngles[i] );
      glUniform3fv( shader->getLightAttenuationFactorsLocation( i ), 1, &AttenuationFactors[i][0] );
   }
}