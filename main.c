#include <assert.h>    
#include "raylib.h"
#include "import_points.h" 
#include "bg_renderer.h"

int main(void) {
    VTKdata unfiltered_data = import_vtk("data/001k.vtk");
    printf("data created");
    InitWindow(800, 800, "Brain Fibers sim");
    VTKdata data = run_filter_with_truncation(unfiltered_data, zfilter, 10);

    RenderTexture2D out = create_background_texture2D(data);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            out.texture,
            (Rectangle){ 0, 0, VIRTUAL_WIDTH, -VIRTUAL_HEIGHT },
            (Rectangle){ 0, 0, GetScreenWidth(), GetScreenHeight() },
            (Vector2){ 0, 0 },
            0.0f,
            WHITE
        );

EndDrawing();
    }

    CloseWindow();
    return 0;
}
