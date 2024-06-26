/**
* Author: Rafael de Leon
* Assignment: Pong Clone
* Date due: 2024-06-29, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>

#define LOG(argument) std::cout << argument << '\n'
enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

constexpr float BG_RED = 0.1922f,
BG_BALL = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char BLUE_SPRITE_FILEPATH[] = "assets/guyBlue.png",
PINK_SPRITE_FILEPATH[] = "assets/guyPink.png",
BALL_SPRITE_FILEPATH[] = "assets/ball.png",
HIT_SPRITE_FILEPATH[] = "assets/ballAlt.png";

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
constexpr glm::vec3 INIT_SCALE = glm::vec3(2.5f, 2.5f, 0.0f),
INIT_SCALE_BALL = glm::vec3(1.0f, 1.0f, 0.0f);


SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_blue_matrix, g_pink_matrix, g_projection_matrix, g_trans_matrix, g_ball_matrix;

float g_previous_ticks = 0.0f;

GLuint g_blue_texture_id;
GLuint g_pink_texture_id;
GLuint g_ball_texture_id;
GLuint g_hit_texture_id;

glm::vec3 g_blue_position = glm::vec3(3.5f, 0.0f, 0.0f);
glm::vec3 g_blue_movement = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 g_pink_position = glm::vec3(-3.5f, 0.0f, 0.0f);
glm::vec3 g_pink_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(-2.0f, -2.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(1.0f, 1.0f, 0.0f);
glm::vec3 g_ball_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_spin = glm::vec3(0.0f, 0.0f, -1.0f); // bad naming, but this will be spin direction


float g_blue_speed = 2.4f;  // move 1.6 unit per second
float g_ball_speed = 1.0f;  // move 1 unit per second

bool hit = false;
int hitFrames = 0;
constexpr int HIT_RECOVERY = 360;

bool playerTwo = true;

#define LOG(argument) std::cout << argument << '\n'
void initialise();
void process_input();
void update();
void render();
void shutdown();

float clamp(float value, float min, float max);
bool checkBoxCircleCollision(const glm::vec3& boxCenter, float boxHalfWidth, float boxHalfHeight, const glm::vec3& circleCenter, float circleRadius);

constexpr int NUMBER_OF_TEXTURES = 1; // to be generated, that is
constexpr GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
constexpr GLint TEXTURE_BORDER = 0;   // this value MUST be zero

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
    g_display_window = SDL_CreateWindow("Pawng",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);


    if (g_display_window == nullptr)
    {
        shutdown();
    }
#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_blue_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    g_ball_position += g_ball_movement;

    g_view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BALL, BG_GREEN, BG_OPACITY);

    g_blue_texture_id = load_texture(BLUE_SPRITE_FILEPATH);
    g_pink_texture_id = load_texture(PINK_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    g_hit_texture_id = load_texture(HIT_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    if (playerTwo) g_blue_movement = glm::vec3(0.0f);
    g_pink_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_LEFT:
                // Move the player left
                break;

            case SDLK_RIGHT:
                // Move the player right
                //g_blue_movement.x = 1.0f;
                break;

            case SDLK_t:
                // Quit the game with a keystroke
                playerTwo = !playerTwo;
                if (!playerTwo)
                {
                    g_blue_movement.y = 1.0f;
                }
                break;

            case SDLK_q:
                // Quit the game with a keystroke
                g_app_status = TERMINATED;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }


    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_W])
    {
        g_pink_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_S])
    {
        g_pink_movement.y = -1.0f;
    }

    if (key_state[SDL_SCANCODE_UP])
    {
        if (playerTwo)
        { 
			g_blue_movement.y = 1.0f;
        }
    }
    else if (key_state[SDL_SCANCODE_DOWN])
    {
        if (playerTwo)
        {
            g_blue_movement.y = -1.0f;
        }
    }

    // This makes sure that the player can't "cheat" their way into moving
    // faster
    if (glm::length(g_blue_movement) > 1.0f)
    {
        g_blue_movement = glm::normalize(g_blue_movement);
    }
}

/**
 Uses distance formula.
 */
