#include "renderer.h"

RendererGL::RendererGL() :
   Window( nullptr ), Pause( false ), FrameWidth( 1920 ), FrameHeight( 1080 ), ShadowMapSize( 1024 ),
   ActiveLightIndex( 0 ), PassNum( 3 ), DepthFBO( 0 ), DepthTextureID( 0 ), MomentsFBO( 0 ), MomentsTextureID( 0 ),
   ClickedPoint( -1, -1 ), Texter( std::make_unique<TextGL>() ), MainCamera( std::make_unique<CameraGL>() ),
   TextCamera( std::make_unique<CameraGL>() ), LightCamera( std::make_unique<CameraGL>() ),
   TextShader( std::make_unique<ShaderGL>() ), PCFSceneShader( std::make_unique<ShaderGL>() ),
   VSMSceneShader( std::make_unique<ShaderGL>() ),
   LightViewDepthShader( std::make_unique<ShaderGL>() ), LightViewMomentsShader( std::make_unique<ShaderGL>() ),
   Lights( std::make_unique<LightGL>() ), Object( std::make_unique<ObjectGL>() ),
   WallObject( std::make_unique<ObjectGL>() ), AlgorithmToCompare( ALGORITHM_TO_COMPARE::VSM )
{
   Renderer = this;

   initialize();
   printOpenGLInformation();
}

RendererGL::~RendererGL()
{
   if (DepthTextureID != 0) glDeleteTextures( 1, &DepthTextureID );
   if (MomentsTextureID != 0) glDeleteTextures( 1, &MomentsTextureID );
   if (DepthFBO != 0) glDeleteFramebuffers( 1, &DepthFBO );
   if (MomentsFBO != 0) glDeleteFramebuffers( 1, &MomentsFBO );
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
   glClearColor( 0.4f, 0.5f, 0.6f, 1.0f );

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
   LightViewDepthShader->setShader(
      std::string(shader_directory_path + "/light_view_depth_generator.vert").c_str(),
      std::string(shader_directory_path + "/light_view_depth_generator.frag").c_str()
   );
   LightViewMomentsShader->setShader(
      std::string(shader_directory_path + "/light_view_moments_generator.vert").c_str(),
      std::string(shader_directory_path + "/light_view_moments_generator.frag").c_str()
   );
}

void RendererGL::writeFrame(const std::string& name) const
{
   const int size = FrameWidth * FrameHeight * 3;
   auto* buffer = new uint8_t[size];
   glPixelStorei( GL_PACK_ALIGNMENT, 1 );
   glReadBuffer( GL_COLOR_ATTACHMENT0 );
   glReadPixels( 0, 0, FrameWidth, FrameHeight, GL_BGR, GL_UNSIGNED_BYTE, buffer );
   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, FrameWidth, FrameHeight, FrameWidth * 3, 24,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, name.c_str() );
   FreeImage_Unload( image );
   delete [] buffer;
}

void RendererGL::writeDepthTexture(const std::string& name) const
{
   const int size = ShadowMapSize * ShadowMapSize;
   auto* buffer = new uint8_t[size];
   auto* raw_buffer = new GLfloat[size];
   glGetTextureImage( DepthTextureID, 0, GL_DEPTH_COMPONENT, GL_FLOAT, static_cast<GLsizei>(size * sizeof( GLfloat )), raw_buffer );

   for (int i = 0; i < size; ++i) {
      buffer[i] = static_cast<uint8_t>(LightCamera->linearizeDepthValue( raw_buffer[i] ) * 255.0f);
   }

   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, ShadowMapSize, ShadowMapSize, ShadowMapSize, 8,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, name.c_str() );
   FreeImage_Unload( image );
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
      case GLFW_KEY_UP:
         if (!Renderer->Pause) {
            Renderer->PassNum++;
            std::cout << ">> Pass Num: " << Renderer->PassNum << std::endl;
         }
         break;
      case GLFW_KEY_DOWN:
         if (!Renderer->Pause) {
            Renderer->PassNum--;
            if (Renderer->PassNum < 2) Renderer->PassNum = 2;
            std::cout << ">> Pass Num: " << Renderer->PassNum << std::endl;
         }
         break;
      case GLFW_KEY_C:
         Renderer->writeFrame( "../result.png" );
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
   const glm::vec4 light_position(500.0f, 500.0f, 500.0f, 0.0f);
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
      std::string(sample_directory_path + "/Panda/panda.obj")
   );
   Object->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
}

