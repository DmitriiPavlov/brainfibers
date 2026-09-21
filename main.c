#include <assert.h>    
#include "raylib.h"
#include "import_points.h" 
#include "vtk_primitives.h"
#include "discrete_simulation.h"
#include "bg_renderer.h"

int main(void) {
    VTKdata unfiltered_data = import_vtk("data/001k.vtk");
    printf("data created");
    InitWindow(800, 800, "Brain Fibers sim");

    ZFILTER_MIN = -20.0;
    ZFILTER_MAX = 20.0;
    printf("\nUNFILTERED\n");
    print_diagnostics(&unfiltered_data);
    VTKdata data = run_filter_with_truncation(unfiltered_data, zfilter, 100);
    printf("\nFILTERED\n");
    print_diagnostics(&data);
    populate_neighbor_data(&data);
    printf("\nNEIGHBOR DATA\n");
    print_diagnostics(&data);
    /*populate_intersections(&data);*/
    populate_intersections_optimized(&data);
    printf("\nINTERSECTIONS\n");
    print_diagnostics(&data);
    cache_point_bounds(&(data.points));

    RenderTexture2D out = create_background_texture2D(data);

    DiscreteFrame frame = init_frame(data);

    RenderTexture2D screen_target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    print_diagnostics(&data);
    populate_frame(frame, 0.01, 120);

    bool running = false;
    Rectangle button = { 10, 10, 100, 40 };
    int* particles;
    SetTargetFPS(10);
    while (!WindowShouldClose()) {
        /*printf("Frame step");*/
        BeginTextureMode(screen_target);
            ClearBackground(BLACK); 

            DrawTexturePro( 
                out.texture, 
                (Rectangle){ 0, 0, VIRTUAL_WIDTH, -VIRTUAL_HEIGHT }, 
                (Rectangle){ 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT }, 
                (Vector2){ 0, 0 }, 0.0f, WHITE 
            ); 

            render_discrete_frame(data, frame); 
        EndTextureMode();   

        BeginDrawing();
        ClearBackground(BLACK);
        // Button
        DrawTexturePro(
            screen_target.texture,
            (Rectangle){ 0, 0, VIRTUAL_WIDTH, -VIRTUAL_HEIGHT },
            (Rectangle){ 0, 0, GetScreenWidth(), GetScreenHeight() },
            (Vector2){ 0, 0 },
            0.0f,
            WHITE
        );
        DrawRectangleRec(button, running ? GREEN : RED);
        DrawText(
            running ? "PAUSE" : "PLAY",
            button.x + 20,
            button.y + 10,
            20,
            WHITE
        );

        EndDrawing();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), button)) {
            running = !running;
        }
        if (running){
        for (int i = 0; i < 1; i++)step_once(data,&frame);
        }
    }

    CloseWindow();
    return 0;
}
