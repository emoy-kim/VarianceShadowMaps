#include "renderer.h"

RendererGL::RendererGL() :
   Window( nullptr ), Pause( false ), FrameWidth( 1920 ), FrameHeight( 1080 ), ShadowMapSize( 1024 ),
   ActiveLightIndex( 0 ), SplitNum( 3 ), BoxHalfSide( 500.0f ), DepthFBO( 0 ), DepthTextureID( 0 ), MomentsFBO( 0 ),
   MomentsTextureID( 0 ), MomentsLayerFBO( 0 ), MomentsTextureArrayID( 0 ), SATTextureID( 0 ), ClickedPoint( -1, -1 ),
   Texter( std::make_unique<TextGL>() ), MainCamera( std::make_unique<CameraGL>() ),
   TextCamera( std::make_unique<CameraGL>() ), LightCamera( std::make_unique<CameraGL>() ),
   TextShader( std::make_unique<ShaderGL>() ), PCFSceneShader( std::make_unique<ShaderGL>() ),
   VSMSceneShader( std::make_unique<ShaderGL>() ), PSVSMSceneShader( std::make_unique<ShaderGL>() ),
   SATVSMSceneShader( std::make_unique<ShaderGL>() ), LightViewDepthShader( std::make_unique<ShaderGL>() ),
   LightViewMomentsShader( std::make_unique<ShaderGL>() ), LightViewMomentsArrayShader( std::make_unique<ShaderGL>() ),
   SATShader( std::make_unique<ShaderGL>() ), Lights( std::make_unique<LightGL>() ),
   Object( std::make_unique<ObjectGL>() ), WallObject( std::make_unique<ObjectGL>() ),
   AlgorithmToCompare( ALGORITHM_TO_COMPARE::SATVSM )
{
   Renderer = this;

   initialize();
   printOpenGLInformation();
}

RendererGL::~RendererGL()
{
   if (DepthTextureID != 0) glDeleteTextures( 1, &DepthTextureID );
   if (MomentsTextureID != 0) glDeleteTextures( 1, &MomentsTextureID );
   if (MomentsTextureArrayID != 0) glDeleteTextures( 1, &MomentsTextureArrayID );
   if (SATTextureID != 0) glDeleteTextures( 1, &SATTextureID );
   if (DepthFBO != 0) glDeleteFramebuffers( 1, &DepthFBO );
   if (MomentsFBO != 0) glDeleteFramebuffers( 1, &MomentsFBO );
   if (MomentsLayerFBO != 0) glDeleteFramebuffers( 1, &MomentsLayerFBO );
}

void RendererGL::printOpenGLInformation()
{
   std::cout << "****************************************************************\n";
   std::cout << " - GLFW version supported: " << glfwGetVersionString() << "\n";
   std::cout << " - OpenGL renderer: " << glGetString( GL_RENDERER ) << "\n";
   std::cout << " - OpenGL version supported: " << glGetString( GL_VERSION ) << "\n";
   std::cout << " - OpenGL shader version supported: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n";
   std::cout << "****************************************************************\n\n";
}

