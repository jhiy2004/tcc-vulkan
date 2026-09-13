#include "opengl_renderer.h"

#include "util.h"

void OpenGLRenderer::init(IWindow *window, 
                          const std::vector<glm::vec2>& grid,
                          const std::vector<float>& bathymetryZ,
                          const std::vector<Triangle>& triangles,
                          float x_extent,
                          float y_extent
                          ) {
    create_shader();

    // Configurar posicao inicial da camera
    float xcenter{x_extent / 2};
    float ycenter{y_extent / 2};
    
    camera.setTarget(glm::vec3{xcenter, ycenter, 0});
}

void OpenGLRenderer::draw(AppInfo &info, PlaybackState &playback_state, SimulationMetadata& metadata, RenderSettings& settings) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    }; 

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 

    glUseProgram(shader_program);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

Camera& OpenGLRenderer::get_camera() {
    return camera;
}

void OpenGLRenderer::update_frame_z_data(Frame& frame) {
    return;
}

void OpenGLRenderer::update_scene(float zmin, float zmax, float dt) {

}

void OpenGLRenderer::recreate_swap_chain() {
    // No swap chain in OpenGL, so nothing to do here
    return;
}

void OpenGLRenderer::create_shader() {
    auto vert_shader_vec{ read_file(std::filesystem::path(SHADERS_DIR) / "gl_shader.vert")};
    auto frag_shader_vec{ read_file(std::filesystem::path(SHADERS_DIR) / "gl_shader.frag")};

    const GLchar* vert_shader_source = vert_shader_vec.data();
    GLint vert_shader_length = static_cast<GLint>(vert_shader_vec.size());

    const GLchar* frag_shader_source = frag_shader_vec.data();
    GLint frag_shader_length = static_cast<GLint>(frag_shader_vec.size());

    int success;
    char info_log[512];

    // Create vertex shader
    unsigned int vertex_shader;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex_shader, 1, &vert_shader_source, &vert_shader_length);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
        throw std::runtime_error(
            std::string("Failed to compile vertex shader: ") + info_log
        );
    }
    #ifndef NDEBUG
        std::cerr << "Created vertex shader\n";
    #endif

    // Create fragment shader
    unsigned int fragment_shader;
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragment_shader, 1, &frag_shader_source, &frag_shader_length);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
        throw std::runtime_error(
            std::string("Failed to compile fragment shader: ") + info_log
        );
    }
    #ifndef NDEBUG
        std::cerr << "Created fragment shader\n";
    #endif

    // Create program
    shader_program = glCreateProgram();

    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
        throw std::runtime_error(
            std::string("Failed to link program: ") + info_log
        );
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    #ifndef NDEBUG
        std::cerr << "Created shader program\n";
    #endif
}