bool check_collision(glm::vec3& position_a, glm::vec3& position_b)
{
    return sqrt(pow(position_b[0] - position_a[0], 2) + pow(position_b[1] - position_b[1], 2)) < MINIMUM_COLLISION_DISTANCE;
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    if (hit && hitFrames < HIT_RECOVERY)
    {
        hitFrames++;
    }
    else if (hit && hitFrames >= HIT_RECOVERY)
    {
        hitFrames = 0;
        hit = false;
    }

    // Add direction * units per second * elapsed time
    g_blue_position += g_blue_movement * g_blue_speed * delta_time;
    g_pink_position += g_pink_movement * g_blue_speed * delta_time;
    g_ball_position += g_ball_movement * g_ball_speed * delta_time;
    g_ball_rotation.z += 1.0f * delta_time;

    g_blue_matrix = glm::mat4(1.0f);
    g_blue_matrix = glm::translate(g_blue_matrix, g_blue_position);

    g_pink_matrix = glm::mat4(1.0f);
    g_pink_matrix = glm::translate(g_pink_matrix, g_pink_position);

    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);

    g_ball_matrix = glm::rotate(g_ball_matrix,
        g_ball_rotation.z,
        g_ball_spin);

    int collision_box_scale = 12; // restricts collision to the field side of the player, instead of a box a bar
    float x_distance_blue = fabs(g_blue_position.x - ((collision_box_scale/2 - 2)*INIT_SCALE.x/collision_box_scale) - g_ball_position.x) - ((INIT_SCALE.x/collision_box_scale + INIT_SCALE_BALL.x) / 3.0f);
    float y_distance_blue = fabs(g_blue_position.y - g_ball_position.y) - ((INIT_SCALE.y + INIT_SCALE_BALL.y) / 2.7f);
    float x_distance_pink = fabs(g_pink_position.x + ((collision_box_scale/2 - 2)*INIT_SCALE.x/collision_box_scale) - g_ball_position.x) - ((INIT_SCALE.x/collision_box_scale + INIT_SCALE_BALL.x) / 2.7f);
    float y_distance_pink = fabs(g_pink_position.y - g_ball_position.y) - ((INIT_SCALE.y + INIT_SCALE_BALL.y) / 2.7f);

    // ball - blue collision
    if (x_distance_blue < 0 && y_distance_blue < 0)
    {
        std::cout << std::time(nullptr) << ": Collision.\n";
		hit = true;
        g_ball_position.x -= 0.1f;
        g_ball_movement.x *= -1.0f;
        g_ball_spin *= -1.0f;
    }
    // ball - pink collision
    if (x_distance_pink < 0 && y_distance_pink < 0)
    {
        std::cout << std::time(nullptr) << ": Collision.\n";
		hit = true;
        g_ball_position.x += 0.1f;
        g_ball_movement.x *= -1.0f;
        g_ball_spin *= -1.0f;
    }
    // ball - wall collision
    if (g_ball_position.y + INIT_SCALE_BALL.y/2.4 >= 3.75f)
    {
		hit = true;
        g_ball_position.y -= 0.1f;
        g_ball_movement.y *= -1.0f;
    } else if (g_ball_position.y - INIT_SCALE_BALL.y/2.4 <= -3.75f) {
		hit = true;
        g_ball_position.y += 0.1f;
        g_ball_movement.y *= -1.0f;
    }
    if (g_ball_position.x - INIT_SCALE_BALL.x/2.0 >= 5.0f)
    {
		g_app_status = TERMINATED;
    } else if (g_ball_position.x + INIT_SCALE_BALL.x/2.0 <= -5.0f) {
		g_app_status = TERMINATED;
    }
    // blue - wall collision
    if (g_blue_position.y + INIT_SCALE.y/3.0 >= 3.75f)
    {
        g_blue_position.y -= 0.1f;
        g_blue_movement.y *= -1.0f;
    } else if (g_blue_position.y - INIT_SCALE.y/3.0 <= -3.75f) {
        g_blue_position.y += 0.1f;
        g_blue_movement.y *= -1.0f;
    }
    // pink - wall collision
    if (g_pink_position.y + INIT_SCALE.y/2.4 >= 3.75f)
    {
        g_pink_position.y -= 0.1f;
        g_pink_movement.y *= -1.0f;
    } else if (g_pink_position.y - INIT_SCALE.y/2.4 <= -3.75f) {
        g_pink_position.y += 0.1f;
        g_pink_movement.y *= -1.0f;
    }
    g_blue_matrix = glm::scale(g_blue_matrix, INIT_SCALE);
    g_pink_matrix = glm::scale(g_pink_matrix, INIT_SCALE);
    g_ball_matrix = glm::scale(g_ball_matrix, INIT_SCALE_BALL);
}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_blue_matrix, g_blue_texture_id);
    draw_object(g_pink_matrix, g_pink_texture_id);
    if (!hit)
    {
		draw_object(g_ball_matrix, g_ball_texture_id);
    }
    else {
		draw_object(g_ball_matrix, g_hit_texture_id);
    }

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

#include <cmath>

// Struct to represent a 2D vector or point
struct Vec2 {
    float x, y;
};

// Function to clamp a value between min and max, helper function to find closest point on box to circle
float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Function to check box-circle collision
bool checkBoxCircleCollision(const glm::vec3& boxCenter, float boxWidth, float boxHeight, const glm::vec3& circleCenter, float circleRadius) {
    // Find the closest point on the box to the circle
    float closestX = clamp(circleCenter.x, boxCenter.x - boxWidth/2, boxCenter.x + boxWidth/2);
    float closestY = clamp(circleCenter.y, boxCenter.y - boxHeight/2, boxCenter.y + boxHeight/2);

    // Calculate the distance from the circle's center to the closest point
    float distanceX = circleCenter.x - closestX;
    float distanceY = circleCenter.y - closestY;

    // Calculate the squared distance and compare with squared radius
    float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
    float radiusSquared = circleRadius * circleRadius;

    return distanceSquared <= radiusSquared;
}



int main(int argc, char* argv[])
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
