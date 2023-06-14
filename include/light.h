#pragma once

#include "shader.h"

class LightGL final
{
public:
   LightGL();
   ~LightGL() = default;

   [[nodiscard]] bool isLightOn() const;
   void toggleLightSwitch();

   void addLight(
      const glm::vec4& light_position,
      const glm::vec4& ambient_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
      const glm::vec4& diffuse_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
      const glm::vec4& specular_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
      const glm::vec3& spotlight_direction = glm::vec3(0.0f, 0.0f, -1.0f),
      float spotlight_exponent = 0.0f,
      float spotlight_cutoff_angle_in_degree = 180.0f,
      const glm::vec3& attenuation_factor = glm::vec3(1.0f, 0.0f, 0.0f)
   );

   void setGlobalAmbientColor(const glm::vec4& global_ambient_color);
   void setAmbientColor(const glm::vec4& ambient_color, const int& light_index);
   void setDiffuseColor(const glm::vec4& diffuse_color, const int& light_index);
   void setSpecularColor(const glm::vec4& specular_color, const int& light_index);
   void setSpotlightDirection(const glm::vec3& spotlight_direction, const int& light_index);
   void setSpotlightExponent(const float& spotlight_exponent, const int& light_index);
   void setSpotlightCutoffAngle(const float& spotlight_cutoff_angle_in_degree, const int& light_index);
   void setAttenuationFactor(const glm::vec3& attenuation_factor, const int& light_index);
   void setLightPosition(const glm::vec4& light_position, const int& light_index);
   void activateLight(const int& light_index);
   void deactivateLight(const int& light_index);
   void transferUniformsToShader(const ShaderGL* shader);
   [[nodiscard]] int getTotalLightNum() const { return TotalLightNum; }
   [[nodiscard]] glm::vec4 getLightPosition(int light_index) { return Positions[light_index]; }

private:
   bool TurnLightOn;
   int TotalLightNum;
   glm::vec4 GlobalAmbientColor;
   std::vector<bool> IsActivated;
   std::vector<glm::vec4> Positions;
   std::vector<glm::vec4> AmbientColors;
   std::vector<glm::vec4> DiffuseColors;
   std::vector<glm::vec4> SpecularColors;
   std::vector<glm::vec3> SpotlightDirections;
   std::vector<float> SpotlightExponents;
   std::vector<float> SpotlightCutoffAngles;
   std::vector<glm::vec3> AttenuationFactors;
};