void RendererGL::initialize()
{
   if (!glfwInit()) {
      std::cout << "Cannot Initialize OpenGL...\n";
      return;
   }
   glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
   glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
   glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );
   glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
   glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

   Window = glfwCreateWindow( FrameWidth, FrameHeight, "Variance Shadow Maps", nullptr, nullptr );
   glfwMakeContextCurrent( Window );

   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
   }

   registerCallbacks();

   glEnable( GL_CULL_FACE );
   glEnable( GL_DEPTH_TEST );
   glClearColor( 0.094, 0.07f, 0.17f, 1.0f );

   Texter->initialize( 30.0f );

   TextCamera->update2DCamera( FrameWidth, FrameHeight );
   MainCamera->updatePerspectiveCamera( FrameWidth, FrameHeight );
   LightCamera->updateOrthographicCamera( ShadowMapSize, ShadowMapSize );

   const std::string shader_directory_path = std::string(CMAKE_SOURCE_DIR) + "/shaders";
   TextShader->setShader(
      std::string(shader_directory_path + "/text.vert").c_str(),
      std::string(shader_directory_path + "/text.frag").c_str()
   );
   PCFSceneShader->setShader(
      std::string(shader_directory_path + "/pcf/scene_shader.vert").c_str(),
      std::string(shader_directory_path + "/pcf/scene_shader.frag").c_str()
   );
   VSMSceneShader->setShader(
      std::string(shader_directory_path + "/vsm/scene_shader.vert").c_str(),
      std::string(shader_directory_path + "/vsm/scene_shader.frag").c_str()
   );
   PSVSMSceneShader->setShader(
      std::string(shader_directory_path + "/psvsm/scene_shader.vert").c_str(),
      std::string(shader_directory_path + "/psvsm/scene_shader.frag").c_str()
   );
   SATVSMSceneShader->setShader(
      std::string(shader_directory_path + "/satvsm/scene_shader.vert").c_str(),
      std::string(shader_directory_path + "/satvsm/scene_shader.frag").c_str()
   );
   LightViewDepthShader->setShader(
      std::string(shader_directory_path + "/depth/light_view_depth_generator.vert").c_str(),
      std::string(shader_directory_path + "/depth/light_view_depth_generator.frag").c_str()
   );
   LightViewMomentsShader->setShader(
      std::string(shader_directory_path + "/depth/light_view_moments_generator.vert").c_str(),
      std::string(shader_directory_path + "/depth/light_view_moments_generator.frag").c_str()
   );
   LightViewMomentsArrayShader->setShader(
      std::string(shader_directory_path + "/depth/light_view_moments_array_generator.vert").c_str(),
      std::string(shader_directory_path + "/depth/light_view_moments_array_generator.frag").c_str()
   );
   SATShader->setComputeShader( std::string(shader_directory_path + "/satvsm/sat_generator.comp").c_str() );
}

void RendererGL::writeFrame() const
{
   const int size = FrameWidth * FrameHeight * 3;
   auto* buffer = new uint8_t[size];
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glNamedFramebufferReadBuffer( 0, GL_COLOR_ATTACHMENT0 );
   glReadPixels( 0, 0, FrameWidth, FrameHeight, GL_BGR, GL_UNSIGNED_BYTE, buffer );
   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, FrameWidth, FrameHeight, FrameWidth * 3, 24,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, "../result.png" );
   FreeImage_Unload( image );
   delete [] buffer;
}

void RendererGL::writeSATTexture() const
{
   const int size = ShadowMapSize * ShadowMapSize;
   auto* buffer = new uint8_t[size];
   auto* raw_buffer = new GLfloat[size * 2];
   glBindFramebuffer( GL_FRAMEBUFFER, MomentsFBO );
   glNamedFramebufferReadBuffer( MomentsFBO, GL_COLOR_ATTACHMENT0 );
   glReadPixels( 0, 0, ShadowMapSize, ShadowMapSize, GL_RG, GL_FLOAT, raw_buffer );

   float max_value = 0.0f;
   for (int i = 0; i < size; ++i) {
      max_value = std::max( max_value, raw_buffer[i * 2] );
   }

   const float inv_max_value = max_value > 0.0f ? 255.0f / max_value : 0.0f;
   for (int i = 0; i < size; ++i) {
      buffer[i] = static_cast<uint8_t>(raw_buffer[i * 2] * inv_max_value);
   }

   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, ShadowMapSize, ShadowMapSize, ShadowMapSize, 8,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, "../sat.png" );
   FreeImage_Unload( image );
   delete [] raw_buffer;
   delete [] buffer;
}

void RendererGL::writeMomentsArrayTexture() const
{
   const int size = ShadowMapSize * ShadowMapSize;
   auto* buffer = new uint8_t[size];
   auto* raw_buffer = new GLfloat[size * 2];
   glBindFramebuffer( GL_FRAMEBUFFER, MomentsLayerFBO );
   for (int s = 0; s < SplitNum; ++s) {
      glNamedFramebufferReadBuffer( MomentsLayerFBO, GL_COLOR_ATTACHMENT0 + s );
      glReadPixels( 0, 0, ShadowMapSize, ShadowMapSize, GL_RG, GL_FLOAT, raw_buffer );

      for (int i = 0; i < size; ++i) {
         buffer[i] = static_cast<uint8_t>(LightCamera->linearizeDepthValue( raw_buffer[i * 2] ) * 255.0f);
      }

      FIBITMAP* image = FreeImage_ConvertFromRawBits(
         buffer, ShadowMapSize, ShadowMapSize, ShadowMapSize, 8,
         FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
      );
      FreeImage_Save( FIF_PNG, image, std::string("../moments" + std::to_string( s ) + ".png").c_str() );
      FreeImage_Unload( image );
   }
   delete [] raw_buffer;
   delete [] buffer;
}

