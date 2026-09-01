#include "types.h"
#include <math.h>
#include <stdlib.h>
#include "raylib.h"

#ifndef RENDERER_H
#define RENDERER_H

#define VIRTUAL_WIDTH 1000
#define VIRTUAL_HEIGHT 1000

//only handles 2D
RenderTexture2D create_background_texture2D(VTKdata data){
    RenderTexture2D out = LoadRenderTexture(VIRTUAL_WIDTH , VIRTUAL_HEIGHT);
    Line line;
    Camera2D camera = {0};

    camera.target = (Vector2){
        (data.points.xmin + data.points.xmax) / 2.0f,
        (data.points.ymin + data.points.ymax) / 2.0f
    };

    camera.offset = (Vector2){
        VIRTUAL_WIDTH / 2.0f,
        VIRTUAL_HEIGHT / 2.0f
    };

    //potential issue: we are only computing zoom based on width
    camera.zoom = amin(VIRTUAL_WIDTH / (data.points.xmax - data.points.xmin), VIRTUAL_HEIGHT / (data.points.ymax - data.points.ymin));

    BeginTextureMode(out);
        ClearBackground(BLACK);
        BeginMode2D(camera);
        for (int i = 0; i < LEN_ARRAY(data.lines.meta_data); i++) {
            for (int j = 0; j + 1 < AT_ARRAY(data.lines.meta_data, i).len; j++){
                DrawLineV(
                    get_projected_point(data, i, j),
                    get_projected_point(data, i, j + 1),
                    WHITE
                );
            }
        }
          for (int i = data.start_intersections; i < LEN_ARRAY(data.points.data); i++){
              DrawCircle(AT_ARRAY(data.points.data, i).x, AT_ARRAY(data.points.data, i).y,
                         sqrtf((float)1) * 0.5f, ORANGE);
          }
        EndMode2D();
    EndTextureMode();

    return out;
}






#endif



