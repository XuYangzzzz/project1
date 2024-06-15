


/**
* Author:Regan Zhu
* Assignment: Simple 2D Scene
* Date due: 2023-06-11, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640*1.8,
              WINDOW_HEIGHT = 480*1.8;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr float ROT_INCREMENT = 1.0f;


constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";


constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr float DEGREES_PER_SECOND     = 1.0f;

constexpr char bw_SPRITE_FILEPATH[]   = "bwtriangle.png",
               neon_SPRITE_FILEPATH[] = "neontriangle.png";

constexpr glm::vec3 INIT_SCALE      = glm::vec3(1.0f, 1.0f, 0.0f),
                    INIT_POS_bw   = glm::vec3(2.0f, 0.0f, 0.0f),
                    INIT_POS_neon = glm::vec3(-2.0f, 0.0f, 0.0f);

AppStatus g_app_status = RUNNING;

SDL_Window* g_display_window;

ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix,
          g_bw_matrix,
          g_neon_matrix,
          g_projection_matrix,
          g_trans_matrix;

float g_triangle_x      = 0.0f;
float g_triangle_y      = 0.0f;
float g_triangle_rotate = 0.0f;
float g_previous_ticks  = 0.0f;

glm::vec3 g_model_movement = glm::vec3(0.0f,0.0f,0.0f);
glm::vec3 g_model_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_bw_pos = glm::vec3(0.0f,0.0f,0.0f);
glm::vec3 g_neon_pos = glm::vec3(0.0f,0.0f,0.0f);

float g_model_speed = 5.0f;  // move 1 unit per second

glm::vec3 g_rotation_bw   = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_neon = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_bw_texture_id,
       g_neon_texture_id;

//for heartbeat scaling stuff
constexpr float GROWTH_FACTOR = 1.1f;  // growth rate of 1.0% per frame
constexpr float SHRINK_FACTOR = 0.9f;  // growth rate of -1.0% per frame
constexpr int MAX_FRAME = 20;           // this value is, of course, up to you

int g_frame_counter = 0;
bool g_is_growing = true;
 
GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}






void initialise()
{
   SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("デルタ時間！！！",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
        
    }
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_bw_matrix       = glm::mat4(1.0f);
    g_neon_matrix     = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    g_bw_texture_id   = load_texture(bw_SPRITE_FILEPATH);
    g_neon_texture_id = load_texture(neon_SPRITE_FILEPATH);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_model_movement = glm::vec3(0.0f);
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch(event.type){
            case SDL_QUIT:
                g_app_status = TERMINATED;
                break;
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;
//                    case SDLK_RIGHT:
//                        g_model_movement.x = 1.0f;
                }
                break;
        }
        break;
    }
//    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
//    if (key_state[SDL_SCANCODE_RIGHT]) g_model_movement.x = 1.0f;
//    else if(key_state[SDL_SCANCODE_LEFT]) g_model_movement.x = -1.0f;
//    if (key_state[SDL_SCANCODE_UP])
//    {
//        g_model_movement.y = 1.0f;
//    }
//    else if (key_state[SDL_SCANCODE_DOWN])
//    {
//        g_model_movement.y = -1.0f;
//    }
//
//    // This makes sure that the player can't "cheat" their way into moving
//    // faster
//    if (glm::length(g_model_movement) > 1.0f)
//    {
//        g_model_movement = glm::normalize(g_model_movement);
//    }
}

void update()
{
    /* Delta Time Calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND; // current # of ticks
    float delta_time = ticks - g_previous_ticks; // tick difference from the last frame
    g_previous_ticks = ticks;
    
    g_triangle_rotate += 1 * delta_time;
    
    //for heartbeat scaling
    glm::vec3 scalevector;
    g_frame_counter += 1;
    if (g_frame_counter >= MAX_FRAME){
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }
    scalevector = glm::vec3(g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                                 g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                                 1.0f);

    
    float r = 1.0f;
    g_rotation_bw.y += ROT_INCREMENT * delta_time;
    g_rotation_neon.y += -1 * ROT_INCREMENT * delta_time;
    /* Update Your Logic*/
//    if(g_model_position.y + model_width < right_limit){
//        g_model_position += g_model_movement * delta_time * g_model_speed;
//    }
    /* Reset the Model Matrix */
    g_bw_matrix = glm::mat4(1.0f);
    g_neon_matrix = glm::mat4(1.0f);
    
    g_bw_pos.x = r * cos(g_triangle_rotate);
    g_bw_pos.y = r * sin(g_triangle_rotate);
    g_neon_pos.x = -r * cos(g_triangle_rotate);
    g_neon_pos.y = -r * sin(g_triangle_rotate);
    
    
    g_bw_matrix = glm::translate(g_bw_matrix, INIT_POS_bw);
    g_bw_matrix = glm::translate(g_bw_matrix, g_bw_pos);
    g_bw_matrix = glm::rotate(g_bw_matrix,
                                 g_rotation_bw.y,
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    g_bw_matrix = glm::scale(g_bw_matrix, INIT_SCALE);
    
    g_neon_matrix = glm::translate(g_neon_matrix, INIT_POS_neon);
    g_neon_matrix = glm::translate(g_neon_matrix, g_neon_pos);
//    g_neon_matrix = glm::scale(g_neon_matrix, INIT_SCALE);
    g_neon_matrix = glm::scale(g_neon_matrix, scalevector);


    /* Translate -> Rotate */
//    g_bw_matrix = glm::translate(g_bw_matrix, g_model_position);
//    g_bw_matrix = glm::rotate(g_bw_matrix, glm::radians(g_triangle_rotate), glm::vec3(0.0f, 0.0f, 1.0f));
}

void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
        
        // Vertices
        float vertices[] =
        {
            -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
            -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
        };

        // Textures
        float texture_coordinates[] =
        {
            0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
            0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
        };
        
        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                              0, vertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());
        
        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                              false, 0, texture_coordinates);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        
        // Bind texture
        draw_object(g_bw_matrix, g_bw_texture_id);
        draw_object(g_neon_matrix, g_neon_texture_id);
        
        // We disable two attribute arrays now
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        
        SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main()
{
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