void RendererGL::setWallObject() const
{
   constexpr float half_length = 512.0f;
   std::vector<glm::vec3> wall_vertices;
   wall_vertices.emplace_back( half_length, 0.0f, half_length );
   wall_vertices.emplace_back( half_length, 0.0f, -half_length );
   wall_vertices.emplace_back( -half_length, 0.0f, -half_length );

   wall_vertices.emplace_back( -half_length, 0.0f, half_length );
   wall_vertices.emplace_back( half_length, 0.0f, half_length );
   wall_vertices.emplace_back( -half_length, 0.0f, -half_length );

   std::vector<glm::vec3> wall_normals;
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   WallObject->setObject( GL_TRIANGLES, wall_vertices, wall_normals );
   WallObject->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
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
}

void RendererGL::drawObject(ShaderGL* shader, CameraGL* camera) const
{
   const glm::mat4 to_world = glm::scale( glm::mat4(1.0f), glm::vec3(100.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera );
   Object->transferUniformsToShader( shader );

   glBindVertexArray( Object->getVAO() );
   glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, Object->getIBO() );
   glDrawElements( Object->getDrawMode(), Object->getIndexNum(), GL_UNSIGNED_INT, nullptr );
}

void RendererGL::drawBoxObject(ShaderGL* shader, const CameraGL* camera) const
{
   glm::mat4 to_world(1.0f);
   shader->transferBasicTransformationUniforms( to_world, camera );
   WallObject->setDiffuseReflectionColor( { 0.27f, 0.49f, 0.81f, 1.0f } );
   WallObject->transferUniformsToShader( shader );
   glBindVertexArray( WallObject->getVAO() );
   glDrawArrays( WallObject->getDrawMode(), 0, WallObject->getVertexNum() );

   to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(0.0f, 512.0f, -512.0f) ) *
      glm::rotate( glm::mat4(1.0f), glm::radians( 90.0f ), glm::vec3(1.0f, 0.0f, 0.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera );
   WallObject->setDiffuseReflectionColor( { 0.32f, 0.81f, 0.29f, 1.0f } );
   WallObject->transferUniformsToShader( shader );
   glDrawArrays( WallObject->getDrawMode(), 0, WallObject->getVertexNum() );

   to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(-512.0f, 512.0f, 0.0f) ) *
      glm::rotate( glm::mat4(1.0f), glm::radians( -90.0f ), glm::vec3(0.0f, 0.0f, 1.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera );
   WallObject->setDiffuseReflectionColor( { 0.83f, 0.35f, 0.29f, 1.0f } );
   WallObject->transferUniformsToShader( shader );
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

void RendererGL::drawShadowWithPCF() const
{
   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( PCFSceneShader->getShaderProgram() );

   Lights->transferUniformsToShader( PCFSceneShader.get() );
   glUniform1i( PCFSceneShader->getLocation( "LightIndex" ), ActiveLightIndex );

   const glm::mat4 view_projection = LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix();
   glUniformMatrix4fv( PCFSceneShader->getLocation( "LightViewProjectionMatrix" ), 1, GL_FALSE, &view_projection[0][0] );

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
   glUniform1i( VSMSceneShader->getLocation( "LightIndex" ), ActiveLightIndex );

   const glm::mat4 view_projection = LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix();
   glUniformMatrix4fv( VSMSceneShader->getLocation( "LightViewProjectionMatrix" ), 1, GL_FALSE, &view_projection[0][0] );

   glBindTextureUnit( 0, MomentsTextureID );
   drawObject( VSMSceneShader.get(), MainCamera.get() );
   drawBoxObject( VSMSceneShader.get(), MainCamera.get() );
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
      glm::vec3(-1.0f, -1.0f, 0.0f),
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
   LightViewDepthShader->setLightViewUniformLocations();
   LightViewMomentsShader->setLightViewUniformLocations();

   while (!glfwWindowShouldClose( Window )) {
      if (!Pause) render();

      glfwSwapBuffers( Window );
      glfwPollEvents();
   }
   glfwDestroyWindow( Window );
}