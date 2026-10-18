/* Build-time asset preparation. The installed runtime reads WPM1 only. */
#include <WorkphoneGraphics/workphone_graphics_mesh_writer.h>
#include <string.h>

int main(int argc, char **argv)
{
    wp_graphics_mesh *mesh;
    wp_graphics_mesh_vertex_pc vertices[3];
    int i, written;
    if(argc != 2) return 1;
    mesh = wp_graphics_mesh_create();
    if(!mesh) return 2;
    memset(vertices, 0, sizeof(vertices));
    vertices[0].position.x = -0.7f; vertices[0].position.y = -0.7f;
    vertices[1].position.x = 0.7f; vertices[1].position.y = -0.7f;
    vertices[2].position.y = 0.7f;
    for(i = 0; i < 3; ++i) { vertices[i].position.z = 0.5f; vertices[i].color = 0xFFFFFFFFu; }
    written = wp_graphics_mesh_set_vertices(mesh, WORKPHONE_VERTEX_FORMAT_PC, vertices, 3) &&
              wp_graphics_mesh_write(mesh, argv[1]);
    wp_graphics_mesh_destroy(mesh);
    return written ? 0 : 3;
}
