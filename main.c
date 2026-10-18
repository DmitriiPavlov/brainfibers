#include <assert.h>
#include <time.h>
#include "raylib.h"
#include "import_points.h"
#include "vtk_primitives.h"
#include "discrete_simulation.h"
#include "bg_renderer.h"
#include "continous_simulation.h"

typedef struct {
    bool running;
    bool show_background;
    Rectangle play_button;
    Rectangle bg_button;
} UIState;

// ---------- Setup ----------

VTKdata load_and_prepare_data(const char* path) {
    VTKdata unfiltered_data = import_vtk(path);
    printf("data created");

    ZFILTER_MIN = -10.0;
    ZFILTER_MAX = 10.0;
    printf("\nUNFILTERED\n");
    print_diagnostics(&unfiltered_data);

    VTKdata data = run_filter_with_truncation(unfiltered_data, zfilter, 100);
    printf("\nFILTERED\n");
    print_diagnostics(&data);

    populate_neighbor_data(&data);
    printf("\nNEIGHBOR DATA\n");
    print_diagnostics(&data);

    populate_intersections_optimized(&data);
    printf("\nINTERSECTIONS\n");
    print_diagnostics(&data);

    cache_point_bounds(&(data.points));

    return data;
}

UIState init_ui(void) {
    UIState ui = {0};
    ui.running = false;
    ui.show_background = true;
    ui.play_button = (Rectangle){ 10, 10, 100, 40 };
    ui.bg_button   = (Rectangle){ 120, 10, 140, 40 };
    return ui;
}

// ---------- Input ----------

void handle_input(UIState* ui) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, ui->play_button)) {
            ui->running = !ui->running;
        }
        if (CheckCollisionPointRec(mouse, ui->bg_button)) {
            ui->show_background = !ui->show_background;
        }
    }
}

// ---------- Update ----------

double step_simulation(VTKdata data, discreteFrame frame) {
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < 1; i++) step_dframe(data, frame, 0.1);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
}

// ---------- Render ----------


void render_cframe_scene_to_texture(RenderTexture2D screen_target, RenderTexture2D background,
                              VTKdata data, discreteSpace(float) concentration, bool show_background) {
    BeginTextureMode(screen_target);
        ClearBackground(BLACK);

        if (show_background) {
            DrawTexturePro(
                background.texture,
                (Rectangle){ 0, 0, VIRTUAL_WIDTH, -VIRTUAL_HEIGHT },
                (Rectangle){ 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT },
                (Vector2){ 0, 0 }, 0.0f, WHITE
            );
        }

        render_concentration(data, concentration);
    EndTextureMode();
}

void draw_ui(UIState ui, double step_ms) {
    DrawRectangleRec(ui.play_button, ui.running ? GREEN : RED);
    DrawText(
        ui.running ? "PAUSE" : "PLAY",
        ui.play_button.x + 20,
        ui.play_button.y + 10,
        20,
        WHITE
    );

    DrawRectangleRec(ui.bg_button, ui.show_background ? GREEN : GRAY);
    DrawText(
        ui.show_background ? "HIDE BG" : "SHOW BG",
        ui.bg_button.x + 10,
        ui.bg_button.y + 10,
        20,
        WHITE
    );

    DrawFPS(GetScreenWidth() - 100, 10);

    char step_text[64];
    snprintf(step_text, sizeof(step_text), "step: %.3f ms", step_ms);
    DrawText(step_text, GetScreenWidth() - 150, 40, 20, LIME);
}

void render_frame(RenderTexture2D screen_target, UIState ui, double step_ms) {
    BeginDrawing();
    ClearBackground(BLACK);

    DrawTexturePro(
        screen_target.texture,
        (Rectangle){ 0, 0, VIRTUAL_WIDTH, -VIRTUAL_HEIGHT },
        (Rectangle){ 0, 0, GetScreenWidth(), GetScreenHeight() },
        (Vector2){ 0, 0 }, 0.0f, WHITE
    );

    draw_ui(ui, step_ms);

    EndDrawing();
}

// ---------- Main ----------

int main(void) {
    srand((unsigned)time(NULL));
    InitWindow(800, 800, "Brain Fibers sim");

    VTKdata data = load_and_prepare_data("data/001k.vtk");
    RenderTexture2D background = create_background_texture2D(data);
    discreteFrame frame = init_dframe_v2(data, pdf_super_high, 500, 500, 5000, 0, 25000);
    step_dframe(data, frame, 1.0);
    continuousFrame cframe = init_cframe(data, frame);
    printf("Dimensions concentration: %d %d \n", cframe.concentration->xresolution, cframe.concentration->yresolution);
    fflush(stdout);
    compute_coefficients(data, &cframe, 1.0);
    RenderTexture2D screen_target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);

    print_diagnostics(&data);

    UIState ui = init_ui();
    double step_ms = 0.0;

    SetTargetFPS(1200);

    while (!WindowShouldClose()) {
        handle_input(&ui);

        if (ui.running) {
            step_cframe(cframe);
        }

        render_cframe_scene_to_texture(screen_target, background, data, cframe.concentration, ui.show_background);
        render_frame(screen_target, ui, step_ms);
    }

    CloseWindow();
    return 0;
}
