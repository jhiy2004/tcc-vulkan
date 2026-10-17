#include "opengl_renderer.h"

#include "util.h"

#include "imgui_menu.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

void OpenGLRenderer::init(IWindow *window, 
                          const std::vector<glm::vec2>& grid,
                          const std::vector<float>& bathymetryZ,
                          const std::vector<Triangle>& triangles,
                          float x_extent,
                          float y_extent
                          ) {
    _window_width = window->get_width();
    _window_height = window->get_height();

    #ifndef NDEBUG
        std::cerr << "Window size: " << _window_width << " x " << _window_height << "\n";
    #endif

    _window = window->get_window();

    create_shader();
    create_vbos(grid, bathymetryZ, triangles);

    init_imgui();

    // Configurar posicao inicial da camera
    float xcenter{x_extent / 2};
    float ycenter{y_extent / 2};
    
    camera.setTarget(glm::vec3{xcenter, ycenter, 0});
}

void OpenGLRenderer::draw(AppInfo& info, PlaybackState& playback_state, SimulationMetadata& metadata, RenderSettings& settings) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_program);
    glBindVertexArray(_vao);

    // A grade XY é permanente e já está associada
    // ao atributo 0 do VAO.
    glBindBuffer(GL_ARRAY_BUFFER, _grid_xy_vbo);

    // --------------------------------------------------
    // 1. Renderizar batimetria
    // --------------------------------------------------
    glBindBuffer(GL_ARRAY_BUFFER, _bathymetry_z_vbo);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float),
        nullptr
    );

    glUniform1i(
        _render_mode_location,
        settings.bathymetryRenderMode
    );

    glUniform1f(
        _z_scale_location,
        settings.bathymetryZScale
    );

    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(_indices_size),
        GL_UNSIGNED_INT,
        nullptr
    );

    // --------------------------------------------------
    // 2. Renderizar superfície da simulação
    // --------------------------------------------------

    glBindBuffer(GL_ARRAY_BUFFER, _frame_z_vbo);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float),
        nullptr
    );

    glUniform1i(
        _render_mode_location,
        settings.surfaceRenderMode
    );

    glUniform1f(
        _z_scale_location,
        settings.surfaceZScale
    );

    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(_indices_size),
        GL_UNSIGNED_INT,
        nullptr
    );

    glBindVertexArray(0);
    glUseProgram(0);

    draw_imgui(info, playback_state, metadata, settings);
}

Camera& OpenGLRenderer::get_camera() {
    return camera;
}

void OpenGLRenderer::update_frame_z_data(Frame& frame) {
    const size_t size = frame.z_data.size() * sizeof(float);

    if (size > _frame_z_capacity) {
        glBindBuffer(GL_ARRAY_BUFFER, _frame_z_vbo);

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(size),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        _frame_z_capacity = size;
    }

    glBindBuffer(GL_ARRAY_BUFFER, _frame_z_vbo);

    if (size > 0) {
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(size),
            frame.z_data.data()
        );
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}



void OpenGLRenderer::update_scene(
    float zmin,
    float zmax,
    float dt
) {
    camera.update(dt);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();

    float aspect = static_cast<float>(_window_width) /
                   static_cast<float>(_window_height);

    glm::mat4 proj = glm::perspective(
        glm::radians(70.0f),
        aspect,
        0.1f,
        1000.0f
    );
    
    glm::mat4 mvp = proj * view * model;

    glUseProgram(shader_program);
    glUniformMatrix4fv(
        _mvp_location,
        1,
        GL_FALSE,
        glm::value_ptr(mvp)
    );

    glUniform1f(_z_min_location, zmin);
    glUniform1f(_z_max_location, zmax);
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

void OpenGLRenderer::init_imgui() {
    // O contexto OpenGL da janela precisa estar ativo aqui.
    if (!_window) {
        throw std::runtime_error("Window is nullptr");
    }

    // Cria o contexto do Dear ImGui.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::SetCurrentContext(ImGui::GetCurrentContext());

    // Configura o ImGui.
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Inicializa o backend GLFW.
    if (!ImGui_ImplGlfw_InitForOpenGL(_window, true)) {
        throw std::runtime_error("Failed ImGui GLFW init");
    }

    // Escolha a versão GLSL conforme o contexto OpenGL criado.
    const char* glsl_version = "#version 330";

    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        ImGui_ImplGlfw_Shutdown();
        throw std::runtime_error("Failed ImGui OpenGL init");
    }

#ifndef NDEBUG
    std::cout << "Initialized ImGui OpenGL" << std::endl;
#endif
}


void OpenGLRenderer::draw_imgui(AppInfo& info, PlaybackState& playback_state, SimulationMetadata& metadata, RenderSettings& settings) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    
    draw_imgui_menu(info, playback_state, metadata, settings, camera);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void OpenGLRenderer::create_vbos(
    const std::vector<glm::vec2>& grid,
    const std::vector<float>& bathymetryZ,
    const std::vector<Triangle>& triangles
) {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    // Buffer permanente de coordenadas XY
    glGenBuffers(1, &_grid_xy_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _grid_xy_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        grid.size() * sizeof(glm::vec2),
        grid.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(float), nullptr
    );

    // Buffer permanente de profundidades Z
    glGenBuffers(1, &_bathymetry_z_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _bathymetry_z_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        bathymetryZ.size() * sizeof(float),
        bathymetryZ.data(),
        GL_STATIC_DRAW
    );

    glGenBuffers(1, &_frame_z_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _frame_z_vbo);

    _frame_z_capacity = bathymetryZ.size() * sizeof(float);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(_frame_z_capacity),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 1, GL_FLOAT, GL_FALSE,
        sizeof(float), nullptr
    );

    // Buffer permanente de índices dos triângulos
    glGenBuffers(1, &_triangles_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _triangles_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        triangles.size() * sizeof(Triangle),
        triangles.data(),
        GL_STATIC_DRAW
    );
    _indices_size = triangles.size() * 3;

    // O EBO permanece associado ao VAO.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    _mvp_location = glGetUniformLocation(shader_program, "mvp");
    _render_mode_location = glGetUniformLocation(shader_program, "renderMode");
    _z_min_location = glGetUniformLocation(shader_program, "zMin");
    _z_max_location = glGetUniformLocation(shader_program, "zMax");
    _z_scale_location = glGetUniformLocation(shader_program, "zScale");
}