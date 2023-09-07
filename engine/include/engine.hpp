#pragma once

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "allocator.hpp"
#include "camera.hpp"
#include "core.hpp"
#include "depth_buffer.hpp"
#include "pipeline_builder.hpp"
#include "scene.hpp"
#include "shader.hpp"
#include "timer.hpp"
#include "uniform_buffer.hpp"
#include "vk_context.hpp"

struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;

namespace hvk {

struct PushConstants {
    glm::mat4 model{};
    glm::mat4 normal_transform{};
};

enum class BufferingMode : usize {
    None = 1u,
    Double,
    Triple,
};

struct FrameData {
    vk::UniqueSemaphore present_semaphore{};
    vk::UniqueSemaphore render_semaphore{};
    vk::UniqueFence render_fence{};
    vk::UniqueCommandPool cmd_pool{};
    vk::UniqueCommandBuffer cmd{};
    UniformBuffer camera_ubo{};
    vk::DescriptorSet descriptor{};
    // NOTE: this descriptor set is freed by the owning pool, and since we are
    //   not using VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, we don't
    //   need to explicitly destroy them in the cleanup method
};

struct WindowData {
    std::string title{};
    i32 width{1600};
    i32 height{800};
    i32 start_x{};
    i32 start_y{};
    bool is_fullscreen{};
    GLFWwindow* handle{nullptr};
    GLFWmonitor* monitor{nullptr};
    const GLFWvidmode* mode{nullptr};
};

class Engine {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    explicit Engine(
        std::string_view title,
        i32 width,
        i32 height,
        BufferingMode buffering = BufferingMode::None
    )
        : _max_frames_in_flight(static_cast<usize>(buffering)),
          _window{std::string{title}, width, height},
          _frames{_max_frames_in_flight} {}

    Engine() = delete;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;
    ~Engine() = default;

    void init();
    void run();
    void update(double dt);
    void render();
    void cleanup();

    void cycle_pipeline();
    void toggle_fullscreen();
    void on_resize();
    void on_window_resize(i32 width, i32 height);
    void on_window_move(i32 x, i32 y);
    void on_focus(bool focused);
    void on_mouse_move(glm::dvec2 pos);
    void on_scroll(double dx, double dy);
    void on_key_press(i32 keycode, i32 mods);

private:
    void init_glfw();
    void init_vulkan();
    void create_scene();
    void init_commands();
    void init_renderpass();
    void create_framebuffers();
    void init_descriptors();
    void create_pipelines();
    void create_sync_obj();
    void load_shaders();
    void recreate_swapchain();
    void destroy_swapchain();

    bool _is_init{};
    bool _focused{};
    bool _resized{};
    usize _frame_count{};
    usize _frame_idx{};
    usize _max_frames_in_flight{2};
    WindowData _window{};
    Timer _timer{};
    Camera _camera{};
    glm::dvec2 _cursor{};
    Scene _scene{};
    UniformBuffer _scene_ubo{};

    UploadContext _upload_ctx{};
    ShaderMap _shaders{};
    DepthBuffer _depth_buffer{};
    vk::UniqueBuffer _vertex_buffer{}, _index_buffer{};
    vk::UniqueDeviceMemory _vertex_buffer_mem{}, _index_buffer_mem{};
    std::vector<FrameData> _frames{};
    vk::UniqueRenderPass _render_pass{};
    std::vector<vk::UniqueFramebuffer> _framebuffers{};
    vk::UniqueDescriptorPool _desc_pool{};
    vk::UniqueDescriptorSetLayout _global_desc_set_layout{};
    usize _pipeline_idx{};
    GraphicsPipeline _gfx_pipelines{};
};

}  // namespace hvk