void RendererGL::cleanup(GLFWwindow* window)
{
   glfwSetWindowShouldClose( window, GLFW_TRUE );
}

void RendererGL::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (action != GLFW_PRESS) return;

   switch (key) {
      case GLFW_KEY_1:
         if (!Renderer->Pause) {
            Renderer->AlgorithmToCompare = ALGORITHM_TO_COMPARE::PCF;
            std::cout << ">> Percentage-Closer Filtering Selected\n";
         }
         break;
      case GLFW_KEY_2:
         if (!Renderer->Pause) {
            Renderer->AlgorithmToCompare = ALGORITHM_TO_COMPARE::VSM;
            std::cout << ">> Variance Shadow Map Selected\n";
         }
         break;
      case GLFW_KEY_3:
         if (!Renderer->Pause) {
            Renderer->AlgorithmToCompare = ALGORITHM_TO_COMPARE::PSVSM;
            std::cout << ">> Parallel-Split Variance Shadow Map Selected\n";
         }
         break;
      case GLFW_KEY_4:
         if (!Renderer->Pause) {
            Renderer->AlgorithmToCompare = ALGORITHM_TO_COMPARE::SATVSM;
            std::cout << ">> Summed Area Table Variance Shadow Map Selected\n";
         }
         break;
      case GLFW_KEY_C:
         Renderer->writeFrame();
         std::cout << ">> Framebuffer Captured\n";
         break;
      case GLFW_KEY_D:
         if (Renderer->AlgorithmToCompare == ALGORITHM_TO_COMPARE::PSVSM) {
            Renderer->writeMomentsArrayTexture();
            std::cout << ">> Split Depth Maps Captured\n";
         }
         break;
      case GLFW_KEY_S:
         if (Renderer->AlgorithmToCompare == ALGORITHM_TO_COMPARE::SATVSM) {
            Renderer->writeSATTexture();
            std::cout << ">> SAT Texture Captured\n";
         }
         break;
      case GLFW_KEY_L:
         Renderer->Lights->toggleLightSwitch();
         std::cout << ">> Light Turned " << (Renderer->Lights->isLightOn() ? "On!\n" : "Off!\n");
         break;
      case GLFW_KEY_P: {
         const glm::vec3 pos = Renderer->MainCamera->getCameraPosition();
         std::cout << ">> Camera Position: " << pos.x << ", " << pos.y << ", " << pos.z << "\n";
      } break;
      case GLFW_KEY_SPACE:
         Renderer->Pause = !Renderer->Pause;
         break;
      case GLFW_KEY_Q:
      case GLFW_KEY_ESCAPE:
         cleanup( window );
         break;
      default:
         return;
   }
}

void RendererGL::cursor(GLFWwindow* window, double xpos, double ypos)
{
   if (Renderer->Pause) return;

   if (Renderer->MainCamera->getMovingState()) {
      const auto x = static_cast<int>(std::round( xpos ));
      const auto y = static_cast<int>(std::round( ypos ));
      const int dx = x - Renderer->ClickedPoint.x;
      const int dy = y - Renderer->ClickedPoint.y;
      Renderer->MainCamera->moveForward( -dy );
      Renderer->MainCamera->rotateAroundWorldY( -dx );

      if (glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS) {
         Renderer->MainCamera->pitch( -dy );
      }

      Renderer->ClickedPoint.x = x;
      Renderer->ClickedPoint.y = y;
   }
}

void RendererGL::mouse(GLFWwindow* window, int button, int action, int mods)
{
   if (Renderer->Pause) return;

   if (button == GLFW_MOUSE_BUTTON_LEFT) {
      const bool moving_state = action == GLFW_PRESS;
      if (moving_state) {
         double x, y;
         glfwGetCursorPos( window, &x, &y );
         Renderer->ClickedPoint.x = static_cast<int>(std::round( x ));
         Renderer->ClickedPoint.y = static_cast<int>(std::round( y ));
      }
      Renderer->MainCamera->setMovingState( moving_state );
   }
}

void RendererGL::mousewheel(GLFWwindow* window, double xoffset, double yoffset)
{
   if (Renderer->Pause) return;

   if (yoffset >= 0.0) Renderer->MainCamera->zoomIn();
   else Renderer->MainCamera->zoomOut();
}

