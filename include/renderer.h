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
   enum class ALGORITHM_TO_COMPARE { PCF = 0 };

   inline static RendererGL* Renderer = nullptr;
   GLFWwindow* Window;
   bool Pause;
   int FrameWidth;
   int FrameHeight;
   int ShadowMapSize;
   int ActiveLightIndex;
   int PassNum;
   GLuint FBO;
   GLuint DepthTextureID;
   glm::ivec2 ClickedPoint;
   std::unique_ptr<TextGL> Texter;
   std::unique_ptr<CameraGL> MainCamera;
   std::unique_ptr<CameraGL> TextCamera;
   std::unique_ptr<CameraGL> LightCamera;
   std::unique_ptr<ShaderGL> TextShader;
   std::unique_ptr<ShaderGL> PCFSceneShader;
   std::unique_ptr<ShaderGL> LightViewShader;
   std::unique_ptr<LightGL> Lights;
   std::unique_ptr<ObjectGL> Object;
   std::unique_ptr<ObjectGL> WallObject;
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
   void writeFrame(const std::string& name) const;
   void writeDepthTexture(const std::string& name) const;

   static void printOpenGLInformation();

   static void cleanup(GLFWwindow* window);
   static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
   static void cursor(GLFWwindow* window, double xpos, double ypos);
   static void mouse(GLFWwindow* window, int button, int action, int mods);
   static void mousewheel(GLFWwindow* window, double xoffset, double yoffset);

   void setLights() const;
   void setObject() const;
   void setWallObject() const;
   void setDepthFrameBuffer();
   void drawObject(ShaderGL* shader, CameraGL* camera) const;
   void drawBoxObject(ShaderGL* shader, const CameraGL* camera) const;
   void drawDepthMapFromLightView() const;
   void drawShadowWithPCF() const;
   void drawText(const std::string& text, glm::vec2 start_position) const;
   void render();
};