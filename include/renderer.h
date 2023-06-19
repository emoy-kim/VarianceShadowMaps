/*
 * Author: Jeesun Kim
 * E-mail: emoy.kim_AT_gmail.com
 *
 */

#pragma once

#include "base.h"
#include "text.h"
#include "light.h"

class RendererGL final
{
public:
   RendererGL();
   ~RendererGL();

   RendererGL(const RendererGL&) = delete;
   RendererGL(const RendererGL&&) = delete;
   RendererGL& operator=(const RendererGL&) = delete;
   RendererGL& operator=(const RendererGL&&) = delete;

   void play();

private:
   enum class ALGORITHM_TO_COMPARE { PCF = 0, VSM, PSVSM, SATVSM };

   inline static RendererGL* Renderer = nullptr;
   GLFWwindow* Window;
   bool Pause;
   int FrameWidth;
   int FrameHeight;
   int ShadowMapSize;
   int ActiveLightIndex;
   int SplitNum;
   float BoxHalfSide;
   GLuint DepthFBO;
   GLuint DepthTextureID;
   GLuint MomentsFBO;
   GLuint MomentsTextureID;
   GLuint MomentsLayerFBO;
   GLuint MomentsTextureArrayID;
   GLuint SATTextureID;
   glm::ivec2 ClickedPoint;
   std::unique_ptr<TextGL> Texter;
   std::unique_ptr<CameraGL> MainCamera;
   std::unique_ptr<CameraGL> TextCamera;
   std::unique_ptr<CameraGL> LightCamera;
   std::unique_ptr<ShaderGL> TextShader;
   std::unique_ptr<ShaderGL> PCFSceneShader;
   std::unique_ptr<ShaderGL> VSMSceneShader;
   std::unique_ptr<ShaderGL> PSVSMSceneShader;
   std::unique_ptr<ShaderGL> SATVSMSceneShader;
   std::unique_ptr<ShaderGL> LightViewDepthShader;
   std::unique_ptr<ShaderGL> LightViewMomentsShader;
   std::unique_ptr<ShaderGL> LightViewMomentsArrayShader;
   std::unique_ptr<ShaderGL> SATShader;
   std::unique_ptr<LightGL> Lights;
   std::unique_ptr<ObjectGL> Object;
   std::unique_ptr<ObjectGL> WallObject;
   std::vector<float> SplitPositions;
   std::vector<glm::mat4> LightViewProjectionMatrices;
   ALGORITHM_TO_COMPARE AlgorithmToCompare;

   // 16 and 32 do well, anything in between or below is bad.
   // 32 seems to do well on laptop/desktop Windows Intel and on NVidia/AMD as well.
   // further hardware-specific tuning might be needed for optimal performance.
   static constexpr int ThreadGroupSize = 32;
   [[nodiscard]] static int getGroupSize(int size)
   {
      return (size + ThreadGroupSize - 1) / ThreadGroupSize;
   }

   void registerCallbacks() const;
   void initialize();
   void writeFrame() const;
   void writeSATTexture() const;
   void writeMomentsArrayTexture() const;
   static void printOpenGLInformation();
   static void cleanup(GLFWwindow* window);
   static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
   static void cursor(GLFWwindow* window, double xpos, double ypos);
   static void mouse(GLFWwindow* window, int button, int action, int mods);
   static void mousewheel(GLFWwindow* window, double xoffset, double yoffset);

   void setLights() const;
   void setObject() const;
   void setWallObject() const;
   void setLightViewFrameBuffers();
   void drawObject(ShaderGL* shader, CameraGL* camera) const;
   void drawBoxObject(ShaderGL* shader, const CameraGL* camera) const;
   void drawDepthMapFromLightView() const;
   void drawMomentsMapFromLightView() const;
   void drawMomentsArrayMapFromLightView() const;
   void splitViewFrustum();
   void calculateLightCropMatrices();
   void generateSummedAreaTable() const;
   void drawShadowWithPCF() const;
   void drawShadowWithVSM() const;
   void drawShadowWithPSVSM() const;
   void drawShadowWithSATVSM() const;
   void drawText(const std::string& text, glm::vec2 start_position) const;
   void render();
};