void RendererGL::registerCallbacks() const
{
   glfwSetWindowCloseCallback( Window, cleanup );
   glfwSetKeyCallback( Window, keyboard );
   glfwSetCursorPosCallback( Window, cursor );
   glfwSetMouseButtonCallback( Window, mouse );
   glfwSetScrollCallback( Window, mousewheel );
}

void RendererGL::setLights() const
{
   const glm::vec4 light_position(500.0f, 500.0f, 700.0f, 0.0f);
   const glm::vec4 ambient_color(1.0f, 1.0f, 1.0f, 1.0f);
   const glm::vec4 diffuse_color(0.9f, 0.9f, 0.9f, 1.0f);
   const glm::vec4 specular_color(0.9f, 0.9f, 0.9f, 1.0f);
   Lights->addLight( light_position, ambient_color, diffuse_color, specular_color );
}

void RendererGL::setObject() const
{
   const std::string sample_directory_path = std::string(CMAKE_SOURCE_DIR) + "/samples";
   Object->setObject(
      GL_TRIANGLES,
      std::string(sample_directory_path + "/Buddha/buddha.obj")
   );
   Object->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
}

void RendererGL::setWallObject() const
{
   std::vector<glm::vec3> wall_vertices;
   wall_vertices.emplace_back( BoxHalfSide, 0.0f, BoxHalfSide );
   wall_vertices.emplace_back( BoxHalfSide, 0.0f, -BoxHalfSide );
   wall_vertices.emplace_back( -BoxHalfSide, 0.0f, -BoxHalfSide );

   wall_vertices.emplace_back( -BoxHalfSide, 0.0f, BoxHalfSide );
   wall_vertices.emplace_back( BoxHalfSide, 0.0f, BoxHalfSide );
   wall_vertices.emplace_back( -BoxHalfSide, 0.0f, -BoxHalfSide );

   std::vector<glm::vec3> wall_normals;
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   WallObject->setObject( GL_TRIANGLES, wall_vertices, wall_normals );
   WallObject->setDiffuseReflectionColor( { 0.39f, 0.35f, 0.52f, 1.0f } );
}

void RendererGL::setLightViewFrameBuffers()
{
   glCreateTextures( GL_TEXTURE_2D, 1, &DepthTextureID );
   glTextureStorage2D( DepthTextureID, 1, GL_DEPTH_COMPONENT32F, ShadowMapSize, ShadowMapSize );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );

   glCreateFramebuffers( 1, &DepthFBO );
   glNamedFramebufferTexture( DepthFBO, GL_DEPTH_ATTACHMENT, DepthTextureID, 0 );

   if (glCheckNamedFramebufferStatus( DepthFBO, GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "DepthFBO Setup Error\n";
   }

   glCreateTextures( GL_TEXTURE_2D, 1, &MomentsTextureID );
   glTextureStorage2D( MomentsTextureID, 1, GL_RG32F, ShadowMapSize, ShadowMapSize );
   glTextureParameteri( MomentsTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
   glTextureParameteri( MomentsTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTextureParameteri( MomentsTextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
   glTextureParameteri( MomentsTextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

   glCreateFramebuffers( 1, &MomentsFBO );
   glNamedFramebufferTexture( MomentsFBO, GL_COLOR_ATTACHMENT0, MomentsTextureID, 0 );
   glNamedFramebufferTexture( MomentsFBO, GL_DEPTH_ATTACHMENT, DepthTextureID, 0 );

   if (glCheckNamedFramebufferStatus( MomentsFBO, GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "MomentsFBO Setup Error\n";
   }

   glCreateTextures( GL_TEXTURE_2D_ARRAY, 1, &MomentsTextureArrayID );
   glTextureStorage3D( MomentsTextureArrayID, 1, GL_RG32F, ShadowMapSize, ShadowMapSize, SplitNum );
   glTextureParameteri( MomentsTextureArrayID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
   glTextureParameteri( MomentsTextureArrayID, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTextureParameteri( MomentsTextureArrayID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
   glTextureParameteri( MomentsTextureArrayID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

   glCreateFramebuffers( 1, &MomentsLayerFBO );
   for (int i = 0; i < SplitNum; ++i) {
      glNamedFramebufferTextureLayer( MomentsLayerFBO, GL_COLOR_ATTACHMENT0 + i, MomentsTextureArrayID, 0, i );
   }
   glNamedFramebufferTexture( MomentsLayerFBO, GL_DEPTH_ATTACHMENT, DepthTextureID, 0 );

   if (glCheckNamedFramebufferStatus( MomentsLayerFBO, GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "MomentsLayerFBO Setup Error\n";
   }

   glCreateTextures( GL_TEXTURE_2D, 1, &SATTextureID );
   glTextureStorage2D( SATTextureID, 1, GL_RG32F, ShadowMapSize, ShadowMapSize );
   glTextureParameteri( SATTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
   glTextureParameteri( SATTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTextureParameteri( SATTextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
   glTextureParameteri( SATTextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
}

void RendererGL::drawObject(ShaderGL* shader, CameraGL* camera) const
{
   glBindVertexArray( Object->getVAO() );
   glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, Object->getIBO() );
   Object->transferUniformsToShader( shader );

   const glm::mat4 to_object =
      glm::translate( glm::mat4(1.0f), glm::vec3(0.0f, 80.0f, 0.0f) ) *
      glm::rotate( glm::mat4(1.0f), glm::radians( -90.0f ), glm::vec3(1.0f, 0.0f, 0.0f) ) *
      glm::scale( glm::mat4(1.0f), glm::vec3(0.3f) );
   glm::mat4 to_world = glm::translate( glm::mat4(1.0f), glm::vec3(100.0f, 0.0f, -200.0f) ) * to_object;
   shader->transferBasicTransformationUniforms( to_world, camera );
   glDrawElements( Object->getDrawMode(), Object->getIndexNum(), GL_UNSIGNED_INT, nullptr );

   to_world = glm::translate( glm::mat4(1.0f), glm::vec3(-100.0f, 0.0f, 0.0f) ) * to_object;
   shader->transferBasicTransformationUniforms( to_world, camera );
   glDrawElements( Object->getDrawMode(), Object->getIndexNum(), GL_UNSIGNED_INT, nullptr );

   to_world = glm::translate( glm::mat4(1.0f), glm::vec3(300.0f, 0.0f, 250.0f) ) * to_object;
   shader->transferBasicTransformationUniforms( to_world, camera );
   glDrawElements( Object->getDrawMode(), Object->getIndexNum(), GL_UNSIGNED_INT, nullptr );
}

void RendererGL::drawBoxObject(ShaderGL* shader, const CameraGL* camera) const
{
   shader->transferBasicTransformationUniforms( glm::mat4(1.0f), camera );
   WallObject->transferUniformsToShader( shader );
   glBindVertexArray( WallObject->getVAO() );
   glDrawArrays( WallObject->getDrawMode(), 0, WallObject->getVertexNum() );
}

void RendererGL::drawDepthMapFromLightView() const
{
   glViewport( 0, 0, ShadowMapSize, ShadowMapSize );
   glBindFramebuffer( GL_FRAMEBUFFER, DepthFBO );

   constexpr GLfloat one = 1.0f;
   glClearNamedFramebufferfv( DepthFBO, GL_DEPTH, 0, &one );
   glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

   glUseProgram( LightViewDepthShader->getShaderProgram() );
   drawObject( LightViewDepthShader.get(), LightCamera.get() );
   drawBoxObject( LightViewDepthShader.get(), LightCamera.get() );

   glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
}

void RendererGL::drawMomentsMapFromLightView() const
{
   glViewport( 0, 0, ShadowMapSize, ShadowMapSize );
   glBindFramebuffer( GL_FRAMEBUFFER, MomentsFBO );

   constexpr std::array<GLfloat, 2> clear_moments = { 1.0f, 1.0f };
   glClearNamedFramebufferfv( MomentsFBO, GL_COLOR, 0, &clear_moments[0] );

   constexpr GLfloat one = 1.0f;
   glClearNamedFramebufferfv( MomentsFBO, GL_DEPTH, 0, &one );

   glUseProgram( LightViewMomentsShader->getShaderProgram() );
   drawObject( LightViewMomentsShader.get(), LightCamera.get() );
   drawBoxObject( LightViewMomentsShader.get(), LightCamera.get() );
}

void RendererGL::drawMomentsArrayMapFromLightView() const
{
   glViewport( 0, 0, ShadowMapSize, ShadowMapSize );
   glBindFramebuffer( GL_FRAMEBUFFER, MomentsLayerFBO );

   constexpr GLfloat one = 1.0f;
   constexpr std::array<GLfloat, 2> clear_moments = { 1.0f, 1.0f };

   glUseProgram( LightViewMomentsArrayShader->getShaderProgram() );
   LightViewMomentsArrayShader->uniformMat4fv( "LightViewProjectionMatrix", LightViewProjectionMatrices );

   for (int i = 0; i < SplitNum; ++i) {
      const GLenum draw_buffer = GL_COLOR_ATTACHMENT0 + i;
      glNamedFramebufferDrawBuffers( MomentsLayerFBO, 1, &draw_buffer );
      glClearNamedFramebufferfv( MomentsLayerFBO, GL_COLOR, 0, &clear_moments[0] );
      glClearNamedFramebufferfv( MomentsLayerFBO, GL_DEPTH, 0, &one );

      LightViewMomentsArrayShader->uniform1i( "TextureIndex", i );
      drawObject( LightViewMomentsArrayShader.get(), LightCamera.get() );
      drawBoxObject( LightViewMomentsArrayShader.get(), LightCamera.get() );
   }
}

void RendererGL::splitViewFrustum()
{
   constexpr float split_weight = 0.5f;
   const float n = MainCamera->getNearPlane();
   const float f = MainCamera->getFarPlane();
   SplitPositions.resize( SplitNum + 1 );
   SplitPositions[0] = n;
   SplitPositions[SplitNum] = f;
   for (int i = 1; i < SplitNum; ++i) {
      const auto r = static_cast<float>(i) / static_cast<float>(SplitNum);
      const float logarithmic_split = n * std::pow( f / n, r );
      const float uniform_split = n + (f - n) * r;
      SplitPositions[i] = glm::mix( uniform_split, logarithmic_split, split_weight );
   }
}

void RendererGL::calculateLightCropMatrices()
{
   const glm::mat4& projection_matrix = MainCamera->getProjectionMatrix();
   const float scale_x = 1.0f / projection_matrix[0][0];
   const float scale_y = 1.0f / projection_matrix[1][1];

   LightViewProjectionMatrices.clear();
   const glm::mat4 light_view_projection_matrix = LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix();
   for (int s = 0; s < SplitNum; ++s) {
      const float near = SplitPositions[s];
      const float far = SplitPositions[s + 1];
      const float near_plane_half_width = near * scale_x;
      const float near_plane_half_height = near * scale_y;
      const float far_plane_half_width = far * scale_x;
      const float far_plane_half_height = far * scale_y;

      std::array<glm::vec3, 8> frustum{};
      frustum[0] = glm::vec3(-near_plane_half_width, -near_plane_half_height, -near);
      frustum[1] = glm::vec3(-near_plane_half_width, near_plane_half_height, -near);
      frustum[2] = glm::vec3(near_plane_half_width, near_plane_half_height, -near);
      frustum[3] = glm::vec3(near_plane_half_width, -near_plane_half_height, -near);

      frustum[4] = glm::vec3(-far_plane_half_width, -far_plane_half_height, -far);
      frustum[5] = glm::vec3(-far_plane_half_width, far_plane_half_height, -far);
      frustum[6] = glm::vec3(far_plane_half_width, far_plane_half_height, -far);
      frustum[7] = glm::vec3(far_plane_half_width, -far_plane_half_height, -far);

      std::array<glm::vec4, 8> ndc_points{};
      const glm::mat4 to_light =
         LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix() * glm::inverse( MainCamera->getViewMatrix() );
      for (int i = 0; i < 8; ++i) {
         ndc_points[i] = to_light * glm::vec4(frustum[i], 1.0f);
         ndc_points[i].x /= ndc_points[i].w;
         ndc_points[i].y /= ndc_points[i].w;
         ndc_points[i].z /= ndc_points[i].w;
      }

      auto min_point = glm::vec3(std::numeric_limits<float>::max());
      auto max_point = glm::vec3(std::numeric_limits<float>::lowest());
      for (int i = 0; i < 8; ++i) {
         if (ndc_points[i].x < min_point.x) min_point.x = ndc_points[i].x;
         if (ndc_points[i].y < min_point.y) min_point.y = ndc_points[i].y;
         if (ndc_points[i].z < min_point.z) min_point.z = ndc_points[i].z;

         if (ndc_points[i].x > max_point.x) max_point.x = ndc_points[i].x;
         if (ndc_points[i].y > max_point.y) max_point.y = ndc_points[i].y;
         if (ndc_points[i].z > max_point.z) max_point.z = ndc_points[i].z;
      }
      min_point.z = -1.0f;

      constexpr float min_filter_width = 1.0f;
      const glm::vec2 half_min_filter_width_in_ndc(
         min_filter_width / static_cast<float>(FrameWidth),
         min_filter_width / static_cast<float>(FrameHeight)
      );
      min_point.x -= half_min_filter_width_in_ndc.x;
      min_point.y -= half_min_filter_width_in_ndc.y;
      max_point.x += half_min_filter_width_in_ndc.x;
      max_point.y += half_min_filter_width_in_ndc.y;

      min_point = glm::clamp( min_point, -1.0f, 1.0f );
      max_point = glm::clamp( max_point, -1.0f, 1.0f );

      glm::mat4 crop(1.0f);
      crop[0][0] = 2.0f / (max_point.x - min_point.x);
      crop[1][1] = 2.0f / (max_point.y - min_point.y);
      crop[2][2] = 2.0f / (max_point.z - min_point.z);
      crop[3][0] = -0.5f * (max_point.x + min_point.x) * crop[0][0];
      crop[3][1] = -0.5f * (max_point.y + min_point.y) * crop[1][1];
      crop[3][2] = -0.5f * (max_point.z + min_point.z) * crop[2][2];
      LightViewProjectionMatrices.emplace_back( crop * light_view_projection_matrix );
   }
}

void RendererGL::generateSummedAreaTable() const
{
   glm::ivec2 offsets(0, 0);
   constexpr int samples = 4;
   const int g = getGroupSize( ShadowMapSize );
   glUseProgram( SATShader->getShaderProgram() );
   SATShader->uniform1i( "Size", ShadowMapSize );

   // textures must be set to GL_CLAMP_TO_BORDER with the border value 0.
   int t = 0;
   const std::array<GLuint, 2> textures = { SATTextureID, MomentsTextureID };
   for (int i = 1; i < ShadowMapSize; i *= samples) {
      offsets.x = i;
      SATShader->uniform2iv( "Offsets", offsets );
      glBindImageTexture( 0, textures[t], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F );
      glBindImageTexture( 1, textures[t ^ 1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F );
      glDispatchCompute( g, g, 1 );
      glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
      t ^= 1;
   }

   offsets.x = 0;
   for (int i = 1; i < ShadowMapSize; i *= samples) {
      offsets.y = i;
      SATShader->uniform2iv( "Offsets", offsets );
      glBindImageTexture( 0, textures[t], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F );
      glBindImageTexture( 1, textures[t ^ 1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F );
      glDispatchCompute( g, g, 1 );
      glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
      t ^= 1;
   }

   assert( t == 0 );
}

void RendererGL::drawShadowWithPCF() const
{
   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( PCFSceneShader->getShaderProgram() );

   Lights->transferUniformsToShader( PCFSceneShader.get() );
   PCFSceneShader->uniform1i( "LightIndex", ActiveLightIndex );

   const glm::mat4 view_projection = LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix();
   PCFSceneShader->uniformMat4fv( "LightViewProjectionMatrix", view_projection );

   glBindTextureUnit( 0, DepthTextureID );
   drawObject( PCFSceneShader.get(), MainCamera.get() );
   drawBoxObject( PCFSceneShader.get(), MainCamera.get() );
}

void RendererGL::drawShadowWithVSM() const
{
   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( VSMSceneShader->getShaderProgram() );

   Lights->transferUniformsToShader( VSMSceneShader.get() );
   VSMSceneShader->uniform1i( "LightIndex", ActiveLightIndex );

   const glm::mat4 view_projection = LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix();
   VSMSceneShader->uniformMat4fv( "LightViewProjectionMatrix", view_projection );

   glBindTextureUnit( 0, MomentsTextureID );
   drawObject( VSMSceneShader.get(), MainCamera.get() );
   drawBoxObject( VSMSceneShader.get(), MainCamera.get() );
}

void RendererGL::drawShadowWithPSVSM() const
{
   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( PSVSMSceneShader->getShaderProgram() );

   Lights->transferUniformsToShader( PSVSMSceneShader.get() );
   PSVSMSceneShader->uniform1i( "LightIndex", ActiveLightIndex );
   PSVSMSceneShader->uniform1fv( "SplitPositions", SplitNum, SplitPositions.data() );
   PSVSMSceneShader->uniformMat4fv( "LightViewProjectionMatrix", LightViewProjectionMatrices );

   glBindTextureUnit( 0, MomentsTextureArrayID );
   drawObject( PSVSMSceneShader.get(), MainCamera.get() );
   drawBoxObject( PSVSMSceneShader.get(), MainCamera.get() );
}

void RendererGL::drawShadowWithSATVSM() const
{
   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( SATVSMSceneShader->getShaderProgram() );

   Lights->transferUniformsToShader( SATVSMSceneShader.get() );
   SATVSMSceneShader->uniform1i( "LightIndex", ActiveLightIndex );

   const glm::mat4 view_projection = LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix();
   SATVSMSceneShader->uniformMat4fv( "LightViewProjectionMatrix", view_projection );

   glBindTextureUnit( 0, MomentsTextureID );
   drawObject( SATVSMSceneShader.get(), MainCamera.get() );
   drawBoxObject( SATVSMSceneShader.get(), MainCamera.get() );
}

void RendererGL::drawText(const std::string& text, glm::vec2 start_position) const
{
   std::vector<TextGL::Glyph*> glyphs;
   Texter->getGlyphsFromText( glyphs, text );

   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( TextShader->getShaderProgram() );

   glEnable( GL_BLEND );
   glBlendFunc( GL_SRC_ALPHA, GL_ONE );
   glDisable( GL_DEPTH_TEST );

   glm::vec2 text_position = start_position;
   const ObjectGL* glyph_object = Texter->getGlyphObject();
   glBindVertexArray( glyph_object->getVAO() );
   for (const auto& glyph : glyphs) {
      if (glyph->IsNewLine) {
         text_position.x = start_position.x;
         text_position.y -= Texter->getFontSize();
         continue;
      }

      const glm::vec2 position(
         std::round( text_position.x + glyph->Bearing.x ),
         std::round( text_position.y + glyph->Bearing.y - glyph->Size.y )
      );
      const glm::mat4 to_world =
         glm::translate( glm::mat4(1.0f), glm::vec3(position, 0.0f) ) *
         glm::scale( glm::mat4(1.0f), glm::vec3(glyph->Size.x, glyph->Size.y, 1.0f) );
      TextShader->transferBasicTransformationUniforms( to_world, TextCamera.get() );
      TextShader->uniform2fv( "TextScale", glyph->TopRightTextureCoord );
      glBindTextureUnit( 0, glyph_object->getTextureID( glyph->TextureIDIndex ) );
      glDrawArrays( glyph_object->getDrawMode(), 0, glyph_object->getVertexNum() );

      text_position.x += glyph->Advance.x;
      text_position.y -= glyph->Advance.y;
   }
   glEnable( GL_DEPTH_TEST );
   glDisable( GL_BLEND );
}

void RendererGL::render()
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   LightCamera->updateCameraView(
      glm::vec3(Lights->getLightPosition( ActiveLightIndex )),
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f)
   );

   std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

   switch (AlgorithmToCompare) {
      case ALGORITHM_TO_COMPARE::PCF:
         drawDepthMapFromLightView();
         drawShadowWithPCF();
         break;
      case ALGORITHM_TO_COMPARE::VSM:
         drawMomentsMapFromLightView();
         drawShadowWithVSM();
         break;
      case ALGORITHM_TO_COMPARE::PSVSM:
         splitViewFrustum();
         calculateLightCropMatrices();
         drawMomentsArrayMapFromLightView();
         drawShadowWithPSVSM();
         break;
      case ALGORITHM_TO_COMPARE::SATVSM:
         drawMomentsMapFromLightView();
         generateSummedAreaTable();
         drawShadowWithSATVSM();
         break;
   }

   std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
   const auto fps = 1E+6 / static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

   std::stringstream text;
   text << std::fixed << std::setprecision( 2 ) << fps << " fps";
   drawText( text.str(), { 80.0f, 100.0f } );
}

void RendererGL::play()
{
   if (glfwWindowShouldClose( Window )) initialize();

   setLights();
   setObject();
   setWallObject();
   setLightViewFrameBuffers();
   TextShader->setTextUniformLocations();
   PCFSceneShader->setSceneUniformLocations( 1 );
   VSMSceneShader->setSceneUniformLocations( 1 );
   PSVSMSceneShader->setPSSMSceneUniformLocations( 1 );
   SATVSMSceneShader->setSceneUniformLocations( 1 );
   LightViewDepthShader->setLightViewUniformLocations();
   LightViewMomentsShader->setLightViewUniformLocations();
   LightViewMomentsArrayShader->setLightViewArrayUniformLocations();
   SATShader->setSATUniformLocations();

   while (!glfwWindowShouldClose( Window )) {
      if (!Pause) render();

      glfwSwapBuffers( Window );
      glfwPollEvents();
   }
   glfwDestroyWindow( Window );
}